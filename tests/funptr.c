typedef struct {
	int (*ta)();
	int (*tb)(int);
} fstr;

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
	fstr fb;
	fstr *fs = &fb;
	fs->ta = t1;
	fs->tb = t2;
	ra = fs->ta();
	rb = fs->tb(10);

	printf("%d\n", ra);
	printf("%d\n", rb);
	printf("Hello World\n");
	return 0;
}
