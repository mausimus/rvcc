/* rvcc C compiler - source->IL parser */

#include <stdio.h>
#include <stdlib.h>

void p_read_function_call(function_def *fn, int param_no, block_def *parent);
void p_read_lvalue(lvalue_def *lvalue, variable_def *var, block_def *parent, int param_no, int evaluate);
void p_read_expression(int param_no, block_def *parent);
void p_read_code_block(function_def *function, block_def *parent);

int p_write_symbol(char *data, int len)
{
	int startLen = _e_data_idx;
	e_write_data_string(data, len);
	return startLen;
}

int p_get_size(variable_def *var, type_def *type)
{
	if (var->is_pointer) {
		return PTR_SIZE;
	}
	return type->size;
}

void p_initialize()
{
	il_instr *ii;
	type_def *type;

	type = add_type();
	strcpy(type->type_name, "void");
	type->base_type = bt_void;
	type->size = 0;

	type = add_type();
	strcpy(type->type_name, "char");
	type->base_type = bt_char;
	type->size = 1;

	type = add_type();
	strcpy(type->type_name, "int");
	type->base_type = bt_int;
	type->size = 4;

	add_block(NULL, NULL); /* global block */
	e_add_symbol("", 0, 0); /* undef symbol */

	/* main() parameters */
	add_generic(r_lw(r_a0, r_sp, 0)); /* argc */
	add_generic(r_addi(r_a1, r_sp, 4)); /* argv */

	/* binary entry point: call main, then exit */
	ii = add_instr(op_function_call);
	ii->string_param1 = "main";
	ii = add_instr(op_label);
	ii->string_param1 = "_exit";
	add_instr(op_exit);
}

int p_read_numeric_constant(char buffer[])
{
	int i = 0;
	int value = 0;
	while (buffer[i] != 0) {
		if (i == 1 && (buffer[i] == 'x' || buffer[i] == 'X')) {
			value = 0;
			i = 2;
			while (buffer[i] != 0) {
				char c = buffer[i];
				value = value << 4;
				if (c >= '0' && c <= '9') {
					value += c - '0';
				} else if (c >= 'a' && c <= 'f') {
					value += c - 'a' + 10;
				} else if (c >= 'A' && c <= 'F') {
					value += c - 'A' + 10;
				}
				i++;
			}
			return value;
		}
		value = value * 10 + buffer[i] - '0';
		i++;
	}
	return value;
}

void p_read_inner_variable_declaration(variable_def *vd)
{
	if (l_accept(t_star)) {
		vd->is_pointer = 1;
	} else {
		vd->is_pointer = 0;
	}
	l_ident(t_identifier, vd->variable_name);
	if (l_accept(t_op_square)) {
		char buffer[10];

		/* array with size*/
		if (l_peek(t_numeric, buffer)) {
			vd->array_size = p_read_numeric_constant(buffer);
			l_expect(t_numeric);
		} else {
			/* array without size - just a pointer although could be nested */
			vd->is_pointer++;
		}
		l_expect(t_cl_square);
	} else {
		vd->array_size = 0;
	}
}

/* we are starting it _l_next_token, need to check the type */
void p_read_full_variable_declaration(variable_def *vd)
{
	l_accept(t_struct); /* ignore struct def */
	l_ident(t_identifier, vd->type_name);
	p_read_inner_variable_declaration(vd);
}

/* we are starting it _l_next_token, need to check the type */
void p_read_partial_variable_declaration(variable_def *vd, variable_def *template)
{
	strcpy(vd->type_name, template->type_name);
	p_read_inner_variable_declaration(vd);
}

int p_read_parameter_list_declaration(variable_def vds[])
{
	int vn = 0;

	l_expect(t_op_bracket);
	while (l_peek(t_identifier, NULL) == 1) {
		p_read_full_variable_declaration(&vds[vn]);
		vn++;
		l_accept(t_comma);
	}
	if (l_accept(t_elipsis)) {
		/* variadic function - max 8 params total, create dummy parameters to put all on stack  */
		for (; vn < MAX_PARAMS; vn++) {
			strcpy(vds[vn].type_name, "int");
			strcpy(vds[vn].variable_name, "var_arg");
			vds[vn].is_pointer = 1;
		}
	}
	l_expect(t_cl_bracket);

	return vn;
}

