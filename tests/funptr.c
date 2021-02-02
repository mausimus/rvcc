int t1()
{
	return 7;
}

int t2(int x)
{
	return x + 1;
}

int main(int argc, char *argv[])
{
	int ra, rb;
	int (*ta)();
	int (*tb)(int);
	ta = t1;
	tb = t2;
	ra = ta();
	rb = tb(10);

	printf("%d\n", ra);
	printf("%d\n", rb);
	printf("Hello World\n");
	return 0;
}
