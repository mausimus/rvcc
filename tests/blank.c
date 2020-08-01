/*#define __syscall_exit 1
#define __syscall_read 3
#define __syscall_write 4
#define __syscall_close 6
#define __syscall_open 5
#define __syscall_brk 45
*/

int main(int argc, char *argv[])
{
/*char *str = "Hell\n";*/
/*int i = 0;*/

	__syscall(__syscall_write, 0, "1\n", 2);
	__syscall(__syscall_write, 0, "2\n", 2);
	__syscall(__syscall_write, 0, "3\n", 2);
	__syscall(__syscall_write, 0, "4\n", 2);
	__syscall(__syscall_write, 0, "5\n", 2);
	__syscall(__syscall_write, 0, "6\n", 2);
	__syscall(__syscall_write, 0, "7\n", 2);
	__syscall(__syscall_write, 0, "8\n", 2);
	__syscall(__syscall_write, 0, "9\n", 2);
/*	__syscall(__syscall_write, 0, "S\n", 2);*/
/*while(str[i] != 0)
{
	__syscall(__syscall_write, 0, "X", 1);
i++;
}
*/
/*i = 6;
while(i != 0)
{
	__syscall(__syscall_write, 0, "+", 1);
	i--;
}*/
/*
i++;
i++;
i++;
i++;
i++;
i++;
i++;
i++;
i++;
i++;
i++;
i++;
i++;
i++;
i++;
i++;
*/
	return 0;
}