void p_read_literal_param(int param_no)
{
	char literal[MAX_TOKEN_LEN];
	il_instr *ii;
	int didx;

	l_ident(t_string, literal);

	didx = p_write_symbol(literal, strlen(literal) + 1);
	ii = add_instr(op_load_data_address);
	ii->param_no = param_no;
	ii->int_param1 = didx;
}

void p_read_numeric_param(int param_no, int isneg)
{
	char token[MAX_ID_LEN];
	int value = 0;
	int i = 0;
	il_instr *ii;
	char c;

	l_ident(t_numeric, token);

	if (token[0] == '-') {
		isneg = 1 - isneg;
		i++;
	}
	if (token[0] == '0' && (token[1] == 'x' || token[0] == 'X')) {
		i = 2;
		do {
			c = token[i];
			if (c >= '0' && c <= '9') {
				c -= '0';
			} else if (c >= 'a' && c <= 'f') {
				c -= 'a';
				c += 10;
			} else if (c >= 'A' && c <= 'F') {
				c -= 'A';
				c += 10;
			} else {
				error("Invalid numeric constant");
			}

			value = (value * 16) + c;
			i++;
		} while (is_hex(token[i]));
	} else {
		do {
			c = token[i] - '0';
			value = (value * 10) + c;
			i++;
		} while (is_digit(token[i]));
	}

	ii = add_instr(op_load_numeric_constant);
	ii->param_no = param_no;
	if (isneg) {
		value = -value;
	}
	ii->int_param1 = value;
}

void p_read_char_param(int param_no)
{
	il_instr *ii;
	char token[5];

	l_ident(t_char, token);

	ii = add_instr(op_load_numeric_constant);
	ii->param_no = param_no;
	ii->int_param1 = token[0];
}

/* maintain a stack of expression values and operators,
 depending on next operators's priority either apply it or operator on stack first */
void p_read_expression_operand(int param_no, block_def *parent)
{
	int isneg = 0;
	if (l_accept(t_minus)) {
		isneg = 1;
		if (l_peek(t_numeric, NULL) == 0 && l_peek(t_identifier, NULL) == 0 &&
		    l_peek(t_op_bracket, NULL) == 0) {
			error("Unexpected token after unary minus");
		}
	}

	if (l_peek(t_string, NULL)) {
		p_read_literal_param(param_no);
	} else if (l_peek(t_char, NULL)) {
		p_read_char_param(param_no);
	} else if (l_peek(t_numeric, NULL)) {
		p_read_numeric_param(param_no, isneg);
	} else if (l_accept(t_log_not)) {
		il_instr *ii;
		p_read_expression(param_no, parent);
		ii = add_instr(op_not);
		ii->param_no = param_no;
	} else if (l_accept(t_ampersand)) {
		char token[MAX_VAR_LEN];
		variable_def *var;
		lvalue_def lvalue;

		l_peek(t_identifier, token);
		var = find_variable(token, parent);
		p_read_lvalue(&lvalue, var, parent, param_no, 0);
	} else if (l_peek(t_star, NULL)) {
		/* dereference */
		char token[MAX_VAR_LEN];
		variable_def *var;
		lvalue_def lvalue;
		il_instr *ii;

		l_accept(t_op_bracket);
		l_peek(t_identifier, token);
		var = find_variable(token, parent);
		p_read_lvalue(&lvalue, var, parent, param_no, 1);
		l_accept(t_cl_bracket);
		ii = add_instr(op_read_addr);
		ii->param_no = param_no;
		ii->int_param1 = param_no;
		ii->int_param2 = lvalue.size;
	} else if (l_accept(t_op_bracket)) {
		p_read_expression(param_no, parent);
		l_expect(t_cl_bracket);

		if (isneg) {
			il_instr *ii = add_instr(op_negate);
			ii->param_no = param_no;
		}
	} else if (l_accept(t_sizeof)) {
		char token[MAX_TYPE_LEN];
		type_def *type;
		il_instr *ii = add_instr(op_load_numeric_constant);

		l_expect(t_op_bracket);
		l_ident(t_identifier, token);
		type = find_type(token);
		if (type == NULL) {
			error("Unable to find type");
		}

		ii->param_no = param_no;
		ii->int_param1 = type->size;
		l_expect(t_cl_bracket);
	} else {
		/* function call, constant or variable - read token and determine */
		char token[MAX_ID_LEN];
		function_def *fn;
		variable_def *var;
		constant_def *con;

		l_peek(t_identifier, token);

		/* is it a constant or variable? */
		con = find_constant(token);
		var = find_variable(token, parent);
		fn = find_function(token);

		if (con != NULL) {
			int value = con->value;
			il_instr *ii = add_instr(op_load_numeric_constant);
			ii->param_no = param_no;
			ii->int_param1 = value;
			l_expect(t_identifier);
		} else if (var != NULL) {
			/* evalue lvalue expression */
			lvalue_def lvalue;
			p_read_lvalue(&lvalue, var, parent, param_no, 1);
		} else if (fn != NULL) {
			il_instr *ii;
			int pn;

			for (pn = 0; pn < param_no; pn++) {
				ii = add_instr(op_push);
				ii->param_no = pn;
			}

			/* we should push existing parameters onto the stack since function calls use same? */
			p_read_function_call(fn, param_no, parent);

			for (pn = param_no - 1; pn >= 0; pn--) {
				ii = add_instr(op_pop);
				ii->param_no = pn;
			}
		} else {
			printf("%s\n", token);
			error("Unrecognized expression token"); /* unknown expression */
		}

		if (isneg) {
			il_instr *ii = add_instr(op_negate);
			ii->param_no = param_no;
		}
	}
}

