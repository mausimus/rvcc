#include <stdio.h>
#include <string.h>

void s_write_string(char *src)
{
	int i = 0;
	while (src[i] != 0) {
		_source[_source_idx] = src[i];
		_source_idx++;
		i++;
	}
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
		if (strncmp(buffer, "#include ", 9) == 0 && buffer[9] == '"') {
			char path[MAX_LINE_LEN];
			int c = strlen(file) - 1;
			while (c > 0 && file[c] != '/') {
				c--;
			}
			if (c != 0) {
				/* prepend directory name */
				strncpy(path, file, c + 1);
				c++;
			}
			path[c] = 0;
			buffer[strlen(buffer) - 2] = 0;
			strcpy(path + c, buffer + 10);
			s_load(path);
		} else {
			strcpy(_source + _source_idx, buffer);
			_source_idx += strlen(buffer);
		}
	}
	fclose(f);
}
