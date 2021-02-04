/* rvcc C compiler - ARMv7 ISA encoder */

/* ARMv7 opcodes */
typedef enum {
	ar_and = 0,
	ar_eor = 1,
	ar_sub = 2,
	ar_rsb = 3,
	ar_add = 4,
	ar_adc = 5,
	ar_sbc = 6,
	ar_rsc = 7,
	ar_tst = 8,
	ar_teq = 9,
	ar_cmp = 10,
	ar_cmn = 11,
	ar_orr = 12,
	ar_mov = 13,
	ar_bic = 14,
	ar_mvn = 15
} ar_op;

/* ARMv7 conditions */
typedef enum {
	ac_eq = 0,
	ac_ne = 1,
	ac_cs = 2,
	ac_cc = 3,
	ac_mi = 4,
	ac_pl = 5,
	ac_vs = 6,
	ac_vc = 7,
	ac_hi = 8,
	ac_ls = 9,
	ac_ge = 10,
	ac_lt = 11,
	ac_gt = 12,
	ac_le = 13,
	ac_al = 14
} ar_cond;

/* ARMv7 registers */
typedef enum {
	a_r0 = 0,
	a_r1 = 1,
	a_r2 = 2,
	a_r3 = 3,
	a_r4 = 4,
	a_r5 = 5,
	a_r6 = 6,
	a_r7 = 7,
	a_r8 = 8,
	a_r9 = 9,
	a_r10 = 10,
	a_s0 = 11, /* r11 */
	a_r12 = 12,
	a_sp = 13,
	a_lr = 14,
	a_pc = 15
} a_reg;

ar_cond a_get_cond(il_op op)
{
	switch (op) {
	case op_equals:
		return ac_eq;
	case op_not_equals:
		return ac_ne;
	case op_less_than:
		return ac_lt;
	case op_greater_eq_than:
		return ac_ge;
	case op_greater_than:
		return ac_gt;
	case op_less_eq_than:
		return ac_le;
	default:
		error("Unsupported condition IL op");
	}
	return ac_al;
}

int a_extract_bits(int imm, int i_start, int i_end, int d_start, int d_end)
{
	int v;

	if (d_end - d_start != i_end - i_start || i_start > i_end || d_start > d_end)
		error("Invalid bit copy");

	v = imm >> i_start;
	v = v & ((2 << (i_end - i_start)) - 1);
	v = v << d_start;
	return v;
}

int a_encode(ar_cond cond, int opcode, int rn, int rd, int op2)
{
	return (cond << 28) + (opcode << 20) + (rn << 16) + (rd << 12) + op2;
}

int a_swi()
{
	return a_encode(ac_al, 240, 0, 0, 0);
}

int a_mov(ar_cond cond, int io, int opcode, int s, int rn, int rd, int op2)
{
	int shift = 0;
	if (op2 > 255) {
		shift = 16; /* full rotation */
		while ((op2 & 3) == 0) {
			/* we can shift by two bits */
			op2 = op2 >> 2;
			shift -= 1;
		}
		if (op2 > 255)
			error("Unable to represent value"); /* value spans more than 8 bits */
	}
	return a_encode(cond, s + (opcode << 1) + (io << 5), rn, rd, (shift << 8) + (op2 & 255));
}

int a_and_i(ar_cond cond, a_reg rd, a_reg rs, int imm)
{
	return a_mov(cond, 1, ar_and, 0, rs, rd, imm);
}

int a_or_i(ar_cond cond, a_reg rd, a_reg rs, int imm)
{
	return a_mov(cond, 1, ar_orr, 0, rs, rd, imm);
}

int a_and_r(ar_cond cond, a_reg rd, a_reg rs, a_reg rm)
{
	return a_mov(cond, 0, ar_and, 0, rs, rd, rm);
}

int a_or_r(ar_cond cond, a_reg rd, a_reg rs, a_reg rm)
{
	return a_mov(cond, 0, ar_orr, 0, rs, rd, rm);
}

