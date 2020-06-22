/* rvcc C compiler - IL->binary code generator */

#include <stdio.h>
#include <stdlib.h>

/* calculates stack space needed for function's parameters */
void c_size_function(function_def *fn)
{
	int s = 0, i;

	/* parameters are turned into local variables */
	for (i = 0; i < fn->num_params; i++) {
		int vs = size_variable(&fn->param_defs[i]);
		fn->param_defs[i].offset = s + vs; /* set stack offset */
		s += vs;
	}

	/* align to 16 bytes */
	if ((s & 15) > 0) {
		s = (s - (s & 15)) + 16;
	}
	if (s > 2047) {
		error("Local stack size exceeded");
	}

	fn->params_size = s;
}

/* returns stack size required after block local variables */
int c_size_block(block_def *bd)
{
	int size = 0, i, offset;

	/* our offset starts from parent's offset */
	if (bd->parent == NULL) {
		if (bd->function != NULL) {
			offset = bd->function->params_size;
		} else {
			offset = 0;
		}
	} else {
		offset = c_size_block(bd->parent);
	}

	/* declared locals */
	for (i = 0; i < bd->next_local; i++) {
		int vs = size_variable(&bd->locals[i]);
		bd->locals[i].offset = size + offset + vs; /* for looking up value off stack */
		size += vs;
	}

	/* align to 16 bytes */
	if ((size & 15) > 0) {
		size = (size - (size & 15)) + 16;
	}
	if (size > 2047) {
		error("Local stack size exceeded");
	}

	bd->locals_size = size; /* save in block for stack allocation */
	return size + offset;
}

/* calculate stack necessary sizes for all functions */
void c_size_functions(int data_start)
{
	block_def *bd;
	int i;

	/* size functions */
	for (i = 0; i < _functions_idx; i++) {
		c_size_function(&_functions[i]);
	}

	/* size blocks excl. global block */
	for (i = 1; i < _blocks_idx; i++) {
		c_size_block(&_blocks[i]);
	}

	/* allocate data for globals, in block 0 */
	bd = &_blocks[0];
	for (i = 0; i < bd->next_local; i++) {
		bd->locals[i].offset = _e_data_idx; /* set offset in data section */
		e_add_symbol(bd->locals[i].variable_name, strlen(bd->locals[i].variable_name),
			     data_start + _e_data_idx);
		_e_data_idx += size_variable(&bd->locals[i]);
	}
}

/* returns expected binary length of an IL instruction in bytes */
int c_get_code_length(il_instr *ii)
{
	il_op op = ii->op;

	if (op == op_entry_point) {
		/* depends on number of parameters to push onto the stack */
		function_def *fn = find_function(ii->string_param1);
		return 16 + (fn->num_params << 2);
	}
	if (op == op_exit) {
		return 12;
	}
	if (op == op_load_data_address) {
		return 8;
	}
	if (op == op_function_call) {
		if (ii->param_no + 10 != r_a0) {
			return 8;
		}
		return 4;
	}
	if (op == op_exit_point) {
		return 16;
	}
	if (op == op_jump) {
		return 4;
	}
	if (op == op_return) {
		return 4;
	}
	if (op == op_generic) {
		return 4;
	}
	if (op == op_load_numeric_constant) {
		if (ii->int_param1 > -2048 && ii->int_param1 < 2047) {
			return 4;
		}
		return 8;
	}
	if (op == op_add) {
		return 4;
	}
	if (op == op_sub) {
		return 4;
	}
	if (op == op_mul) {
		return 4;
	}
	if (op == op_label) {
		return 0;
	}
	if (op == op_jz || op == op_jnz) {
		return 8;
	}
	if (op == op_equals || op == op_not_equals || op == op_less_than || op == op_less_eq_than ||
	    op == op_greater_than || op == op_greater_eq_than) {
		return 16;
	}
	if (op == op_push || op == op_pop) {
		return 8;
	}
	if (op == op_block_start || op == op_block_end) {
		block_def *bd = &_blocks[ii->int_param1];
		if (bd->next_local > 0) {
			return 4;
		} else {
			return 0;
		}
	}
	if (op == op_get_var_addr) {
		return 8;
	}
	if (op == op_read_addr || op == op_write_addr) {
		return 4;
	}
	if (op == op_log_or || op == op_log_and || op == op_not) {
		return 4;
	}
	if (op == op_bit_or || op == op_bit_and) {
		return 4;
	}
	if (op == op_negate) {
		return 4;
	}
	if (op == op_bit_lshift || op == op_bit_rshift) {
		return 4;
	}
	error("Unsupported IL op");
	return 0;
}

