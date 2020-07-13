int main(int argc, char *argv[])
{
char *str = "Hell\n";
int i = 0;
/*int i = strlen(str);
	__syscall(__syscall_write, 0, "s", 1);
while(i > 0)
{
	__syscall(__syscall_write, 0, ".", 1);
	i--;
}
	__syscall(__syscall_write, 0, "e", 1);
*/
/*
if(str[i] == 'H')
{
	__syscall(__syscall_write, 0, "Y", 1);
}
else{
	__syscall(__syscall_write, 0, "N", 1);
}*/


	__syscall(__syscall_write, 0, ".", 1);
while(str[i] != 0)
{
	__syscall(__syscall_write, 0, ".", 1);
i++;
}
i = 6;
while(i != 0)
{
	__syscall(__syscall_write, 0, "+", 1);
	i--;
}

/*if(i == 0)
{
	__syscall(__syscall_write, 0, "zero\n", 5);
}
else
{
	__syscall(__syscall_write, 0, "nonzero\n", 8);
}
__syscall(__syscall_write, 0, str, i);
*/
/*	printf("Hello world!");*/
	/*int x = argc;*/
/*	x = 666;*/
/*	char *c;
	c = "Hello";*/
	return 0;
}