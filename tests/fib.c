#include <stdio.h>
#include <stdlib.h>

#define NUM_FIBS 10

/*#include "compat.c"*/

typedef enum { enum1 = 5, enum2 } enum_t;

typedef struct {
	int v1;
	char txt[10];
	char *ptr;
	enum_t v2;
} struct_t;

void dump(int v)
{
	printf("%#010x\n", v);
}

int fib(int n);

int fibi(int n)
{
	int i;
	int t;
	int f1;
	int f2;
	i = 0;
	f1 = 0;
	f2 = 1;
	while (i != n) {
		t = f1;
		f1 = f2;
		f2 = f2 + t;
		i++;
	}
	return f1;
}

void printa(char text[])
{
	printf(text);
}

/*int main()
{
	printf("Hello %06d over %#010x in %s!\n", 600, 900, "textos");
	return 0;
}*/

int r_fib[NUM_FIBS];

int main()
{
	int i_fib[NUM_FIBS];

	r_fib[0] = 5;
	i_fib[0] = 6;

	enum_t i = 0;
	char *s;

	/*
	r_fib[0] = fib(6);
	i_fib[0] = fibi(6);
	printf("%d - %d\n", fib(6), fibi(6));
	printf("%d - %d\n", r_fib[0], i_fib[0]);
*/

	/*
	dump(1 + 1 + 1 + 1 + 1);

	printf("\"Start\"\n");
	printf("[");
	printf("");
	printf("]\n");

	int q = -6, r;
	r = -7;
	dump(q);
	dump(q - 1);
	dump(r);
	dump(r + 1);

	dump(2 * 3 + 4);
	dump((2 * 3) + 4);
	dump(2 * (3 + 4));
	dump(2 + 3 * 4);
	dump((2 + 3) * 4);
	dump(2 + (3 * 4));

	s = ">Hello\n";
	s[2] = '3';
	s[5] = '0';
	printa(s + 1);
	printf(&s[1]);*/

	int vvv = 10 - 1 - 1;

	printf("%d\n", vvv);

	while (i < NUM_FIBS) {
		r_fib[i] = fib(i);
		i_fib[i] = fibi(i);
		printf("i = %d, r = %d, i = %d\n", i, r_fib[i], i_fib[i]);
		if (i == enum1 || i == enum2) {
			printf("Hoho || = %d\n", -i);
		}
		if (i >= enum1 && i <= enum2) {
			printf("Hoho && = %d\n", -i);
		}
		i++;
	}

	i = NUM_FIBS - 1;
	printf("i = %d\n", i);
	do {
		int ix = 10 - 1 - i;
		printf("ix = %d\n", ix);
		dump(r_fib[NUM_FIBS - 1 - i]);
		dump(i_fib[NUM_FIBS - 1 - i]);
		i--;
	} while (!(i == 0));
	printa("World\n");
	return 0;
}

int fac(int n)
{
	if(n)
		return n * fac(n-1);
	else
		return 0;
}

int fib(int n)
{
	if (n == 0)
		return 0;
	else if (n == 1)
		return 1;
	else
		return fib(n - 1) + fib(n - 2);
}
