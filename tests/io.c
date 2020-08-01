#include <stdio.h>

void copy(FILE *in, FILE *out)
{
	char buffer[200];
	int i = 1;

	do {
		printf("Attempting to read line %d\n", i);
		if (fgets(buffer, 200, in) == NULL) {
			printf("Line %d is NULL\n", i);
			return;
		}
		printf("Attempting to write line %d\n", i);
		fputs(buffer, out);
		i++;
	} while (1);
}

void fcopy_test()
{
	FILE *in = fopen("in.txt", "rb");
	FILE *out = fopen("out.txt", "wb");
	copy(in, out);
	printf("Closing files\n");
	fclose(in);
	fclose(out);
}

void malloc_test()
{
	int *array1, *array2, i = 0;
	array1 = malloc(1024 * 4);
	array2 = malloc(1024 * 4);
	printf("%d %d\n", array1, array2);
	while (i < 1024) {
		array1[i] = 1;
		array2[i] = 2;
		i++;
	}
}

int main(int argc, char *argv[])
{
	printf("Num args: %d\n", argc);
	while (argc > 0) {
		argc--;
		printf("%s\n", argv[argc]);
	}

	fcopy_test();
	malloc_test();

	return 0;
}
