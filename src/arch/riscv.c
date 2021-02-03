/* rvcc C compiler - RISC-V ISA encoder */

/* RISC-V opcodes */
typedef enum {
	/* R type */
	ri_add = 51 /* 0b110011 + (0 << 12) */,
	ri_sub = 1073741875 /* 0b110011 + (0 << 12) + (0x20 << 25) */,
	ri_xor = 16435 /* 0b110011 + (4 << 12) */,
	ri_or = 24627 /* 0b110011 + (6 << 12) */,
	ri_and = 28723 /* 0b110011 + (7 << 12) */,
	ri_sll = 4147 /* 0b110011 + (1 << 12) */,
	ri_srl = 20531 /* 0b110011 + (5 << 12) */,
	ri_sra = 1073762355 /* 0b110011 + (5 << 12) + (0x20 << 25) */,
	ri_slt = 8243 /* 0b110011 + (2 << 12) */,
	ri_sltu = 12339 /* 0b110011 + (3 << 12) */,
	/* I type */
	ri_addi = 19 /* 0b0010011 */,
	ri_xori = 16403 /* 0b0010011 + (4 << 12) */,
	ri_ori = 24595 /* 0b0010011 + (6 << 12) */,
	ri_andi = 28691 /* 0b0010011 + (7 << 12) */,
	ri_slli = 4115 /* 0b0010011 + (1 << 12) */,
	ri_srli = 20499 /* 0b0010011 + (5 << 12) */,
	ri_srai = 1073762323 /* 0b0010011 + (5 << 12) + (0x20 << 25) */,
	ri_slti = 8211 /* 0b0010011 + (2 << 12) */,
	ri_sltiu = 12307 /* 0b0010011 + (3 << 12) */,
	/* load/store */
	ri_lb = 3 /* 0b11 */,
	ri_lh = 4099 /* 0b11 + (1 << 12) */,
	ri_lw = 8195 /* 0b11 + (2 << 12) */,
	ri_lbu = 16387 /* 0b11 + (4 << 12) */,
	ri_lhu = 20483 /* 0b11 + (5 << 12) */,
	ri_sb = 35 /* 0b0100011 */,
	ri_sh = 4131 /* 0b0100011 + (1 << 12) */,
	ri_sw = 8227 /* 0b0100011 + (2 << 12) */,
	/* branch */
	ri_beq = 99 /* 0b1100011 */,
	ri_bne = 4195 /* 0b1100011 + (1 << 12) */,
	ri_blt = 16483 /* 0b1100011 + (4 << 12) */,
	ri_bge = 20579 /* 0b1100011 + (5 << 12) */,
	ri_bltu = 24675 /* 0b1100011 + (6 << 12) */,
	ri_bgeu = 28771 /* 0b1100011 + (7 << 12) */,
	/* jumps */
	ri_jal = 111 /* 0b1101111 */,
	ri_jalr = 103 /* 0b1100111 */,
	/* misc */
	ri_lui = 55 /* 0b0110111 */,
	ri_auipc = 23 /*0 b0010111 */,
	ri_ecall = 115 /* 0b1110011 */,
	ri_ebreak = 1048691 /* 0b1110011 + (1 << 20) */,
	/* m */
	ri_mul = 33554483 /* 0b0110011 + (1 << 25) */
} ri_op;

/* RISC-V registers */
typedef enum {
	r_zero = 0,
	r_ra = 1,
	r_sp = 2,
	r_gp = 3,
	r_tp = 4,
	r_t0 = 5,
	r_t1 = 6,
	r_t2 = 7,
	r_s0 = 8,
	r_s1 = 9,
	r_a0 = 10,
	r_a1 = 11,
	r_a2 = 12,
	r_a3 = 13,
	r_a4 = 14,
	r_a5 = 15,
	r_a6 = 16,
	r_a7 = 17,
	r_s2 = 18,
	r_s3 = 19,
	r_s4 = 20,
	r_s5 = 21,
	r_s6 = 22,
	r_s7 = 23,
	r_s8 = 24,
	r_s9 = 25,
	r_s10 = 26,
	r_s11 = 27,
	r_t3 = 28,
	r_t4 = 29,
	r_t5 = 30,
	r_t6 = 31
} r_reg;

