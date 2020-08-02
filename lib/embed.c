/* rvcc C compiler - tool to refresh embedded version of rvclib from source */

#include <stdio.h>
#include <stdlib.h>

#define MAX_LINE_LEN 200
#define MAX_SIZE 65536

char *_source;
int _source_idx;

void s_write_char(char c)
{
	_source[_source_idx++] = c;
}

void s_write_string(char *str)
{
	int i = 0;
	while (str[i] != 0) {
		s_write_char(str[i++]);
	}
}

void s_write_line(char *src)
{
	int i = 0;

	s_write_string("__s(\"");

	while (src[i] != 0) {
		if (src[i] == '\"') {
			s_write_char('\\');
			s_write_char('\"');
		} else if (src[i] != '\n') {
			s_write_char(src[i]);
		}
		i++;
	}

	s_write_char('\\');
	s_write_char('n');
	s_write_string("\");\n");
}

void s_load(char *file)
{
	FILE *f;
	char buffer[MAX_LINE_LEN];

	printf("Loading source file %s\n", file);

	f = fopen(file, "rb");
	for (;;) {
		if (fgets(buffer, MAX_LINE_LEN, f) == NULL) {
			fclose(f);
			return;
		}
		s_write_line(buffer);
	}
	fclose(f);
}

void s_save(char *file)
{
	FILE *f;
	int i;

	printf("Writing destination file %s\n", file);

	f = fopen(file, "wb");
	for (i = 0; i < _source_idx; i++) {
		fputc(_source[i], f);
	}
	fclose(f);
}

int main(int argc, char *argv[])
{
	if (argc > 2) {
		_source_idx = 0;
		_source = malloc(MAX_SIZE);

		s_write_string("void s_clib() {\n");
		s_load(argv[1]);
		s_write_string("}\n");
		s_save(argv[2]);
	} else {
		printf("Usage: embed <input.c> <output.inc>\n");
	}
	return 0;
}