int a_movw(ar_cond cond, a_reg rd, int imm)
{
	return a_encode(cond, 48, 0, rd, 0) + a_extract_bits(imm, 0, 11, 0, 11) + a_extract_bits(imm, 12, 15, 16, 19);
}

int a_movt(ar_cond cond, a_reg rd, int imm)
{
	imm = imm >> 16;
	return a_encode(cond, 52, 0, rd, 0) + a_extract_bits(imm, 0, 11, 0, 11) + a_extract_bits(imm, 12, 15, 16, 19);
}

int a_mov_i(ar_cond cond, a_reg rd, int imm)
{
	return a_mov(cond, 1, ar_mov, 0, 0, rd, imm);
}

int a_mov_r(ar_cond cond, a_reg rd, a_reg rs)
{
	return a_mov(cond, 0, ar_mov, 0, 0, rd, rs);
}

int a_srl(ar_cond cond, a_reg rd, a_reg rm, a_reg rs)
{
	return a_encode(cond, 0 + (ar_mov << 1) + (0 << 5), 0, rd, rm + (1 << 4) + (1 << 5) + (rs << 8));
}

int a_sll(ar_cond cond, a_reg rd, a_reg rm, a_reg rs)
{
	return a_encode(cond, 0 + (ar_mov << 1) + (0 << 5), 0, rd, rm + (1 << 4) + (0 << 5) + (rs << 8));
}

int a_add_i(ar_cond cond, a_reg rd, a_reg rs, int imm)
{
	if (imm >= 0)
		return a_mov(cond, 1, ar_add, 0, rs, rd, imm);
	else
		return a_mov(cond, 1, ar_sub, 0, rs, rd, -imm);
}

int a_sub_i(ar_cond cond, a_reg rd, a_reg rs, int imm)
{
	return a_mov(cond, 1, ar_sub, 0, rs, rd, imm);
}

int a_add_r(ar_cond cond, a_reg rd, a_reg rs, a_reg ro)
{
	return a_mov(cond, 0, ar_add, 0, rs, rd, ro);
}

int a_sub_r(ar_cond cond, a_reg rd, a_reg rs, a_reg ro)
{
	return a_mov(cond, 0, ar_sub, 0, rs, rd, ro);
}

int a_zero(int rd)
{
	return a_mov_i(ac_al, rd, 0);
}

int a_nop()
{
	return a_mov_r(ac_al, a_r8, a_r8);
}

int a_transfer(ar_cond cond, int l, int size, a_reg rn, a_reg rd, int ofs)
{
	int opcode = 64 + 16 + 8 + l;
	if (size == 1)
		opcode += 4;
	if (ofs < 0) {
		opcode -= 8;
		ofs = -ofs;
	}
	return a_encode(cond, opcode, rn, rd, ofs & 4095);
}

int a_lw(ar_cond cond, a_reg rd, a_reg rn, int ofs)
{
	return a_transfer(cond, 1, 4, rn, rd, ofs);
}

int a_lb(ar_cond cond, a_reg rd, a_reg rn, int ofs)
{
	return a_transfer(cond, 1, 1, rn, rd, ofs);
}

int a_sw(ar_cond cond, a_reg rd, a_reg rn, int ofs)
{
	return a_transfer(cond, 0, 4, rn, rd, ofs);
}

int a_sb(ar_cond cond, a_reg rd, a_reg rn, int ofs)
{
	return a_transfer(cond, 0, 1, rn, rd, ofs);
}

int a_bx(ar_cond cond, a_reg rd)
{
	return a_encode(cond, 18, 15, 15, rd + 241);
}

int a_b(ar_cond cond, int ofs)
{
	int o = (ofs - 8) >> 2;
	return a_encode(cond, 160, 0, 0, 0) + (o & 16777215);
}

int a_bl(ar_cond cond, int ofs)
{
	int o = (ofs - 8) >> 2;
	return a_encode(cond, 176, 0, 0, 0) + (o & 16777215);
}

