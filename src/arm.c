/* rvcc C compiler - ARMv7 ISA encoder */

int a_extract_bits(int imm, int i_start, int i_end, int d_start, int d_end)
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

int a_encode(ar_cond cond, int opcode, int rn, int rd, int op2)
{
	return (cond << 28) + (opcode << 20) + (rn << 16) + (rd << 12) + op2;
}

int a_hi(int val)
{
	/* 8 higher bits */
	return (val >> 8) & 255;
}

int a_lo(int val)
{
	/* lowest 8 value bits */
	return val & 255;
}

int a_swi()
{
	return a_encode(ac_al, 240, 0, 0, 0);
}

int a_mov(ar_cond cond, int io, int opcode, int s, int rn, int rd, int op2)
{
	int shift = 0;
	while (op2 > 255) {
		/* shift down if possible */
		if ((op2 & 1) == 0) {
			shift++;
			op2 = op2 >> 1;
		} else {
			error("Unable to represent value");
		}
	}
	return a_encode(cond, s + (opcode << 1) + (io << 5), rn, rd, (shift << 8) + (op2 & 255));
}

int a_movw(ar_cond cond, a_reg rd, int imm)
{
	return a_encode(cond, 48, 0, rd, 0) + a_extract_bits(imm, 0, 11, 0, 11) + a_extract_bits(imm, 12, 15, 16, 19);
	/*return a_encode(cond, 36, 0, rd, 0) + a_extract_bits(imm, 0, 7, 0, 7) + a_extract_bits(imm, 8, 10, 12, 14) +
	       a_extract_bits(imm, 11, 11, 26, 26) + a_extract_bits(imm, 12, 15, 16, 19);*/
}

int a_movt(ar_cond cond, a_reg rd, int imm)
{
	imm = imm >> 16;
	return a_encode(cond, 52, 0, rd, 0) + a_extract_bits(imm, 0, 11, 0, 11) + a_extract_bits(imm, 12, 15, 16, 19);
	/*return a_encode(cond, 44, 0, rd, 0) + a_extract_bits(imm, 0, 7, 0, 7) + a_extract_bits(imm, 8, 10, 12, 14) +
	       a_extract_bits(imm, 11, 11, 26, 26) + a_extract_bits(imm, 12, 15, 16, 19);*/
}

int a_mov_i(ar_cond cond, a_reg rd, int imm)
{
	return a_mov(cond, 1, ar_mov, 0, 0, rd, imm);
}

int a_mov_r(ar_cond cond, a_reg rd, a_reg rs)
{
	return a_mov(cond, 0, ar_mov, 0, 0, rd, rs);
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
	if (size == 1) {
		opcode += 4;
	}
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
