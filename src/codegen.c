/* rvcc C compiler - IL->binary code generator */

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
	if ((s & 15) > 0)
		s = (s - (s & 15)) + 16;
	if (s > 2047)
		error("Local stack size exceeded");

	fn->params_size = s;
}

/* returns stack size required after block local variables */
int c_size_block(block_def *bd)
{
	int size = 0, i, offset;

	/* our offset starts from parent's offset */
	if (bd->parent == NULL)
		if (bd->function != NULL)
			offset = bd->function->params_size;
		else
			offset = 0;
	else
		offset = c_size_block(bd->parent);

	/* declared locals */
	for (i = 0; i < bd->next_local; i++) {
		int vs = size_variable(&bd->locals[i]);
		bd->locals[i].offset = size + offset + vs; /* for looking up value off stack */
		size += vs;
	}

	/* align to 16 bytes */
	if ((size & 15) > 0)
		size = (size - (size & 15)) + 16;
	if (size > 2047)
		error("Local stack size exceeded");

	bd->locals_size = size; /* save in block for stack allocation */
	return size + offset;
}

/* calculate stack necessary sizes for all functions */
void c_size_functions(int data_start)
{
	block_def *bd;
	int i;

	/* size functions */
	for (i = 0; i < _functions_idx; i++)
		c_size_function(&_functions[i]);

	/* size blocks excl. global block */
	for (i = 1; i < _blocks_idx; i++)
		c_size_block(&_blocks[i]);

	/* allocate data for globals, in block 0 */
	bd = &_blocks[0];
	for (i = 0; i < bd->next_local; i++) {
		bd->locals[i].offset = _e_data_idx; /* set offset in data section */
		e_add_symbol(bd->locals[i].variable_name, strlen(bd->locals[i].variable_name),
			     data_start + _e_data_idx);
		_e_data_idx += size_variable(&bd->locals[i]);
	}
}

/* calculates total binary code length based on IL ops */
int c_calculate_code_length()
{
	int code_len = 0, i;
	for (i = 0; i < _il_idx; i++) {
		_il[i].code_offset = code_len;
		_il[i].op_len = _backend->c_get_code_length(&_il[i]);
		code_len += _il[i].op_len;
	}
	return code_len;
}