int a_blx(ar_cond cond, a_reg rd)
{
	return a_encode(cond, 18, 15, 15, rd + 3888);
}

int a_mul(ar_cond cond, a_reg rd, a_reg r1, a_reg r2)
{
	return a_encode(cond, 0, rd, 0, (r1 << 8) + 144 + r2);
}

int a_rsb_i(ar_cond cond, a_reg rd, int imm, a_reg rn)
{
	return a_mov(cond, 1, ar_rsb, 0, rd, rn, imm);
}

int a_cmp_r(ar_cond cond, a_reg rd, a_reg r1, a_reg r2)
{
	return a_mov(cond, 0, ar_cmp, 1, r1, rd, r2);
}

int a_teq(a_reg rd)
{
	return a_mov(ac_al, 1, ar_teq, 1, rd, 0, 0);
}

int a_elf_machine()
{
	return 0x28;
}

int a_elf_flags()
{
	return 0x5000200;
}

int a_dest_reg(int param_no)
{
	return param_no;
}

int a_get_code_length(il_instr *ii)
{
	il_op op = ii->op;
	function_def *fn;
	block_def *bd;

	switch (op) {
	case op_entry_point:
		fn = find_function(ii->string_param1);
		return 16 + (fn->num_params << 2);
	case op_function_call:
	case op_pointer_call:
		if (ii->param_no != 0)
			return 8;
		return 4;
	case op_load_numeric_constant:
		if (ii->int_param1 >= 0 && ii->int_param1 < 256)
			return 4;
		else
			return 8;
	case op_block_start:
	case op_block_end:
		bd = &_blocks[ii->int_param1];
		if (bd->next_local > 0)
			return 4;
		else
			return 0;
	case op_equals:
	case op_not_equals:
	case op_less_than:
	case op_less_eq_than:
	case op_greater_than:
	case op_greater_eq_than:
		return 12;
	case op_syscall:
		return 20;
	case op_exit_point:
		return 16;
	case op_exit:
		return 12;
	case op_load_data_address:
	case op_jz:
	case op_jnz:
	case op_push:
	case op_pop:
	case op_get_var_addr:
	case op_start:
		return 8;
	case op_jump:
	case op_return:
	case op_generic:
	case op_add:
	case op_sub:
	case op_mul:
	case op_read_addr:
	case op_write_addr:
	case op_log_or:
	case op_log_and:
	case op_not:
	case op_bit_or:
	case op_bit_and:
	case op_negate:
	case op_bit_lshift:
	case op_bit_rshift:
		return 4;
	case op_label:
		return 0;
	default:
		error("Unsupported IL op");
	}
	return 0;
}

void a_op_load_data_address(backend_state *state, int ofs)
{
	ofs += state->code_start;
	c_emit(a_movw(ac_al, state->dest_reg, ofs));
	c_emit(a_movt(ac_al, state->dest_reg, ofs));
}

void a_op_load_numeric_constant(backend_state *state, int val)
{
	if (val >= 0 && val < 256) {
		c_emit(a_mov_i(ac_al, state->dest_reg, val));
	} else {
		c_emit(a_movw(ac_al, state->dest_reg, val));
		c_emit(a_movt(ac_al, state->dest_reg, val));
	}
}

void a_op_get_global_addr(backend_state *state, int ofs)
{
	/* need to find the variable offset in data section, absolute */
	ofs += state->code_start;
	c_emit(a_movw(ac_al, state->dest_reg, ofs));
	c_emit(a_movt(ac_al, state->dest_reg, ofs));
}

void a_op_get_local_addr(backend_state *state, int offset)
{
	c_emit(a_add_i(ac_al, state->dest_reg, a_s0, offset & 255));
	c_emit(a_add_i(ac_al, state->dest_reg, state->dest_reg, offset - (offset & 255)));
}