int p_get_operator_priority(il_op op)
{
	if (op == op_log_and || op == op_log_or) {
		return -2; /* apply last, lowest priority */
	}
	if (op == op_equals || op == op_not_equals || op == op_less_than || op == op_less_eq_than ||
	    op == op_greater_than || op == op_greater_eq_than) {
		return -1; /* apply second last, low priority */
	}
	if (op == op_mul) {
		return 1; /* apply first, high priority */
	}
	return 0; /* everything else left to right */
}

il_op p_get_operator()
{
	il_op op = op_generic;
	if (l_accept(t_plus)) {
		op = op_add;
	} else if (l_accept(t_minus)) {
		op = op_sub;
	} else if (l_accept(t_star)) {
		op = op_mul;
	} else if (l_accept(t_lshift)) {
		op = op_bit_lshift;
	} else if (l_accept(t_rshift)) {
		op = op_bit_rshift;
	} else if (l_accept(t_log_and)) {
		op = op_log_and;
	} else if (l_accept(t_log_or)) {
		op = op_log_or;
	} else if (l_accept(t_eq)) {
		op = op_equals;
	} else if (l_accept(t_noteq)) {
		op = op_not_equals;
	} else if (l_accept(t_lt)) {
		op = op_less_than;
	} else if (l_accept(t_le)) {
		op = op_less_eq_than;
	} else if (l_accept(t_gt)) {
		op = op_greater_than;
	} else if (l_accept(t_ge)) {
		op = op_greater_eq_than;
	} else if (l_accept(t_ampersand)) {
		op = op_bit_and;
	} else if (l_accept(t_bit_or)) {
		op = op_bit_or;
	}
	return op;
}

