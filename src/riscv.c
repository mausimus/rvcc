/* rvcc C compiler - RISC-V ISA encoder */

int r_extract_bits(int imm, int i_start, int i_end, int d_start, int d_end)
{
	int v;

	if (d_end - d_start != i_end - i_start || i_start > i_end || d_start > d_end) {
		error("Invalid bit copy");
	}
	v = imm >> i_start;
	v = v & ((2 << (i_end - i_start)) - 1);
	v = v << d_start;
	return v;
}

int r_hi(int val)
{
	if ((val & (1 << 11)) != 0) {
		return val + 4096;
	} else {
		return val;
	}
}

int r_lo(int val)
{
	if ((val & (1 << 11)) != 0) {
		return (val & 0xFFF) - 4096;
	} else {
		return val & 0xFFF;
	}
}

int r_encode_R(ri_op op, r_reg rd, r_reg rs1, r_reg rs2)
{
	return op + (rd << 7) + (rs1 << 15) + (rs2 << 20);
}

int r_encode_I(ri_op op, r_reg rd, r_reg rs1, int imm)
{
	if (imm > 2047 || imm < -2048) {
		error("Offset too large");
	}
	if (imm < 0) {
		imm += 4096;
		imm &= (1 << 13) - 1;
	}
	return op + (rd << 7) + (rs1 << 15) + (imm << 20);
}

int r_encode_S(ri_op op, r_reg rs1, r_reg rs2, int imm)
{
	if (imm > 2047 || imm < -2048) {
		error("Offset too large");
	}
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
	if (imm > 4095 || imm < -4096) {
		error("Offset too large");
	}
	if (imm < 0) {
		sign = 1;
	}
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