int r_extract_bits(int imm, int i_start, int i_end, int d_start, int d_end)
{
	int v;

	if (d_end - d_start != i_end - i_start || i_start > i_end || d_start > d_end)
		error("Invalid bit copy");

	v = imm >> i_start;
	v = v & ((2 << (i_end - i_start)) - 1);
	v = v << d_start;
	return v;
}

int r_hi(int val)
{
	if ((val & (1 << 11)) != 0)
		return val + 4096;
	else
		return val;
}

int r_lo(int val)
{
	if ((val & (1 << 11)) != 0)
		return (val & 0xFFF) - 4096;
	else
		return val & 0xFFF;
}

int r_encode_R(ri_op op, r_reg rd, r_reg rs1, r_reg rs2)
{
	return op + (rd << 7) + (rs1 << 15) + (rs2 << 20);
}

int r_encode_I(ri_op op, r_reg rd, r_reg rs1, int imm)
{
	if (imm > 2047 || imm < -2048)
		error("Offset too large");

	if (imm < 0) {
		imm += 4096;
		imm &= (1 << 13) - 1;
	}
	return op + (rd << 7) + (rs1 << 15) + (imm << 20);
}

int r_encode_S(ri_op op, r_reg rs1, r_reg rs2, int imm)
{
	if (imm > 2047 || imm < -2048)
		error("Offset too large");

	if (imm < 0) {
		imm += 4096;
		imm &= (1 << 13) - 1;
	}
	return op + (rs1 << 15) + (rs2 << 20) + r_extract_bits(imm, 0, 4, 7, 11) + r_extract_bits(imm, 5, 11, 25, 31);
}

int r_encode_B(ri_op op, r_reg rs1, r_reg rs2, int imm)
{
	int sign = 0;

	/* 13 signed bits, with bit zero ignored */
	if (imm > 4095 || imm < -4096)
		error("Offset too large");

	if (imm < 0)
		sign = 1;

	return op + (rs1 << 15) + (rs2 << 20) + r_extract_bits(imm, 11, 11, 7, 7) + r_extract_bits(imm, 1, 4, 8, 11) +
	       r_extract_bits(imm, 5, 10, 25, 30) + (sign << 31);
}

int r_encode_J(ri_op op, r_reg rd, int imm)
{
	int sign = 0;

	if (imm < 0) {
		sign = 1;
		imm = -imm;
		imm = (1 << 21) - imm;
	}
	return op + (rd << 7) + r_extract_bits(imm, 1, 10, 21, 30) + r_extract_bits(imm, 11, 11, 20, 20) +
	       r_extract_bits(imm, 12, 19, 12, 19) + (sign << 31);
}

int r_encode_U(ri_op op, r_reg rd, int imm)
{
	return op + (rd << 7) + r_extract_bits(imm, 12, 31, 12, 31);
}

int r_add(r_reg rd, r_reg rs1, r_reg rs2)
{
	return r_encode_R(ri_add, rd, rs1, rs2);
}

int r_sub(r_reg rd, r_reg rs1, r_reg rs2)
{
	return r_encode_R(ri_sub, rd, rs1, rs2);
}

int r_xor(r_reg rd, r_reg rs1, r_reg rs2)
{
	return r_encode_R(ri_xor, rd, rs1, rs2);
}

int r_or(r_reg rd, r_reg rs1, r_reg rs2)
{
	return r_encode_R(ri_or, rd, rs1, rs2);
}

int r_and(r_reg rd, r_reg rs1, r_reg rs2)
{
	return r_encode_R(ri_and, rd, rs1, rs2);
}

int r_sll(r_reg rd, r_reg rs1, r_reg rs2)
{
	return r_encode_R(ri_sll, rd, rs1, rs2);
}

int r_srl(r_reg rd, r_reg rs1, r_reg rs2)
{
	return r_encode_R(ri_srl, rd, rs1, rs2);
}

int r_sra(r_reg rd, r_reg rs1, r_reg rs2)
{
	return r_encode_R(ri_sra, rd, rs1, rs2);
}

int r_slt(r_reg rd, r_reg rs1, r_reg rs2)
{
	return r_encode_R(ri_slt, rd, rs1, rs2);
}

int r_sltu(r_reg rd, r_reg rs1, r_reg rs2)
{
	return r_encode_R(ri_sltu, rd, rs1, rs2);
}

int r_addi(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_I(ri_addi, rd, rs1, imm);
}

int r_xori(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_I(ri_xori, rd, rs1, imm);
}