void a_op_get_function_addr(backend_state *state, int ofs)
{
	c_emit(a_movw(ac_al, state->dest_reg, ofs));
	c_emit(a_movt(ac_al, state->dest_reg, ofs));
}

void a_op_read_addr(backend_state *state, int len)
{
	switch (len) {
	case 4:
		c_emit(a_lw(ac_al, state->dest_reg, state->op_reg, 0));
		break;
	case 1:
		c_emit(a_lb(ac_al, state->dest_reg, state->op_reg, 0));
		break;
	default:
		error("Unsupported word size");
	}
}

void a_op_write_addr(backend_state *state, int len)
{
	switch (len) {
	case 4:
		c_emit(a_sw(ac_al, state->dest_reg, state->op_reg, 0));
		break;
	case 1:
		c_emit(a_sb(ac_al, state->dest_reg, state->op_reg, 0));
		break;
	default:
		error("Unsupported word size");
	}
}

void a_op_jump(int ofs)
{
	c_emit(a_b(ac_al, ofs));
}

void a_op_return(int ofs)
{
	c_emit(a_b(ac_al, ofs));
}

void a_op_function_call(backend_state *state, int ofs)
{
	c_emit(a_bl(ac_al, ofs));
	if (state->dest_reg != a_r0)
		c_emit(a_mov_r(ac_al, state->dest_reg, a_r0));
}

void a_op_pointer_call(backend_state *state)
{
	c_emit(a_blx(ac_al, state->op_reg));
	if (state->dest_reg != a_r0)
		c_emit(a_mov_r(ac_al, state->dest_reg, a_r0));
}

void a_op_push(backend_state *state)
{
	c_emit(a_add_i(ac_al, a_sp, a_sp, -16)); /* 16 aligned although we only need 4 */
	c_emit(a_sw(ac_al, state->dest_reg, a_sp, 0));
}

void a_op_pop(backend_state *state)
{
	c_emit(a_lw(ac_al, state->dest_reg, a_sp, 0));
	c_emit(a_add_i(ac_al, a_sp, a_sp, 16)); /* 16 aligned although we only need 4 */
}

void a_op_exit_point()
{
	c_emit(a_add_i(ac_al, a_sp, a_s0, 16));
	c_emit(a_lw(ac_al, a_lr, a_sp, -8));
	c_emit(a_lw(ac_al, a_s0, a_sp, -4));
	c_emit(a_mov_r(ac_al, a_pc, a_lr));
}

void a_op_alu(backend_state *state, il_op op)
{
	switch (op) {
	case op_add:
		c_emit(a_add_r(ac_al, state->dest_reg, state->dest_reg, state->op_reg));
		break;
	case op_sub:
		c_emit(a_sub_r(ac_al, state->dest_reg, state->dest_reg, state->op_reg));
		break;
	case op_mul:
		c_emit(a_mul(ac_al, state->dest_reg, state->dest_reg, state->op_reg));
		break;
	case op_negate:
		c_emit(a_rsb_i(ac_al, state->dest_reg, 0, state->dest_reg));
		break;
	default:
		break;
	}
}

void a_op_cmp(backend_state *state, il_op op)
{
	ar_cond cond = a_get_cond(op);
	c_emit(a_cmp_r(ac_al, state->dest_reg, state->dest_reg, state->op_reg));
	c_emit(a_zero(state->dest_reg));
	c_emit(a_mov_i(cond, state->dest_reg, 1));
}

void a_op_log(backend_state *state, il_op op)
{
	switch (op) {
	case op_log_and:
		/* we assume both have to be 1, they can't be just nonzero */
		c_emit(a_and_r(ac_al, state->dest_reg, state->dest_reg, state->op_reg));
		break;
	case op_log_or:
		c_emit(a_or_r(ac_al, state->dest_reg, state->dest_reg, state->op_reg));
		break;
	default:
		break;
	}
}