void p_read_expression(int param_no, block_def *parent)
{
	il_op op_stack[10];
	int op_stack_idx = 0;
	il_op op, next_op;
	il_instr *il;

	/* read value into param_no */
	p_read_expression_operand(param_no, parent);

	/* check for any operator following */
	op = p_get_operator();

	if (op == op_generic) {
		/* no continuation */
		return;
	}

	p_read_expression_operand(param_no + 1, parent);
	next_op = p_get_operator();

	if (next_op == op_generic) {
		/* only two operands, apply and return */
		il = add_instr(op);
		il->param_no = param_no;
		il->int_param1 = param_no + 1;
		return;
	}

	/* more than two operands - use stack */
	il = add_instr(op_push);
	il->param_no = param_no;

	il = add_instr(op_push);
	il->param_no = param_no + 1;

	op_stack[0] = op;
	op_stack_idx++;
	op = next_op;

	while (op != op_generic) {
		/* we have a continuation, use stack */
		int same_op = 0;

		/* if we have operand on stack, compare priorities */
		if (op_stack_idx > 0) {
			do {
				il_op stack_op = op_stack[op_stack_idx - 1];
				if (p_get_operator_priority(stack_op) >= p_get_operator_priority(op)) {
					/* stack has higher priority operator i.e. 5 * 6 + _ */
					/* pop stack and apply operators */
					il = add_instr(op_pop);
					il->param_no = param_no + 1;

					il = add_instr(op_pop);
					il->param_no = param_no;

					/* apply stack operator  */
					il = add_instr(stack_op);
					il->param_no = param_no;
					il->int_param1 = param_no + 1;

					/* push value back on stack */
					il = add_instr(op_push);
					il->param_no = param_no;

					/* pop op stack */
					op_stack_idx--;
				} else {
					same_op = 1;
				}
				/* continue util next operation is higher priority, i.e. 5 + 6 * _ */
			} while (op_stack_idx > 0 && same_op == 0);
		}

		/* push operator on stack */
		op_stack[op_stack_idx] = op;
		op_stack_idx++;

		/* push value on stack */
		p_read_expression_operand(param_no, parent);
		il = add_instr(op_push);
		il->param_no = param_no;

		op = p_get_operator();
	}

	/* unwind stack and apply operations */
	while (op_stack_idx > 0) {
		il_op stack_op = op_stack[op_stack_idx - 1];

		/* pop stack and apply operators */
		il = add_instr(op_pop);
		il->param_no = param_no + 1;

		il = add_instr(op_pop);
		il->param_no = param_no;

		/* apply stack operator  */
		il = add_instr(stack_op);
		il->param_no = param_no;
		il->int_param1 = param_no + 1;

		if (op_stack_idx == 1) {
			/* done */
			return;
		}

		/* push value back on stack */
		il = add_instr(op_push);
		il->param_no = param_no;

		/* pop op stack */
		op_stack_idx--;
	}

	error("Unexpected end of expression");
}

void p_read_function_parameters(block_def *parent)
{
	int param_num = 0;

	l_expect(t_op_bracket);

	while (!l_accept(t_cl_bracket)) {
		p_read_expression(param_num, parent);
		l_accept(t_comma);
		param_num++;
	}
}

void p_read_function_call(function_def *fn, int param_no, block_def *parent)
{
	il_instr *ii;

	/* we already have function name in fn */
	l_expect(t_identifier);

	p_read_function_parameters(parent);

	ii = add_instr(op_function_call);
	ii->string_param1 = fn->return_def.variable_name;
	ii->param_no = param_no; /* return value here */
}

