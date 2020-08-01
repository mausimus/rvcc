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

int main(int argc, char *argv[])
{
	int i = 1, clib = 1;
	arch_t arch = a_riscv;
	char *outfile = NULL, *infile = NULL, *libpath = NULL;

	printf("rvcc C compiler\n");

	while (i < argc) {
		if (strcmp(argv[i], "-noclib") == 0) {
			clib = 0;
		} else if (strcmp(argv[i], "-march=arm") == 0) {
			arch = a_arm;
		} else if (strncmp(argv[i], "-L", 2) == 0) {
			libpath = argv[i] + 2;
		} else if (strcmp(argv[i], "-o") == 0) {
			if (i < argc + 1) {
				outfile = argv[i + 1];
				i++;
			} else {
				abort();
			}
		} else {
			infile = argv[i];
		}
		i++;
	}

	if (infile == NULL) {
		printf("Missing source file!\n");
		printf("Usage: rvcc [-o outfile] [-noclib] [-Llibpath] [-march=riscv|arm] <infile.c>\n");
		return -1;
	}

	/* initialize globals */
	g_initialize();

	/* include clib */
	if (clib) {
		char lib[MAX_TOKEN_LEN];
		if (libpath == NULL) {
			libpath = "lib";
		}
		strcpy(lib, libpath);
		strcpy(lib + strlen(libpath), "/clib.c");
		s_load(lib);
	}

	/* load source code */
	s_load(infile);

	printf("Loaded %d source bytes\n", _source_idx);

	/* parse source into IL */
	p_parse(arch);

	printf("Parsed into %d IL instructions\n", _il_idx);

	/* generate code from IL */
	c_generate(arch);

	printf("Compiled into %d code bytes and %d data bytes\n", _e_code_idx, _e_data_idx);

	/* output code in ELF */
	e_generate(outfile, arch);

	return 0;
}