void c_emit(int code)
{
	e_write_code_int(code);
}

/* calculates total binary code length based on IL ops */
int c_calculate_code_length()
{
	int code_len = 0, i;
	for (i = 0; i < _il_idx; i++) {
		_il[i].code_offset = code_len;
		_il[i].op_len = c_get_code_length(&_il[i]);
		code_len += _il[i].op_len;
	}
	return code_len;
}

/* main code generation loop */
void c_generate()
{
	int i, data_start, code_start;

	code_start = _e_code_start; /* ELF headers size */
	data_start = c_calculate_code_length();
	c_size_functions(code_start + data_start);

	for (i = 0; i < _il_idx; i++) {
		int stack_size, j;
		variable_def *var;
		function_def *fn;
		block_def *bd;

		il_instr *ii = &_il[i];
		il_op op = ii->op;
		int pc = _e_code_idx;

		/* format IL log prefix */
		printf("%4d %3d  %#010x     ", i, op, code_start + pc);
		for (j = 0; j < _c_block_level; j++) {
			printf("  ");
		}

		if (op == op_load_data_address) {
			/* lookup address of a constant in data section as offset from PC */
			int ofs = data_start + ii->int_param1 - pc;
			int dest_reg = ii->param_no + 10;

			c_emit(r_auipc(dest_reg, r_hi(ofs)));
			c_emit(r_addi(dest_reg, dest_reg, r_lo(ofs)));

			printf("  x%d := &data[%d]", dest_reg, ii->int_param1);
		}
		if (op == op_load_numeric_constant) {
			/* load numeric constant */
			int val = ii->int_param1;
			int dest_reg = ii->param_no + 10;
			if (val > -2048 && val < 2047) {
				c_emit(r_addi(dest_reg, r_zero, r_lo(val)));
			} else {
				c_emit(r_lui(dest_reg, r_hi(val)));
				c_emit(r_addi(dest_reg, dest_reg, r_lo(val)));
			}
			printf("  x%d := %d", dest_reg, ii->int_param1);
		}
		if (op == op_get_var_addr) {
			/* lookup address of a variable */
			int dest_reg = ii->param_no + 10;
			int offset;

			var = find_global_variable(ii->string_param1);
			if (var != NULL) {
				/* need to find the variable offset in data section, from PC */
				int ofs = (data_start - pc) + var->offset;

				c_emit(r_auipc(dest_reg, r_hi(ofs)));
				offset = r_lo(ofs);
			} else {
				/* need to find the variable offset on stack, i.e. from s0 */
				var = find_local_variable(ii->string_param1, bd);
				if (var == NULL) {
					abort(); /* not found? */
				}
				offset = -var->offset;
				c_emit(r_addi(dest_reg, r_s0, 0));
			}

			c_emit(r_addi(dest_reg, dest_reg, offset));
			printf("  x%d = &%s", dest_reg, ii->string_param1);
		}
		if (op == op_read_addr) {
			/* read (dereference) memory address */
			int dest_reg = ii->param_no + 10;
			int addr_reg = ii->int_param1 + 10;

			if (ii->int_param2 == 4) {
				c_emit(r_lw(dest_reg, addr_reg, 0));
			} else if (ii->int_param2 == 1) {
				c_emit(r_lb(dest_reg, addr_reg, 0));
			} else {
				abort();
			}
			printf("  x%d = *x%d (%d)", dest_reg, addr_reg, ii->int_param2);
		}
		if (op == op_write_addr) {
			/* write at memory address */
			int dest_reg = ii->param_no + 10;
			int addr_reg = ii->int_param1 + 10;

			if (ii->int_param2 == 4) {
				c_emit(r_sw(dest_reg, addr_reg, 0));
			} else if (ii->int_param2 == 1) {
				c_emit(r_sb(dest_reg, addr_reg, 0));
			} else {
				abort();
			}
			printf("  *x%d = x%d (%d)", addr_reg, dest_reg, ii->int_param2);
		}
		if (op == op_jump) {
			/* unconditional jump to an IL-index */
			int jump_instr_index = ii->int_param1;
			il_instr *jump_instr = &_il[jump_instr_index];
			int jump_location = jump_instr->code_offset;
			int ofs = jump_location - pc;

			c_emit(r_jal(r_zero, ofs));

			printf("  -> %d", ii->int_param1);
		}
		if (op == op_return) {
			/* jump to function exit */
			function_def *fd = find_function(ii->string_param1);
			int jump_instr_index = fd->exit_point;
			il_instr *jump_instr = &_il[jump_instr_index];
			int jump_location = jump_instr->code_offset;
			int ofs = jump_location - pc;

			c_emit(r_jal(r_zero, ofs));

			printf("  return %s", ii->string_param1);
		}
		if (op == op_function_call) {
			/* function call */
			int dest_reg = ii->param_no + 10;
			int ofs;
			int jump_instr_index;
			il_instr *jump_instr;
			int jump_location;

			/* need to find offset */
			fn = find_function(ii->string_param1);

			jump_instr_index = fn->entry_point;
			jump_instr = &_il[jump_instr_index];
			jump_location = jump_instr->code_offset;
			ofs = jump_location - pc;

			c_emit(r_jal(r_ra, ofs));
			if (dest_reg != r_a0) {
				c_emit(r_addi(dest_reg, r_a0, 0));
			}

			printf("  x%d := %s() @ %d", dest_reg, ii->string_param1, fn->entry_point);
		}
		if (op == op_push) {
			int dest_reg = ii->param_no + 10;

			c_emit(r_addi(r_sp, r_sp, -16)); /* 16 aligned although we only need 4 */
			c_emit(r_sw(dest_reg, r_sp, 0));

			printf("  push x%d", dest_reg);
		}
		if (op == op_pop) {
			int dest_reg = ii->param_no + 10;

			c_emit(r_lw(dest_reg, r_sp, 0));
			c_emit(r_addi(r_sp, r_sp, 16)); /* 16 aligned although we only need 4 */

			printf("  pop x%d", dest_reg);
		}
		if (op == op_exit_point) {
			/* restore previous frame */
			c_emit(r_addi(r_sp, r_s0, 16));
			c_emit(r_lw(r_ra, r_sp, -8));
			c_emit(r_lw(r_s0, r_sp, -4));
			c_emit(r_jalr(r_zero, r_ra, 0));

			fn = NULL;
			printf("  exit %s", ii->string_param1);
		}
		if (op == op_add) {
			int dest_reg = ii->param_no + 10;
			int op_reg = ii->int_param1 + 10;

			c_emit(r_add(dest_reg, dest_reg, op_reg));

			printf("  x%d += x%d", dest_reg, op_reg);
		}
		if (op == op_sub) {
			int dest_reg = ii->param_no + 10;
			int op_reg = ii->int_param1 + 10;

			c_emit(r_sub(dest_reg, dest_reg, op_reg));

			printf("  x%d -= x%d", dest_reg, op_reg);
		}
		if (op == op_mul) {
			int dest_reg = ii->param_no + 10;
			int op_reg = ii->int_param1 + 10;

			c_emit(r_mul(dest_reg, dest_reg, op_reg));

			printf("  x%d *= x%d", dest_reg, op_reg);
		}
		if (op == op_negate) {
			int dest_reg = ii->param_no + 10;

			c_emit(r_sub(dest_reg, r_zero, dest_reg));

			printf("  -x%d", dest_reg);
		}
		if (op == op_label) {
			if (ii->string_param1 != 0) {
				/* TODO: lazy eval */
				if (strlen(ii->string_param1) > 0) {
					e_add_symbol(ii->string_param1, strlen(ii->string_param1), code_start + pc);
				}
			}
			printf(" _:");
		}
		if (op == op_equals || op == op_not_equals || op == op_less_than || op == op_less_eq_than ||
		    op == op_greater_than || op == op_greater_eq_than) {
			int dest_reg = ii->param_no + 10;
			int supp_reg = ii->int_param1 + 10;

			/* we want 1/nonzero if equ, 0 otherwise */
			if (op == op_equals)
				c_emit(r_beq(dest_reg, supp_reg, 12));
			else if (op == op_not_equals)
				c_emit(r_bne(dest_reg, supp_reg, 12));
			else if (op == op_less_than)
				c_emit(r_blt(dest_reg, supp_reg, 12));
			else if (op == op_greater_eq_than)
				c_emit(r_bge(dest_reg, supp_reg, 12));
			else if (op == op_greater_than)
				c_emit(r_blt(supp_reg, dest_reg, 12));
			else if (op == op_less_eq_than)
				c_emit(r_bge(supp_reg, dest_reg, 12));

			c_emit(r_addi(dest_reg, r_zero, 0));
			c_emit(r_jal(r_zero, 8));
			c_emit(r_addi(dest_reg, r_zero, 1));

			if (op == op_equals)
				printf("  x%d == x%d ?", dest_reg, supp_reg);
			else if (op == op_not_equals)
				printf("  x%d != x%d ?", dest_reg, supp_reg);
			else if (op == op_less_than)
				printf("  x%d < x%d ?", dest_reg, supp_reg);
			else if (op == op_greater_eq_than)
				printf("  x%d >= x%d ?", dest_reg, supp_reg);
			else if (op == op_greater_than)
				printf("  x%d > x%d ?", dest_reg, supp_reg);
			else if (op == op_less_eq_than)
				printf("  x%d <= x%d ?", dest_reg, supp_reg);
		}
		if (op == op_log_and || op == op_log_or) {
			int dest_reg = ii->param_no + 10;
			int supp_reg = ii->int_param1 + 10;

			if (op == op_log_and) {
				/* we assume both have to be 1, they can't be just nonzero */
				c_emit(r_and(dest_reg, dest_reg, supp_reg));
				printf("  x%d &&= x%d", dest_reg, supp_reg);
			} else if (op == op_log_or) {
				c_emit(r_or(dest_reg, dest_reg, supp_reg));
				printf("  x%d ||= x%d", dest_reg, supp_reg);
			}
		}
		if (op == op_bit_and || op == op_bit_or) {
			int dest_reg = ii->param_no + 10;
			int supp_reg = ii->int_param1 + 10;

			if (op == op_bit_and) {
				c_emit(r_and(dest_reg, dest_reg, supp_reg));
				printf("  x%d &= x%d", dest_reg, supp_reg);
			} else if (op == op_bit_or) {
				c_emit(r_or(dest_reg, dest_reg, supp_reg));
				printf("  x%d |= x%d", dest_reg, supp_reg);
			}
		}
		if (op == op_bit_lshift || op == op_bit_rshift) {
			int dest_reg = ii->param_no + 10;
			int supp_reg = ii->int_param1 + 10;

			if (op == op_bit_lshift) {
				c_emit(r_sll(dest_reg, dest_reg, supp_reg));
				printf("  x%d <<= x%d", dest_reg, supp_reg);
			} else if (op == op_bit_rshift) {
				c_emit(r_srl(dest_reg, dest_reg, supp_reg));
				printf("  x%d >>= x%d", dest_reg, supp_reg);
			}
		}
		if (op == op_not) {
			int dest_reg = ii->param_no + 10;
			/* 1 if zero, 0 if nonzero */
			/* only works for small range integers */
			c_emit(r_sltiu(dest_reg, dest_reg, 1));
			printf("  !x%d", dest_reg);
		}
		if (op == op_jz || op == op_jnz) {
			/* conditional jumps to IL-index */
			int jump_instr_index = ii->int_param1;
			il_instr *jump_instr = &_il[jump_instr_index];
			int jump_location = jump_instr->code_offset;
			int ofs = jump_location - pc - 4;

			if (ofs >= -4096 && ofs <= 4095) {
				/* near jump (branch) */
				if (op == op_jz) {
					c_emit(r_nop());
					c_emit(r_beq(r_a0, r_zero, ofs));
					printf("  if 0 -> %d", ii->int_param1);
				} else {
					c_emit(r_nop());
					c_emit(r_bne(r_a0, r_zero, ofs));
					printf("  if 1 -> %d", ii->int_param1);
				}
			} else {
				/* far jump */
				if (op == op_jz) {
					c_emit(r_bne(r_a0, r_zero, 8)); /* skip next instruction */
					c_emit(r_jal(r_zero, ofs));
					printf("  if 0 --> %d", ii->int_param1);
				} else {
					c_emit(r_beq(r_a0, r_zero, 8));
					c_emit(r_jal(r_zero, ofs));
					printf("  if 1 --> %d", ii->int_param1);
				}
			}
		}
		if (op == op_generic) {
			c_emit(ii->int_param1);
			printf("  asm %#010x", ii->int_param1);
		}
		if (op == op_block_start) {
			bd = &_blocks[ii->int_param1];

			if (bd->next_local > 0) {
				/* reserve stack space for locals */
				c_emit(r_addi(r_sp, r_sp, -bd->locals_size));

				stack_size += bd->locals_size;
			}
			printf("  {");
			_c_block_level++;
		}
		if (op == op_block_end) {
			bd = &_blocks[ii->int_param1]; /* should not be necessarry */

			if (bd->next_local > 0) {
				/* remove stack space for locals */
				c_emit(r_addi(r_sp, r_sp, bd->locals_size));

				stack_size -= bd->locals_size;
			}
			/* bd is current block */
			bd = bd->parent;

			printf("}");
			_c_block_level--;
		}
		if (op == op_entry_point) {
			int pn, ps;

			fn = find_function(ii->string_param1);
			ps = fn->params_size;

			/* add to symbol table */
			e_add_symbol(ii->string_param1, strlen(ii->string_param1), code_start + pc);

			/* create stack space for params and parent frame */
			c_emit(r_addi(r_sp, r_sp, -16 - ps));
			c_emit(r_sw(r_s0, r_sp, 12 + ps));
			c_emit(r_sw(r_ra, r_sp, 8 + ps));
			c_emit(r_addi(r_s0, r_sp, ps));

			stack_size = ps;

			/* push parameters on stack */
			for (pn = 0; pn < fn->num_params; pn++) {
				c_emit(r_sw(r_a0 + pn, r_s0, -fn->param_defs[pn].offset));
			}

			printf("%s:", ii->string_param1);
		}
		if (op == op_exit) {
			c_emit(r_addi(r_a0, r_zero, 0));
			c_emit(r_addi(r_a7, r_zero, 93));
			c_emit(r_ecall());
			printf("  exit");
		}
		printf("\n");
	}

	printf("Finished code generation\n");
}