/* returns address an expression points to, or evaluates its value */
/* x =; x[<expr>] =; x[expr].field =; x[expr]->field =; x + ... */
void p_read_lvalue(lvalue_def *lvalue, variable_def *var, block_def *parent, int param_no, int evaluate)
{
	il_instr *ii;
	int is_reference = 1;

	l_expect(t_identifier); /* we've already peeked and have the variable */

	/* load memory location into param */
	ii = add_instr(op_get_var_addr);
	ii->param_no = param_no;
	ii->string_param1 = var->variable_name;
	lvalue->type = find_type(var->type_name);
	lvalue->size = p_get_size(var, lvalue->type);
	lvalue->is_pointer = var->is_pointer;
	if (var->array_size > 0) {
		is_reference = 0;
	}

	while (l_peek(t_op_square, NULL) || l_peek(t_arrow, NULL) || l_peek(t_dot, NULL)) {
		if (l_accept(t_op_square)) {
			is_reference = 1;
			if (var->is_pointer <= 1) {
				/* if nested pointer, still pointer */
				lvalue->size = lvalue->type->size;
			}

			/* offset, so var must be either a pointer or an array of some type */
			if (var->is_pointer == 0 && var->array_size == 0) {
				error("Cannot apply square operator to non-pointer");
			}

			/* if var is an array, the memory location points to its start,
				but if var is a pointer, we need to dereference */
			if (var->is_pointer) {
				ii = add_instr(op_read_addr);
				ii->param_no = param_no;
				ii->int_param1 = param_no;
				ii->int_param2 = PTR_SIZE; /* pointer */
			}

			p_read_expression(param_no + 1, parent); /* param+1 has the offset in array terms */

			/* multiply by element size */
			if (lvalue->size != 1) {
				ii = add_instr(op_load_numeric_constant);
				ii->int_param1 = lvalue->size;
				ii->param_no = param_no + 2;

				ii = add_instr(op_mul);
				ii->param_no = param_no + 1;
				ii->int_param1 = param_no + 2;
			}

			ii = add_instr(op_add);
			ii->param_no = param_no;
			ii->int_param1 = param_no + 1;

			l_expect(t_cl_square);
		} else {
			char token[MAX_ID_LEN];

			if (l_accept(t_arrow)) {
				/* dereference first */
				ii = add_instr(op_read_addr);
				ii->param_no = param_no;
				ii->int_param1 = param_no;
				ii->int_param2 = PTR_SIZE;
			} else {
				l_expect(t_dot);
			}

			l_ident(t_identifier, token);

			/* change type currently pointed to */
			var = find_member(token, lvalue->type);
			lvalue->type = find_type(var->type_name);
			lvalue->is_pointer = var->is_pointer;

			/* reset target */
			is_reference = 1;
			lvalue->size = p_get_size(var, lvalue->type);
			if (var->array_size > 0) {
				is_reference = 0;
			}

			/* move pointer to offset of structure */
			ii = add_instr(op_load_numeric_constant);
			ii->int_param1 = var->offset;
			ii->param_no = param_no + 1;

			ii = add_instr(op_add);
			ii->param_no = param_no;
			ii->int_param1 = param_no + 1;
		}
	}

	if (evaluate) {
		/* do we need to apply pointer arithmetic? */
		if (l_peek(t_plus, NULL) && (var->is_pointer > 0 || var->array_size > 0)) {
			l_accept(t_plus);

			/* dereference if necessary */
			if (is_reference) {
				ii = add_instr(op_read_addr);
				ii->param_no = param_no;
				ii->int_param1 = param_no;
				ii->int_param2 = PTR_SIZE;
			}

			p_read_expression_operand(param_no + 1, parent); /* param+1 has the offset in array terms */

			/* shift by offset in type sizes */
			lvalue->size = lvalue->type->size;

			/* multiply by element size */
			if (lvalue->size != 1) {
				ii = add_instr(op_load_numeric_constant);
				ii->int_param1 = lvalue->size;
				ii->param_no = param_no + 2;

				ii = add_instr(op_mul);
				ii->param_no = param_no + 1;
				ii->int_param1 = param_no + 2;
			}

			ii = add_instr(op_add);
			ii->param_no = param_no;
			ii->int_param1 = param_no + 1;
		} else {
			/* we should NOT dereference if var is of type array and there was no offset */
			if (is_reference) {
				ii = add_instr(op_read_addr);
				ii->param_no = param_no;
				ii->int_param1 = param_no;
				ii->int_param2 = lvalue->size;
			}
		}
	}
}

int p_read_body_assignment(char *token, block_def *parent)
{
	il_instr *ii;
	variable_def *var = find_local_variable(token, parent);

	if (var == NULL) {
		var = find_global_variable(token);
	}
	if (var != NULL) {
		int one = 0;
		il_op op = op_generic;
		lvalue_def lvalue;
		int size = 0;

		/* a0 has memory address we want to set */
		p_read_lvalue(&lvalue, var, parent, 0, 0);
		size = lvalue.size;

		if (l_accept(t_plusplus)) {
			op = op_add;
			one = 1;
		} else if (l_accept(t_minusminus)) {
			op = op_sub;
			one = 1;
		} else if (l_accept(t_pluseq)) {
			op = op_add;
		} else if (l_accept(t_minuseq)) {
			op = op_sub;
		} else if (l_accept(t_oreq)) {
			op = op_bit_or;
		} else if (l_accept(t_andeq)) {
			op = op_bit_and;
		} else {
			l_expect(t_assign);
		}

		if (op != op_generic) {
			int increment_size = 1;

			/* if we have a pointer, we shift it by element size */
			if (lvalue.is_pointer) {
				increment_size = lvalue.type->size;
			}

			/* get current value into a1 */
			ii = add_instr(op_read_addr);
			ii->param_no = 1;
			ii->int_param1 = 0;
			ii->int_param2 = size;

			/* set a2 with either 1 or expression value */
			if (one == 1) {
				ii = add_instr(op_load_numeric_constant);
				ii->param_no = 2;
				ii->int_param1 = increment_size;
			} else {
				p_read_expression(2, parent);

				/* multiply by element size if necessary */
				if (increment_size != 1) {
					ii = add_instr(op_load_numeric_constant);
					ii->param_no = 3;
					ii->int_param1 = increment_size;

					ii = add_instr(op_mul);
					ii->param_no = 2;
					ii->int_param1 = 3;
				}
			}

			/* apply operation to value in a1 */
			ii = add_instr(op);
			ii->param_no = 1;
			ii->int_param1 = 2;
		} else {
			p_read_expression(1, parent); /* get expression value into a1 */
		}

		/* store a1 at addr a0, but need to know the type/size */
		ii = add_instr(op_write_addr);
		ii->param_no = 1;
		ii->int_param1 = 0;
		ii->int_param2 = size;

		return 1;
	}
	return 0;
}