void a_op_bit(backend_state *state, il_op op)
{
	switch (op) {
	case op_bit_and:
		c_emit(a_and_r(ac_al, state->dest_reg, state->dest_reg, state->op_reg));
		break;
	case op_bit_or:
		c_emit(a_or_r(ac_al, state->dest_reg, state->dest_reg, state->op_reg));
		break;
	case op_bit_lshift:
		c_emit(a_sll(ac_al, state->dest_reg, state->dest_reg, state->op_reg));
		break;
	case op_bit_rshift:
		c_emit(a_srl(ac_al, state->dest_reg, state->dest_reg, state->op_reg));
		break;
	case op_not:
		/* only works for 1/0 */
		c_emit(a_rsb_i(ac_al, state->dest_reg, 1, state->dest_reg));
		break;
	default:
		break;
	}
}

void a_op_jz(backend_state *state, il_op op, int ofs)
{
	c_emit(a_teq(state->dest_reg));
	if (op == op_jz) {
		c_emit(a_b(ac_eq, ofs));
	} else {
		c_emit(a_b(ac_ne, ofs));
	}
}

void a_op_block(int len)
{
	c_emit(a_add_i(ac_al, a_sp, a_sp, len));
}

void a_op_entry_point(int len)
{
	c_emit(a_add_i(ac_al, a_sp, a_sp, -16 - len));
	c_emit(a_sw(ac_al, a_s0, a_sp, 12 + len));
	c_emit(a_sw(ac_al, a_lr, a_sp, 8 + len));
	c_emit(a_add_i(ac_al, a_s0, a_sp, len));
}

void a_op_store_param(int pn, int ofs)
{
	c_emit(a_sw(ac_al, a_r0 + pn, a_s0, ofs));
}

void a_op_start()
{
	c_emit(a_lw(ac_al, a_r0, a_sp, 0)); /* argc */
	c_emit(a_add_i(ac_al, a_r1, a_sp, 4)); /* argv */
}

void a_op_syscall()
{
	c_emit(a_mov_r(ac_al, a_r7, a_r0));
	c_emit(a_mov_r(ac_al, a_r0, a_r1));
	c_emit(a_mov_r(ac_al, a_r1, a_r2));
	c_emit(a_mov_r(ac_al, a_r2, a_r3));
	c_emit(a_swi());
}

void a_op_exit()
{
	c_emit(a_mov_i(ac_al, a_r0, 0));
	c_emit(a_mov_i(ac_al, a_r7, 1));
	c_emit(a_swi());
}

void a_initialize_backend(backend_def *be)
{
	be->arch = a_arm;
	be->source_define = "__ARM";
	be->elf_machine = a_elf_machine;
	be->elf_flags = a_elf_flags;
	be->c_dest_reg = a_dest_reg;
	be->c_get_code_length = a_get_code_length;
	be->op_load_data_address = a_op_load_data_address;
	be->op_load_numeric_constant = a_op_load_numeric_constant;
	be->op_get_global_addr = a_op_get_global_addr;
	be->op_get_local_addr = a_op_get_local_addr;
	be->op_get_function_addr = a_op_get_function_addr;
	be->op_read_addr = a_op_read_addr;
	be->op_write_addr = a_op_write_addr;
	be->op_jump = a_op_jump;
	be->op_return = a_op_return;
	be->op_function_call = a_op_function_call;
	be->op_pointer_call = a_op_pointer_call;
	be->op_push = a_op_push;
	be->op_pop = a_op_pop;
	be->op_exit_point = a_op_exit_point;
	be->op_alu = a_op_alu;
	be->op_cmp = a_op_cmp;
	be->op_log = a_op_log;
	be->op_bit = a_op_bit;
	be->op_jz = a_op_jz;
	be->op_block = a_op_block;
	be->op_entry_point = a_op_entry_point;
	be->op_store_param = a_op_store_param;
	be->op_start = a_op_start;
	be->op_syscall = a_op_syscall;
	be->op_exit = a_op_exit;
}
