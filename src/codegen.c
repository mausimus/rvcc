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
void c_generate()
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
			_backend->op_load_numeric_constant(&state, val);
			printf("  x%d := %d", state.dest_reg, ii->int_param1);
			break;
		case op_get_var_addr:
			/* lookup address of a variable */
			var = find_global_variable(ii->string_param1);
			if (var != NULL) {
				int ofs = state.data_start + var->offset;
				_backend->op_get_global_addr(&state, ofs);
			} else {
				/* need to find the variable offset on stack, i.e. from s0 */
				var = find_local_variable(ii->string_param1, bd);
				if (var != NULL) {
					offset = -var->offset;
					_backend->op_get_local_addr(&state, offset);
				} else {
					/* is it function address? */
					fn = find_function(ii->string_param1);
					if (fn != NULL) {
						int jump_instr_index = fn->entry_point;
						il_instr *jump_instr = &_il[jump_instr_index];
						ofs = state.code_start +
						      jump_instr->code_offset; /* load code offset into variable */
						_backend->op_get_function_addr(&state, ofs);
					} else
						error("Undefined identifier");
				}
			}
			printf("  x%d = &%s", state.dest_reg, ii->string_param1);
			break;
		case op_read_addr:
			/* read (dereference) memory address */
			_backend->op_read_addr(&state, ii->int_param2);
			printf("  x%d = *x%d (%d)", state.dest_reg, state.op_reg, ii->int_param2);
			break;
		case op_write_addr:
			/* write at memory address */
			_backend->op_write_addr(&state, ii->int_param2);
			printf("  *x%d = x%d (%d)", state.op_reg, state.dest_reg, ii->int_param2);
			break;
		case op_jump: {
			/* unconditional jump to an IL-index */
			int jump_instr_index = ii->int_param1;
			il_instr *jump_instr = &_il[jump_instr_index];
			int jump_location = jump_instr->code_offset;
			ofs = jump_location - state.pc;
			_backend->op_jump(ofs);
			printf("  -> %d", ii->int_param1);
		} break;
		case op_return: {
			/* jump to function exit */
			function_def *fd = find_function(ii->string_param1);
			int jump_instr_index = fd->exit_point;
			il_instr *jump_instr = &_il[jump_instr_index];
			int jump_location = jump_instr->code_offset;
			ofs = jump_location - state.pc;
			_backend->op_return(ofs);
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

			_backend->op_function_call(&state, ofs);
			printf("  x%d := %s() @ %d", state.dest_reg, ii->string_param1, fn->entry_point);
		} break;
		case op_pointer_call: {
			/* function pointer call, address in op_reg, result in dest_reg */
			_backend->op_pointer_call(&state);
			printf("  x%d := x%d()", state.dest_reg, state.op_reg);
		} break;
		case op_push:
			_backend->op_push(&state);
			printf("  push x%d", state.dest_reg);
			break;
		case op_pop:
			_backend->op_pop(&state);
			printf("  pop x%d", state.dest_reg);
			break;
		case op_exit_point:
			/* restore previous frame */
			_backend->op_exit_point();
			fn = NULL;
			printf("  exit %s", ii->string_param1);
			break;
		case op_add:
			_backend->op_alu(&state, op_add);
			printf("  x%d += x%d", state.dest_reg, state.op_reg);
			break;
		case op_sub:
			_backend->op_alu(&state, op_sub);
			printf("  x%d -= x%d", state.dest_reg, state.op_reg);
			break;
		case op_mul:
			_backend->op_alu(&state, op_mul);
			printf("  x%d *= x%d", state.dest_reg, state.op_reg);
			break;
		case op_negate:
			_backend->op_alu(&state, op_negate);
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
			_backend->op_cmp(&state, op);
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
			_backend->op_log(&state, op);
			printf("  x%d &&= x%d", state.dest_reg, state.op_reg);
			break;
		case op_log_or:
			_backend->op_log(&state, op);
			printf("  x%d ||= x%d", state.dest_reg, state.op_reg);
			break;
		case op_bit_and:
			_backend->op_bit(&state, op);
			printf("  x%d &= x%d", state.dest_reg, state.op_reg);
			break;
		case op_bit_or:
			_backend->op_bit(&state, op);
			printf("  x%d |= x%d", state.dest_reg, state.op_reg);
			break;
		case op_bit_lshift:
			_backend->op_bit(&state, op);
			printf("  x%d <<= x%d", state.dest_reg, state.op_reg);
			break;
		case op_bit_rshift:
			_backend->op_bit(&state, op);
			printf("  x%d >>= x%d", state.dest_reg, state.op_reg);
			break;
		case op_not:
			/* 1 if zero, 0 if nonzero */
			_backend->op_bit(&state, op);
			printf("  !x%d", state.dest_reg);
			break;
		case op_jz:
		case op_jnz: {
			/* conditional jumps to IL-index */
			int jump_instr_index = ii->int_param1;
			il_instr *jump_instr = &_il[jump_instr_index];
			int jump_location = jump_instr->code_offset;
			int ofs = jump_location - state.pc - 4;
			_backend->op_jz(&state, op, ofs);
			if (op == op_jz)
				printf("  if 0 -> %d", ii->int_param1);
			else
				printf("  if 1 -> %d", ii->int_param1);
		} break;
		case op_generic:
			c_emit(ii->int_param1);
			printf("  asm %#010x", ii->int_param1);
			break;
		case op_block_start:
			bd = &_blocks[ii->int_param1];
			if (bd->next_local > 0) {
				/* reserve stack space for locals */
				_backend->op_block(-bd->locals_size);
				stack_size += bd->locals_size;
			}
			printf("  {");
			_c_block_level++;
			break;
		case op_block_end:
			bd = &_blocks[ii->int_param1]; /* should not be necessarry */
			if (bd->next_local > 0) {
				/* remove stack space for locals */
				_backend->op_block(bd->locals_size);
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
			_backend->op_entry_point(ps);
			stack_size = ps;

			/* push parameters on stack */
			for (pn = 0; pn < fn->num_params; pn++) {
				_backend->op_store_param(pn, -fn->param_defs[pn].offset);
			}
			printf("%s:", ii->string_param1);
		} break;
		case op_start:
			_backend->op_start();
			printf("  start");
			break;
		case op_syscall:
			_backend->op_syscall();
			printf("  syscall");
			break;
		case op_exit:
			_backend->op_exit();
			printf("  exit");
			break;
		default:
			error("Unsupported IL op");
		}
		printf("\n");
	}

	printf("Finished code generation\n");
}