void p_read_body_statement(block_def *parent)
{
	char token[MAX_ID_LEN];
	function_def *fn;
	type_def *type;
	variable_def *var;
	il_instr *ii;

	/* statement can be: function call, variable declaration, assignment operation, keyword, block */

	if (l_peek(t_op_curly, NULL)) {
		p_read_code_block(parent->function, parent);
		return;
	}

	if (l_accept(t_return)) {
		if (!l_accept(t_semicolon)) /* can be void */
		{
			p_read_expression(0, parent); /* get expression value into a0 / return value */
			l_expect(t_semicolon);
		}
		fn = parent->function;
		ii = add_instr(op_return);
		ii->string_param1 = fn->return_def.variable_name;
		return;
	}

	if (l_accept(t_if)) {
		il_instr *false_jump;
		il_instr *true_jump;

		l_expect(t_op_bracket);
		p_read_expression(0, parent); /* get expression value into a0 / return value */
		l_expect(t_cl_bracket);

		false_jump = add_instr(op_jz);
		false_jump->param_no = 0;

		p_read_body_statement(parent);

		/* if we have an else block, jump to finish */
		if (l_accept(t_else)) {
			/* jump true branch to finish */
			true_jump = add_instr(op_jump);

			/* we will emit false branch, link false jump here */
			ii = add_instr(op_label);
			false_jump->int_param1 = ii->il_index;

			/* false branch */
			p_read_body_statement(parent);

			/* this is finish, link true jump */
			ii = add_instr(op_label);
			true_jump->int_param1 = ii->il_index;
		} else {
			/* this is finish, link false jump */
			ii = add_instr(op_label);
			false_jump->int_param1 = ii->il_index;
		}
		return;
	}

	if (l_accept(t_while)) {
		il_instr *false_jump;
		il_instr *start;

		start = add_instr(op_label); /* start to return to */

		l_expect(t_op_bracket);
		p_read_expression(0, parent); /* get expression value into a0 / return value */
		l_expect(t_cl_bracket);

		false_jump = add_instr(op_jz);
		false_jump->param_no = 0;

		p_read_body_statement(parent);

		/* unconditional jump back to expression */
		ii = add_instr(op_jump);
		ii->int_param1 = start->il_index;

		/* exit label */
		ii = add_instr(op_label);
		false_jump->int_param1 = ii->il_index;
		return;
	}

	if (l_accept(t_for)) {
		char token[MAX_VAR_LEN];
		il_instr *condition_start;
		il_instr *condition_jump_out;
		il_instr *condition_jump_in;
		il_instr *increment;
		il_instr *increment_jump;
		il_instr *body_start;
		il_instr *body_jump;
		il_instr *end;

		l_expect(t_op_bracket);

		/* setup - execute once */
		if (!l_accept(t_semicolon)) {
			l_peek(t_identifier, token);
			p_read_body_assignment(token, parent);
			l_expect(t_semicolon);
		}

		/* condition - check before the loop */
		condition_start = add_instr(op_label);
		if (!l_accept(t_semicolon)) {
			p_read_expression(0, parent);
			l_expect(t_semicolon);
		} else {
			/* always true */
			il_instr *true = add_instr(op_load_numeric_constant);
			true->param_no = 0;
			true->int_param1 = 1;
		}

		condition_jump_out = add_instr(op_jz); /* jump out if zero */
		condition_jump_in = add_instr(op_jump); /* else jump to body */

		/* increment after each loop */
		increment = add_instr(op_label);
		if (!l_accept(t_cl_bracket)) {
			l_peek(t_identifier, token);
			p_read_body_assignment(token, parent);
			l_expect(t_cl_bracket);
		}

		/* jump back to condition */
		increment_jump = add_instr(op_jump);
		increment_jump->int_param1 = condition_start->il_index;

		/* loop body */
		body_start = add_instr(op_label);
		condition_jump_in->int_param1 = body_start->il_index;
		p_read_body_statement(parent);

		/* jump to increment */
		body_jump = add_instr(op_jump);
		body_jump->int_param1 = increment->il_index;

		end = add_instr(op_label);
		condition_jump_out->int_param1 = end->il_index;
		return;
	}

	if (l_accept(t_do)) {
		il_instr *false_jump;
		il_instr *start;

		start = add_instr(op_label); /* start to return to */

		p_read_body_statement(parent);
		l_expect(t_while);
		l_expect(t_op_bracket);
		p_read_expression(0, parent); /* get expression value into a0 / return value */
		l_expect(t_cl_bracket);

		false_jump = add_instr(op_jnz);
		false_jump->param_no = 0;
		false_jump->int_param1 = start->il_index;

		l_expect(t_semicolon);
		return;
	}

	if (l_accept(t_asm)) {
		char value[MAX_ID_LEN];
		int val;

		l_expect(t_op_bracket);
		l_ident(t_numeric, value);
		val = p_read_numeric_constant(value);
		l_expect(t_cl_bracket);
		l_expect(t_semicolon);

		add_generic(val);
		return;
	}

	if (l_accept(t_semicolon)) {
		/* empty statement */
		return;
	}

	/* must be an identifier */
	if (!l_peek(t_identifier, token)) {
		error("Unexpected token");
	}

	/* is it a variable declaration? */
	type = find_type(token);
	if (type != NULL) {
		var = &parent->locals[parent->next_local];
		p_read_full_variable_declaration(var);
		parent->next_local++;
		if (l_accept(t_assign)) {
			p_read_expression(1, parent); /* get expression value into a1 */
			/* assign a0 to our new variable */

			/* load variable location into a0 */
			ii = add_instr(op_get_var_addr);
			ii->param_no = 0;
			ii->string_param1 = var->variable_name;

			/* store a1 at addr a0, but need to know the type/size */
			ii = add_instr(op_write_addr);
			ii->param_no = 1;
			ii->int_param1 = 0;
			ii->int_param2 = p_get_size(var, type);
		}
		while (l_accept(t_comma)) {
			/* multiple (partial) declarations */
			variable_def *nv;

			nv = &parent->locals[parent->next_local];
			p_read_partial_variable_declaration(nv, var); /* partial */
			parent->next_local++;
			if (l_accept(t_assign)) {
				p_read_expression(1, parent); /* get expression value into a1 */
				/* assign a0 to our new variable */

				/* load variable location into a0 */
				ii = add_instr(op_get_var_addr);
				ii->param_no = 0;
				ii->string_param1 = nv->variable_name;

				/* store a1 at addr a0, but need to know the type/size */
				ii = add_instr(op_write_addr);
				ii->param_no = 1;
				ii->int_param1 = 0;
				ii->int_param2 = p_get_size(var, type);
			}
		}
		l_expect(t_semicolon);
		return;
	}

	/* is it a function call? */
	fn = find_function(token);
	if (fn != NULL) {
		p_read_function_call(fn, 0, parent);
		l_expect(t_semicolon);
		return;
	}

	/* is it an assignment? */
	if (p_read_body_assignment(token, parent)) {
		l_expect(t_semicolon);
		return;
	}

	error("Unrecognized statement token");
}