int r_ori(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_I(ri_ori, rd, rs1, imm);
}

int r_andi(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_I(ri_andi, rd, rs1, imm);
}

int r_slli(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_I(ri_slli, rd, rs1, imm);
}

int r_srli(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_I(ri_srli, rd, rs1, imm);
}

int r_srai(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_I(ri_srai, rd, rs1, imm);
}

int r_slti(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_I(ri_slti, rd, rs1, imm);
}

int r_sltiu(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_I(ri_sltiu, rd, rs1, imm);
}

int r_lb(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_I(ri_lb, rd, rs1, imm);
}

int r_lh(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_I(ri_lh, rd, rs1, imm);
}

int r_lw(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_I(ri_lw, rd, rs1, imm);
}

int r_lbu(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_I(ri_lbu, rd, rs1, imm);
}

int r_lhu(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_I(ri_lhu, rd, rs1, imm);
}

int r_sb(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_S(ri_sb, rs1, rd, imm);
}

int r_sh(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_S(ri_sh, rs1, rd, imm);
}

int r_sw(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_S(ri_sw, rs1, rd, imm);
}

int r_beq(r_reg rs1, r_reg rs2, int imm)
{
	return r_encode_B(ri_beq, rs1, rs2, imm);
}

int r_bne(r_reg rs1, r_reg rs2, int imm)
{
	return r_encode_B(ri_bne, rs1, rs2, imm);
}

int r_blt(r_reg rs1, r_reg rs2, int imm)
{
	return r_encode_B(ri_blt, rs1, rs2, imm);
}

int r_bge(r_reg rs1, r_reg rs2, int imm)
{
	return r_encode_B(ri_bge, rs1, rs2, imm);
}

int r_bltu(r_reg rs1, r_reg rs2, int imm)
{
	return r_encode_B(ri_bltu, rs1, rs2, imm);
}

int r_bgeu(r_reg rs1, r_reg rs2, int imm)
{
	return r_encode_B(ri_bgeu, rs1, rs2, imm);
}

int r_jal(r_reg rd, int imm)
{
	return r_encode_J(ri_jal, rd, imm);
}

int r_jalr(r_reg rd, r_reg rs1, int imm)
{
	return r_encode_I(ri_jalr, rd, rs1, imm);
}

int r_lui(r_reg rd, int imm)
{
	return r_encode_U(ri_lui, rd, imm);
}

int r_auipc(r_reg rd, int imm)
{
	return r_encode_U(ri_auipc, rd, imm);
}

int r_ecall()
{
	return r_encode_I(ri_ecall, r_zero, r_zero, 0);
}

int r_ebreak()
{
	return r_encode_I(ri_ebreak, r_zero, r_zero, 1);
}

int r_nop()
{
	return r_addi(r_zero, r_zero, 0);
}

int r_mul(r_reg rd, r_reg rs1, r_reg rs2)
{
	return r_encode_R(ri_mul, rd, rs1, rs2);
}

int r_elf_machine()
{
	return 0xf3;
}

int r_elf_flags()
{
	return 0x5000200;
}

int r_dest_reg(int param_no)
{
	return param_no + 10;
}

int r_get_code_length(il_instr *ii)
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
		if (ii->int_param1 > -2048 && ii->int_param1 < 2047)
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
		return 16;
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

void r_op_load_data_address(backend_state *state, int ofs)
{
	ofs -= state->pc;
	c_emit(r_auipc(state->dest_reg, r_hi(ofs)));
	c_emit(r_addi(state->dest_reg, state->dest_reg, r_lo(ofs)));
}
/*
void r_op_load_numeric_constant(backend_state *state, int val)
{
	if (val > -2048 && val < 2047) {
		c_emit(r_addi(state->dest_reg, r_zero, r_lo(val)));
	} else {
		c_emit(r_lui(state->dest_reg, r_hi(val)));
		c_emit(r_addi(state->dest_reg, state->dest_reg, r_lo(val)));
	}
}
*/
void r_initialize_backend(backend_def *be)
{
	be->arch = a_riscv;
	be->source_define = "__RISCV";
	be->elf_machine = r_elf_machine;
	be->elf_flags = r_elf_flags;
	be->c_dest_reg = r_dest_reg;
	be->c_get_code_length = r_get_code_length;
	be->op_load_data_address = r_op_load_data_address;
	/*	be->op_load_numeric_constant = r_op_load_numeric_constant;*/
}