/* main code generation loop */
void c_generate(arch_t arch)
{
	int i;
	int stack_size = 0;
	block_def *bd = NULL;

	backend_state state;
	state.code_start = _e_code_start; /* ELF headers size */
	state.data_start = c_calculate_code_length();
	c_size_functions(state.code_start + state.data_start);

	for (i = 0; i < _il_idx; i++) {
		int j;
		int offset, ofs, val;
		variable_def *var;
		function_def *fn;

		il_instr *ii = &_il[i];
		il_op op = ii->op;
		state.pc = _e_code_idx;
		state.dest_reg = _backend->c_dest_reg(ii->param_no);
		state.op_reg = _backend->c_dest_reg(ii->int_param1);

		/* format IL log prefix */
		printf("%4d %3d  %#010x     ", i, op, state.code_start + state.pc);
		for (j = 0; j < _c_block_level; j++)
			printf("  ");

		switch (op) {
		case op_load_data_address:
			/* lookup address of a constant in data section */
			ofs = state.data_start + ii->int_param1;
			_backend->op_load_data_address(&state, ofs);
			printf("  x%d := &data[%d]", state.dest_reg, ii->int_param1);
			break;
		case op_load_numeric_constant:
			/* load numeric constant */
			val = ii->int_param1;
			/*_backend->op_load_numeric_constant(&state, val);*/
			switch (arch) {
			case a_arm:
				if (val >= 0 && val < 256) {
					c_emit(a_mov_i(ac_al, state.dest_reg, val));
				} else {
					c_emit(a_movw(ac_al, state.dest_reg, val));
					c_emit(a_movt(ac_al, state.dest_reg, val));
				}
				break;
			case a_riscv:
				if (val > -2048 && val < 2047) {
					c_emit(r_addi(state.dest_reg, r_zero, r_lo(val)));
				} else {
					c_emit(r_lui(state.dest_reg, r_hi(val)));
					c_emit(r_addi(state.dest_reg, state.dest_reg, r_lo(val)));
				}
				break;
			}
			printf("  x%d := %d", state.dest_reg, ii->int_param1);
			break;
		case op_get_var_addr:
			/* lookup address of a variable */
			var = find_global_variable(ii->string_param1);
			if (var != NULL) {
				int ofs = state.data_start + var->offset;
				switch (arch) {
				case a_arm:
					/* need to find the variable offset in data section, absolute */
					ofs += state.code_start;

					c_emit(a_movw(ac_al, state.dest_reg, ofs));
					c_emit(a_movt(ac_al, state.dest_reg, ofs));
					break;
				case a_riscv:
					/* need to find the variable offset in data section, from PC */
					ofs -= state.pc;

					c_emit(r_auipc(state.dest_reg, r_hi(ofs)));
					offset = r_lo(ofs);
					c_emit(r_addi(state.dest_reg, state.dest_reg, offset));
					break;
				}
			} else {
				/* need to find the variable offset on stack, i.e. from s0 */
				var = find_local_variable(ii->string_param1, bd);
				if (var != NULL) {
					offset = -var->offset;
					switch (arch) {
					case a_arm:
						c_emit(a_add_i(ac_al, state.dest_reg, a_s0, offset & 255));
						c_emit(a_add_i(ac_al, state.dest_reg, state.dest_reg,
							       offset - (offset & 255)));
						break;
					case a_riscv:
						c_emit(r_addi(state.dest_reg, r_s0, 0));
						c_emit(r_addi(state.dest_reg, state.dest_reg, offset));
						break;
					}
				} else {
					/* is it function address? */
					fn = find_function(ii->string_param1);
					if (fn != NULL) {
						int jump_instr_index = fn->entry_point;
						il_instr *jump_instr = &_il[jump_instr_index];
						ofs = state.code_start +
						      jump_instr->code_offset; /* load code offset into variable */

						switch (arch) {
						case a_arm:
							c_emit(a_movw(ac_al, state.dest_reg, ofs));
							c_emit(a_movt(ac_al, state.dest_reg, ofs));
							break;
						case a_riscv:
							c_emit(r_lui(state.dest_reg, r_hi(ofs)));
							offset = r_lo(ofs);
							c_emit(r_addi(state.dest_reg, state.dest_reg, offset));
							break;
						}
					} else
						error("Undefined identifier");
				}
			}
			printf("  x%d = &%s", state.dest_reg, ii->string_param1);
			break;
		case op_read_addr:
			/* read (dereference) memory address */
			switch (ii->int_param2) {
			case 4:
				switch (arch) {
				case a_arm:
					c_emit(a_lw(ac_al, state.dest_reg, state.op_reg, 0));
					break;
				case a_riscv:
					c_emit(r_lw(state.dest_reg, state.op_reg, 0));
					break;
				}
				break;
			case 1:
				switch (arch) {
				case a_arm:
					c_emit(a_lb(ac_al, state.dest_reg, state.op_reg, 0));
					break;
				case a_riscv:
					c_emit(r_lb(state.dest_reg, state.op_reg, 0));
					break;
				}
				break;
			default:
				error("Unsupported word size");
			}
			printf("  x%d = *x%d (%d)", state.dest_reg, state.op_reg, ii->int_param2);
			break;
		case op_write_addr:
			/* write at memory address */
			switch (ii->int_param2) {
			case 4:
				switch (arch) {
				case a_arm:
					c_emit(a_sw(ac_al, state.dest_reg, state.op_reg, 0));
					break;
				case a_riscv:
					c_emit(r_sw(state.dest_reg, state.op_reg, 0));
					break;
				}
				break;
			case 1:
				switch (arch) {
				case a_arm:
					c_emit(a_sb(ac_al, state.dest_reg, state.op_reg, 0));
					break;
				case a_riscv:
					c_emit(r_sb(state.dest_reg, state.op_reg, 0));
					break;
				}
				break;
			default:
				error("Unsupported word size");
			}
			printf("  *x%d = x%d (%d)", state.op_reg, state.dest_reg, ii->int_param2);
			break;
		case op_jump: {
			/* unconditional jump to an IL-index */
			int jump_instr_index = ii->int_param1;
			il_instr *jump_instr = &_il[jump_instr_index];
			int jump_location = jump_instr->code_offset;
			ofs = jump_location - state.pc;

			switch (arch) {
			case a_arm:
				c_emit(a_b(ac_al, ofs));
				break;
			case a_riscv:
				c_emit(r_jal(r_zero, ofs));
				break;
			}
			printf("  -> %d", ii->int_param1);
		} break;
		case op_return: {
			/* jump to function exit */
			function_def *fd = find_function(ii->string_param1);
			int jump_instr_index = fd->exit_point;
			il_instr *jump_instr = &_il[jump_instr_index];
			int jump_location = jump_instr->code_offset;
			ofs = jump_location - state.pc;

			switch (arch) {
			case a_arm:
				c_emit(a_b(ac_al, ofs));
				break;
			case a_riscv:
				c_emit(r_jal(r_zero, ofs));
				break;
			}
			printf("  return %s", ii->string_param1);
		} break;
		case op_function_call: {
			/* function call */
			int jump_instr_index;
			il_instr *jump_instr;
			int jump_location;

			/* need to find offset */
			fn = find_function(ii->string_param1);
			jump_instr_index = fn->entry_point;
			jump_instr = &_il[jump_instr_index];
			jump_location = jump_instr->code_offset;
			ofs = jump_location - state.pc;

			switch (arch) {
			case a_arm:
				c_emit(a_bl(ac_al, ofs));
				if (state.dest_reg != a_r0)
					c_emit(a_mov_r(ac_al, state.dest_reg, a_r0));
				break;
			case a_riscv:
				c_emit(r_jal(r_ra, ofs));
				if (state.dest_reg != r_a0)
					c_emit(r_addi(state.dest_reg, r_a0, 0));
				break;
			}
			printf("  x%d := %s() @ %d", state.dest_reg, ii->string_param1, fn->entry_point);
		} break;
		case op_pointer_call: {
			/* function pointer call, address in op_reg, result in dest_reg */
			switch (arch) {
			case a_arm:
				c_emit(a_blx(ac_al, state.op_reg));
				if (state.dest_reg != a_r0)
					c_emit(a_mov_r(ac_al, state.dest_reg, a_r0));
				break;
			case a_riscv:
				c_emit(r_jalr(r_ra, state.op_reg, 0));
				if (state.dest_reg != r_a0)
					c_emit(r_addi(state.dest_reg, r_a0, 0));
				break;
			}
			printf("  x%d := x%d()", state.dest_reg, state.op_reg);
		} break;
		case op_push:
			switch (arch) {
			case a_arm:
				c_emit(a_add_i(ac_al, a_sp, a_sp, -16)); /* 16 aligned although we only need 4 */
				c_emit(a_sw(ac_al, state.dest_reg, a_sp, 0));
				break;
			case a_riscv:
				c_emit(r_addi(r_sp, r_sp, -16)); /* 16 aligned although we only need 4 */
				c_emit(r_sw(state.dest_reg, r_sp, 0));
				break;
			}
			printf("  push x%d", state.dest_reg);
			break;
		case op_pop:
			switch (arch) {
			case a_arm:
				c_emit(a_lw(ac_al, state.dest_reg, a_sp, 0));
				c_emit(a_add_i(ac_al, a_sp, a_sp, 16)); /* 16 aligned although we only need 4 */
				break;
			case a_riscv:
				c_emit(r_lw(state.dest_reg, r_sp, 0));
				c_emit(r_addi(r_sp, r_sp, 16)); /* 16 aligned although we only need 4 */
				break;
			}
			printf("  pop x%d", state.dest_reg);
			break;
		case op_exit_point:
			/* restore previous frame */
			switch (arch) {
			case a_arm:
				c_emit(a_add_i(ac_al, a_sp, a_s0, 16));
				c_emit(a_lw(ac_al, a_lr, a_sp, -8));
				c_emit(a_lw(ac_al, a_s0, a_sp, -4));
				c_emit(a_mov_r(ac_al, a_pc, a_lr));
				break;
			case a_riscv:
				c_emit(r_addi(r_sp, r_s0, 16));
				c_emit(r_lw(r_ra, r_sp, -8));
				c_emit(r_lw(r_s0, r_sp, -4));
				c_emit(r_jalr(r_zero, r_ra, 0));
				break;
			}
			fn = NULL;
			printf("  exit %s", ii->string_param1);
			break;
		case op_add:
			switch (arch) {
			case a_arm:
				c_emit(a_add_r(ac_al, state.dest_reg, state.dest_reg, state.op_reg));
				break;
			case a_riscv:
				c_emit(r_add(state.dest_reg, state.dest_reg, state.op_reg));
				break;
			}

			printf("  x%d += x%d", state.dest_reg, state.op_reg);
			break;
		case op_sub:
			switch (arch) {
			case a_arm:
				c_emit(a_sub_r(ac_al, state.dest_reg, state.dest_reg, state.op_reg));
				break;
			case a_riscv:
				c_emit(r_sub(state.dest_reg, state.dest_reg, state.op_reg));
				break;
			}
			printf("  x%d -= x%d", state.dest_reg, state.op_reg);
			break;
		case op_mul:
			switch (arch) {
			case a_arm:
				c_emit(a_mul(ac_al, state.dest_reg, state.dest_reg, state.op_reg));
				break;
			case a_riscv:
				c_emit(r_mul(state.dest_reg, state.dest_reg, state.op_reg));
				break;
			}
			printf("  x%d *= x%d", state.dest_reg, state.op_reg);
			break;
		case op_negate:
			switch (arch) {
			case a_arm:
				c_emit(a_rsb_i(ac_al, state.dest_reg, 0, state.dest_reg));
				break;
			case a_riscv:
				c_emit(r_sub(state.dest_reg, r_zero, state.dest_reg));
				break;
			}
			printf("  -x%d", state.dest_reg);
			break;
		case op_label:
			if (ii->string_param1 != NULL)
				/* TODO: lazy eval */
				if (strlen(ii->string_param1) > 0)
					e_add_symbol(ii->string_param1, strlen(ii->string_param1),
						     state.code_start + state.pc);
			printf(" _:");
			break;
		case op_equals:
		case op_not_equals:
		case op_less_than:
		case op_less_eq_than:
		case op_greater_than:
		case op_greater_eq_than:
			/* we want 1/nonzero if equ, 0 otherwise */
			switch (arch) {
			case a_arm: {
				ar_cond cond = a_get_cond(op);
				c_emit(a_cmp_r(ac_al, state.dest_reg, state.dest_reg, state.op_reg));
				c_emit(a_zero(state.dest_reg));
				c_emit(a_mov_i(cond, state.dest_reg, 1));
			} break;
			case a_riscv:
				switch (op) {
				case op_equals:
					c_emit(r_beq(state.dest_reg, state.op_reg, 12));
					break;
				case op_not_equals:
					c_emit(r_bne(state.dest_reg, state.op_reg, 12));
					break;
				case op_less_than:
					c_emit(r_blt(state.dest_reg, state.op_reg, 12));
					break;
				case op_greater_eq_than:
					c_emit(r_bge(state.dest_reg, state.op_reg, 12));
					break;
				case op_greater_than:
					c_emit(r_blt(state.op_reg, state.dest_reg, 12));
					break;
				case op_less_eq_than:
					c_emit(r_bge(state.op_reg, state.dest_reg, 12));
					break;
				default:
					error("Unsupported conditional IL op");
					break;
				}
				c_emit(r_addi(state.dest_reg, r_zero, 0));
				c_emit(r_jal(r_zero, 8));
				c_emit(r_addi(state.dest_reg, r_zero, 1));
				break;
			}

			switch (op) {
			case op_equals:
				printf("  x%d == x%d ?", state.dest_reg, state.op_reg);
				break;
			case op_not_equals:
				printf("  x%d != x%d ?", state.dest_reg, state.op_reg);
				break;
			case op_less_than:
				printf("  x%d < x%d ?", state.dest_reg, state.op_reg);
				break;
			case op_greater_eq_than:
				printf("  x%d >= x%d ?", state.dest_reg, state.op_reg);
				break;
			case op_greater_than:
				printf("  x%d > x%d ?", state.dest_reg, state.op_reg);
				break;
			case op_less_eq_than:
				printf("  x%d <= x%d ?", state.dest_reg, state.op_reg);
				break;
			default:
				break;
			}
			break;
		case op_log_and:
			/* we assume both have to be 1, they can't be just nonzero */
			switch (arch) {
			case a_arm:
				c_emit(a_and_r(ac_al, state.dest_reg, state.dest_reg, state.op_reg));
				break;
			case a_riscv:
				c_emit(r_and(state.dest_reg, state.dest_reg, state.op_reg));
				break;
			}
			printf("  x%d &&= x%d", state.dest_reg, state.op_reg);
			break;
		case op_log_or:
			switch (arch) {
			case a_arm:
				c_emit(a_or_r(ac_al, state.dest_reg, state.dest_reg, state.op_reg));
				break;
			case a_riscv:
				c_emit(r_or(state.dest_reg, state.dest_reg, state.op_reg));
				break;
			}
			printf("  x%d ||= x%d", state.dest_reg, state.op_reg);
			break;
		case op_bit_and:
			switch (arch) {
			case a_arm:
				c_emit(a_and_r(ac_al, state.dest_reg, state.dest_reg, state.op_reg));
				break;
			case a_riscv:
				c_emit(r_and(state.dest_reg, state.dest_reg, state.op_reg));
				break;
			}
			printf("  x%d &= x%d", state.dest_reg, state.op_reg);
			break;
		case op_bit_or:
			switch (arch) {
			case a_arm:
				c_emit(a_or_r(ac_al, state.dest_reg, state.dest_reg, state.op_reg));
				break;
			case a_riscv:
				c_emit(r_or(state.dest_reg, state.dest_reg, state.op_reg));
				break;
			}
			printf("  x%d |= x%d", state.dest_reg, state.op_reg);
			break;
		case op_bit_lshift:
			switch (arch) {
			case a_arm:
				c_emit(a_sll(ac_al, state.dest_reg, state.dest_reg, state.op_reg));
				break;
			case a_riscv:
				c_emit(r_sll(state.dest_reg, state.dest_reg, state.op_reg));
				break;
			}
			printf("  x%d <<= x%d", state.dest_reg, state.op_reg);
			break;
		case op_bit_rshift:
			switch (arch) {
			case a_arm:
				c_emit(a_srl(ac_al, state.dest_reg, state.dest_reg, state.op_reg));
				break;
			case a_riscv:
				c_emit(r_srl(state.dest_reg, state.dest_reg, state.op_reg));
				break;
			}

			printf("  x%d >>= x%d", state.dest_reg, state.op_reg);
			break;
		case op_not:
			/* 1 if zero, 0 if nonzero */
			switch (arch) {
			case a_arm:
				/* only works for 1/0 */
				c_emit(a_rsb_i(ac_al, state.dest_reg, 1, state.dest_reg));
				break;
			case a_riscv:
				/* only works for small range integers */
				c_emit(r_sltiu(state.dest_reg, state.dest_reg, 1));
				break;
			}
			printf("  !x%d", state.dest_reg);
			break;
		case op_jz:
		case op_jnz: {
			/* conditional jumps to IL-index */
			int jump_instr_index = ii->int_param1;
			il_instr *jump_instr = &_il[jump_instr_index];
			int jump_location = jump_instr->code_offset;
			int ofs = jump_location - state.pc - 4;

			switch (arch) {
			case a_arm:
				c_emit(a_teq(state.dest_reg));
				if (op == op_jz) {
					c_emit(a_b(ac_eq, ofs));
					printf("  if 0 -> %d", ii->int_param1);
				} else {
					c_emit(a_b(ac_ne, ofs));
					printf("  if 1 -> %d", ii->int_param1);
				}
				break;
			case a_riscv:
				if (ofs >= -4096 && ofs <= 4095) {
					/* near jump (branch) */
					if (op == op_jz) {
						c_emit(r_nop());
						c_emit(r_beq(state.dest_reg, r_zero, ofs));
						printf("  if 0 -> %d", ii->int_param1);
					} else if (op == op_jnz) {
						c_emit(r_nop());
						c_emit(r_bne(state.dest_reg, r_zero, ofs));
						printf("  if 1 -> %d", ii->int_param1);
					}
				} else {
					/* far jump */
					if (op == op_jz) {
						c_emit(r_bne(state.dest_reg, r_zero, 8)); /* skip next instruction */
						c_emit(r_jal(r_zero, ofs));
						printf("  if 0 --> %d", ii->int_param1);
					} else if (op == op_jnz) {
						c_emit(r_beq(state.dest_reg, r_zero, 8));
						c_emit(r_jal(r_zero, ofs));
						printf("  if 1 --> %d", ii->int_param1);
					}
				}
				break;
			}
		} break;
		case op_generic:
			c_emit(ii->int_param1);
			printf("  asm %#010x", ii->int_param1);
			break;
		case op_block_start:
			bd = &_blocks[ii->int_param1];
			if (bd->next_local > 0) {
				/* reserve stack space for locals */
				switch (arch) {
				case a_arm:
					c_emit(a_add_i(ac_al, a_sp, a_sp, -bd->locals_size));
					break;
				case a_riscv:
					c_emit(r_addi(r_sp, r_sp, -bd->locals_size));
					break;
				}
				stack_size += bd->locals_size;
			}
			printf("  {");
			_c_block_level++;
			break;
		case op_block_end:
			bd = &_blocks[ii->int_param1]; /* should not be necessarry */
			if (bd->next_local > 0) {
				/* remove stack space for locals */
				switch (arch) {
				case a_arm:
					c_emit(a_add_i(ac_al, a_sp, a_sp, bd->locals_size));
					break;
				case a_riscv:
					c_emit(r_addi(r_sp, r_sp, bd->locals_size));
					break;
				}
				stack_size -= bd->locals_size;
			}
			/* bd is current block */
			bd = bd->parent;
			printf("}");
			_c_block_level--;
			break;
		case op_entry_point: {
			int pn, ps;
			fn = find_function(ii->string_param1);
			ps = fn->params_size;

			/* add to symbol table */
			e_add_symbol(ii->string_param1, strlen(ii->string_param1), state.code_start + state.pc);

			/* create stack space for params and parent frame */
			switch (arch) {
			case a_arm:
				c_emit(a_add_i(ac_al, a_sp, a_sp, -16 - ps));
				c_emit(a_sw(ac_al, a_s0, a_sp, 12 + ps));
				c_emit(a_sw(ac_al, a_lr, a_sp, 8 + ps));
				c_emit(a_add_i(ac_al, a_s0, a_sp, ps));
				break;
			case a_riscv:
				c_emit(r_addi(r_sp, r_sp, -16 - ps));
				c_emit(r_sw(r_s0, r_sp, 12 + ps));
				c_emit(r_sw(r_ra, r_sp, 8 + ps));
				c_emit(r_addi(r_s0, r_sp, ps));
				break;
			}
			stack_size = ps;

			/* push parameters on stack */
			for (pn = 0; pn < fn->num_params; pn++) {
				switch (arch) {
				case a_arm:
					c_emit(a_sw(ac_al, a_r0 + pn, a_s0, -fn->param_defs[pn].offset));
					break;
				case a_riscv:
					c_emit(r_sw(r_a0 + pn, r_s0, -fn->param_defs[pn].offset));
					break;
				}
			}
			printf("%s:", ii->string_param1);
		} break;
		case op_start:
			switch (arch) {
			case a_arm:
				c_emit(a_lw(ac_al, a_r0, a_sp, 0)); /* argc */
				c_emit(a_add_i(ac_al, a_r1, a_sp, 4)); /* argv */
				break;
			case a_riscv:
				c_emit(r_lw(r_a0, r_sp, 0)); /* argc */
				c_emit(r_addi(r_a1, r_sp, 4)); /* argv */
				break;
			}
			printf("  start");
			break;
		case op_syscall:
			switch (arch) {
			case a_arm:
				c_emit(a_mov_r(ac_al, a_r7, a_r0));
				c_emit(a_mov_r(ac_al, a_r0, a_r1));
				c_emit(a_mov_r(ac_al, a_r1, a_r2));
				c_emit(a_mov_r(ac_al, a_r2, a_r3));
				c_emit(a_swi());
				break;
			case a_riscv:
				c_emit(r_addi(r_a7, r_a0, 0));
				c_emit(r_addi(r_a0, r_a1, 0));
				c_emit(r_addi(r_a1, r_a2, 0));
				c_emit(r_addi(r_a2, r_a3, 0));
				c_emit(r_ecall());
				break;
			}
			printf("  syscall");
			break;
		case op_exit:
			switch (arch) {
			case a_arm:
				c_emit(a_mov_i(ac_al, a_r0, 0));
				c_emit(a_mov_i(ac_al, a_r7, 1));
				c_emit(a_swi());
				break;
			case a_riscv:
				c_emit(r_addi(r_a0, r_zero, 0));
				c_emit(r_addi(r_a7, r_zero, 93));
				c_emit(r_ecall());
				break;
			}
			printf("  exit");
			break;
		default:
			error("Unsupported IL op");
		}
		printf("\n");
	}

	printf("Finished code generation\n");
}