void p_read_code_block(function_def *function, block_def *parent)
{
	block_def *bd;
	il_instr *ii;

	bd = add_block(parent, function);
	ii = add_instr(op_block_start);
	ii->int_param1 = bd->bd_index;
	l_expect(t_op_curly);
	while (!l_accept(t_cl_curly)) {
		p_read_body_statement(bd);
	}
	ii = add_instr(op_block_end);
	ii->int_param1 = bd->bd_index;
}

void p_read_function_body(function_def *fdef)
{
	il_instr *ii;

	p_read_code_block(fdef, NULL);

	/* only add return when we have no return type, as otherwise there should have been a return statement */
	/*if (strcmp(fdef->return_def.type_name, "void") == 0) {*/
	ii = add_instr(op_exit_point);
	ii->string_param1 = fdef->return_def.variable_name;
	fdef->exit_point = ii->il_index;
	/*}*/
}

/* if first token in is type */
void p_read_global_declaration(block_def *block)
{
	/* new function, or variables under parent */
	p_read_full_variable_declaration(_temp_variable);

	if (l_peek(t_op_bracket, NULL)) {
		function_def *fd;
		il_instr *ii;

		/* function */
		fd = add_function(_temp_variable->variable_name);
		memcpy(&fd->return_def, _temp_variable, sizeof(variable_def));

		fd->num_params = p_read_parameter_list_declaration(fd->param_defs);

		if (l_peek(t_op_curly, NULL)) {
			ii = add_instr(op_entry_point);
			ii->string_param1 = fd->return_def.variable_name;
			fd->entry_point = ii->il_index;

			p_read_function_body(fd);
			return;
		} else if (l_accept(t_semicolon)) {
			/* forward definition */
			return;
		}
		error("Syntax error in global declaration");
	}

	/* it's a variable */
	memcpy(&block->locals[block->next_local], _temp_variable, sizeof(variable_def));
	block->next_local++;

	if (l_accept(t_assign)) {
		/* we don't support global initialisation */
		error("Global initialization not supported");
	} else if (l_accept(t_comma)) {
		/* TODO: continuation */
		error("Global continuation not supported");
	} else if (l_accept(t_semicolon)) {
		return;
	}
	error("Syntax error in global declaration");
}

