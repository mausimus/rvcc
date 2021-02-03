/* rvcc C compiler */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.c"
#include "globals.c"
#include "helpers.c"
#include "lexer.c"
#include "source.c"
#include "elf.c"
#include "arch/riscv.c"
#include "arch/arm.c"
#include "parser.c"
#include "codegen.c"

/* embedded clib */
#include "rvclib.inc"

int main(int argc, char *argv[])
{
	int i = 1, clib = 1;
	arch_t arch = a_riscv;
	char *outfile = NULL, *infile = NULL;

	printf("rvcc C compiler\n");

	while (i < argc) {
		if (strcmp(argv[i], "-noclib") == 0)
			clib = 0;
		else if (strcmp(argv[i], "-march=arm") == 0)
			arch = a_arm;
		else if (strcmp(argv[i], "-o") == 0)
			if (i < argc + 1) {
				outfile = argv[i + 1];
				i++;
			} else {
				abort();
			}
		else
			infile = argv[i];
		i++;
	}

	if (infile == NULL) {
		printf("Missing source file!\n");
		printf("Usage: rvcc [-o outfile] [-noclib] [-march=riscv|arm] <infile.c>\n");
		return -1;
	}

	/* initialize globals */
	g_initialize();

	/* include clib */
	if (clib) {
		s_clib();
	}

	/* load source code */
	s_load(infile);

	printf("Loaded %d source bytes\n", _source_idx);

	switch (arch) {
	case a_arm:
		a_initialize_backend(_backend);
		break;
	case a_riscv:
		r_initialize_backend(_backend);
		break;
	default:
		printf("Unsupported architecture!\n");
		return -1;
	}

	/* parse source into IL */
	p_parse();

	printf("Parsed into %d IL instructions\n", _il_idx);

	/* generate code from IL */
	c_generate(arch);

	printf("Compiled into %d code bytes and %d data bytes\n", _e_code_idx, _e_data_idx);

	/* output code in ELF */
	e_generate(outfile);

	return 0;
}
