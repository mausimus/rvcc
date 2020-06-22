#include <stdio.h>
#include <stdlib.h>

/* code limits */
#define MAX_TOKEN_LEN 256
#define MAX_ID_LEN 64
#define MAX_LINE_LEN 256
#define MAX_VAR_LEN 64
#define MAX_TYPE_LEN 64
#define MAX_PARAMS 8
#define MAX_LOCALS 64
#define MAX_FIELDS 32
#define MAX_FUNCTIONS 1024
#define MAX_BLOCKS 1048576
#define MAX_TYPES 64
#define MAX_IL 262144
#define MAX_SOURCE 1048576
#define MAX_CODE 1048576
#define MAX_DATA 1048576
#define MAX_SYMTAB 65536
#define MAX_STRTAB 65536
#define MAX_HEADER 1024
#define MAX_FOOTER 1024
#define MAX_ALIASES 1024
#define MAX_CONSTANTS 1024

#define ELF_START 0x10000
#define PTR_SIZE 4

/* IL operation definitions */
typedef enum {
	/* generic - fixed assembly instruction */
	op_generic,
	/* function entry point */
	op_entry_point,
	/* retrieve address of a constant */
	op_load_data_address,
	/* program exit routine */
	op_exit,
	/* function call */
	op_function_call,
	/* function exit code */
	op_exit_point,
	/* jump to function exit */
	op_return,
	/* unconditional jump */
	op_jump,
	/* load constant */
	op_load_numeric_constant,
	/* note label */
	op_label,
	/* jump if false */
	op_jz,
	/* jump if true */
	op_jnz,
	/* push onto stack */
	op_push,
	/* pop from stack */
	op_pop,
	/* code block start */
	op_block_start,
	/* code block end */
	op_block_end,
	/* lookup variable's address */
	op_get_var_addr,
	/* read from memory address */
	op_read_addr,
	/* write to memory address */
	op_write_addr,
	/* arithmetic operators */
	op_add,
	op_sub,
	op_mul,
	op_bit_lshift,
	op_bit_rshift,
	op_log_and,
	op_log_or,
	op_not,
	op_equals,
	op_not_equals,
	op_less_than,
	op_less_eq_than,
	op_greater_than,
	op_greater_eq_than,
	op_bit_or,
	op_bit_and,
	op_negate
} il_op;

/* builtin types */
typedef enum { bt_void = 0, bt_int = 1, bt_char = 2, bt_struct = 3 } base_type;

/* RISC-V ISA opcodes */
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

/* lexer tokens */
typedef enum {
	t_sof,
	t_numeric,
	t_identifier,
	t_comma,
	t_string,
	t_char,
	t_op_bracket,
	t_cl_bracket,
	t_op_curly,
	t_cl_curly,
	t_op_square,
	t_cl_square,
	t_star,
	t_bit_or,
	t_log_and,
	t_log_or,
	t_log_not,
	t_lt,
	t_gt,
	t_le,
	t_ge,
	t_lshift,
	t_rshift,
	t_dot,
	t_arrow,
	t_plus,
	t_minus,
	t_minuseq,
	t_pluseq,
	t_oreq,
	t_andeq,
	t_eq,
	t_noteq,
	t_assign,
	t_plusplus,
	t_minusminus,
	t_semicolon,
	t_eof,
	t_ampersand,
	t_return,
	t_if,
	t_else,
	t_while,
	t_for,
	t_do,
	t_op_comment,
	t_cl_comment,
	t_define,
	t_include,
	t_typedef,
	t_enum,
	t_struct,
	t_sizeof,
	t_elipsis,
	t_asm
} l_token;

/* IL instruction */
typedef struct {
	il_op op; /* IL operation */
	int op_len; /*    binary length */
	int il_index; /* index in IL list */
	int code_offset; /* offset in code */
	int param_no; /* destination */
	int int_param1;
	int int_param2;
	char *string_param1;
} il_instr;

/* variable definition */
typedef struct {
	char type_name[MAX_TYPE_LEN];
	char variable_name[MAX_VAR_LEN];
	int is_pointer;
	int array_size;
	int offset; /* offset from stack or frame */
} variable_def;

/* function definition */
typedef struct {
	variable_def return_def;
	variable_def param_defs[MAX_PARAMS];
	int num_params;
	int entry_point; /* IL index */
	int exit_point; /* IL index */
	int params_size;
} function_def;

/* block definition */
typedef struct block_def {
	variable_def locals[MAX_LOCALS];
	int next_local;
	struct block_def *parent;
	function_def *function;
	int locals_size;
	int bd_index;
} block_def;

/* type definition */
typedef struct {
	char type_name[MAX_TYPE_LEN];
	base_type base_type;
	int size;
	variable_def fields[MAX_FIELDS];
	int num_fields;
} type_def;

/* lvalue details */
typedef struct {
	int size;
	int is_pointer;
	type_def *type;
} lvalue_def;

/* alias for #defines */
typedef struct {
	char alias[MAX_VAR_LEN];
	char value[MAX_VAR_LEN];
} alias_def;

/* constants for enums */
typedef struct defs {
	char alias[MAX_VAR_LEN];
	int value;
} constant_def;