void p_read_global_statement()
{
	char token[MAX_ID_LEN];
	block_def *block;

	block = &_blocks[0]; /* global block */

	if (l_peek(t_include, token)) {
		if (strcmp(_l_token_string, "<stdio.h>") == 0) {
			/* TODO */
		} /* otherwise ignore? */
		l_expect(t_include);
	} else if (l_accept(t_define)) {
		char alias[MAX_VAR_LEN];
		char value[MAX_VAR_LEN];

		l_peek(t_identifier, alias);
		l_expect(t_identifier);
		l_peek(t_numeric, value);
		l_expect(t_numeric);
		add_alias(alias, value);
	} else if (l_accept(t_typedef)) {
		if (l_accept(t_enum)) {
			int val = 0;
			char token[MAX_TYPE_LEN];
			type_def *type = add_type();

			type->base_type = bt_int;
			type->size = 4;
			l_expect(t_op_curly);
			do {
				l_ident(t_identifier, token);
				if (l_accept(t_assign)) {
					char value[MAX_ID_LEN];
					l_ident(t_numeric, value);
					val = p_read_numeric_constant(value);
				}
				add_constant(token, val);
				val++;
			} while (l_accept(t_comma));
			l_expect(t_cl_curly);
			l_ident(t_identifier, token);
			strcpy(type->type_name, token);
			l_expect(t_semicolon);
		} else if (l_accept(t_struct)) {
			char token[MAX_TYPE_LEN];
			int i = 0, size = 0;
			type_def *type = add_type();

			if (l_peek(t_identifier, token)) {
				/* for recursive declaration */
				l_accept(t_identifier);
			}
			l_expect(t_op_curly);
			do {
				variable_def *v = &type->fields[i];
				p_read_full_variable_declaration(v);
				v->offset = size;
				size += size_variable(v);
				i++;
				l_expect(t_semicolon);
			} while (!l_accept(t_cl_curly));

			l_ident(t_identifier, token); /* type name */
			strcpy(type->type_name, token);
			type->size = size;
			type->num_fields = i;
			type->base_type = bt_struct; /* is this used? */
			l_expect(t_semicolon);
		} else {
			char base_type[MAX_TYPE_LEN];
			type_def *base;
			type_def *type = add_type();
			l_ident(t_identifier, base_type);
			base = find_type(base_type);
			if (base == NULL) {
				error("Unable to find base type");
			}
			type->base_type = base->base_type;
			type->size = base->size;
			type->num_fields = 0;
			l_ident(t_identifier, type->type_name);
			l_expect(t_semicolon);
		}
	} else if (l_peek(t_identifier, NULL)) {
		p_read_global_declaration(block);
	} else {
		error("Syntax error in global statement");
	}
}

void p_parse()
{
	p_initialize();
	l_initialize();
	do {
		p_read_global_statement();
	} while (!l_accept(t_eof));
}
