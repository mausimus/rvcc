#define NULL 0

typedef int FILE; /* TODO: struct */

void abort();

int syscall(int no, ...)
{
	/* no into a7, then all params from a0 */
	_asm(0x00050893); /*             addi           a7, a0, 0*/
	_asm(0x00058513); /*             addi           a0, a1, 0*/
	_asm(0x00060593); /*             addi           a1, a2, 0*/
	_asm(0x00068613); /*             addi           a2, a3, 0*/
	_asm(0x00070693); /*             addi           a3, a4, 0*/
	_asm(0x00078713); /*             addi           a4, a5, 0*/
	_asm(0x00080793); /*             addi           a5, a6, 0*/
	_asm(0x00000073); /*             ecall*/
	return;
}

int strlen(char *str)
{
	int i = 0;

	while (str[i] != 0)
		i++;

	return i;
}

int strcmp(char *s1, char *s2)
{
	int i = 0;

	while (s1[i] != 0 && s2[i] != 0) {
		if (s1[i] < s2[i])
			return -1;
		else if (s1[i] > s2[i])
			return 1;
		i++;
	}
	return s2[i] - s1[i];
}

int strncmp(char *s1, char *s2, int len)
{
	int i = 0;
	while (i < len) {
		if (s1[i] < s2[i])
			return -1;
		else if (s1[i] > s2[i])
			return 1;
		i++;
	}
	return 0;
}

char *strcpy(char *dest, char *src)
{
	int i = 0;
	while (src[i] != 0) {
		dest[i] = src[i];
		i++;
	}
	dest[i] = 0;
	return dest;
}

char *strncpy(char *dest, char *src, int len)
{
	int i = 0;
	int beyond = 0;

	while (i < len) {
		if (beyond == 0) {
			dest[i] = src[i];
			if (src[i] == 0) {
				beyond = 1;
			}
		} else {
			dest[i] = 0;
		}
		i++;
	}
	return dest;
}

/* set 10 digits (32bit) without div */
void _render10(char *pb, int val)
{
	int neg = 0;

	if (val < 0) {
		neg = 1;
		val = -val;
	}

	while (val >= 1000000000) {
		val -= 1000000000;
		pb[0]++;
	}
	while (val >= 100000000) {
		val -= 100000000;
		pb[1]++;
	}
	while (val >= 10000000) {
		val -= 10000000;
		pb[2]++;
	}
	while (val >= 1000000) {
		val -= 1000000;
		pb[3]++;
	}
	while (val >= 100000) {
		val -= 100000;
		pb[4]++;
	}
	while (val >= 10000) {
		val -= 10000;
		pb[5]++;
	}
	while (val >= 1000) {
		val -= 1000;
		pb[6]++;
	}
	while (val >= 100) {
		val -= 100;
		pb[7]++;
	}
	while (val >= 10) {
		val -= 10;
		pb[8]++;
	}
	while (val >= 1) {
		val -= 1;
		pb[9]++;
	}

	if (neg == 1) {
		int c = 0;
		while (pb[c] == '0') {
			c++;
		}
		if (c > 0) {
			pb[c - 1] = '-';
		}
	}
}

void _render16(char *pb, int val)
{
	int c = 9;
	while (c > 0) {
		int v = val & 0xf;
		if (v < 10) {
			pb[c] = '0' + v;
		} else if (v < 16) {
			pb[c] = 'a' + v - 10;
		} else {
			abort();
		}
		val = val >> 4;
		c--;
	}
}

int _format(char *buffer, int val, int width, int zeropad, int base, int hexprefix)
{
	int bi = 0;
	char pb[10];
	int pbi = 0;

	if (hexprefix == 1) {
		buffer[0] = '0';
		buffer[1] = 'x';
		bi = 2;
		if (width > 2) {
			width -= 2;
		} else {
			width = 0;
		}
	}

	/* set to zeroes */
	while (pbi < 10) {
		pb[pbi] = '0';
		pbi++;
	}

	if (base == 10) {
		_render10(pb, val);
	} else if (base == 16) {
		_render16(pb, val);
	} else {
		abort();
	}

	while (width > 10) {
		/* need to add extra padding */
		if (zeropad == 1) {
			buffer[bi] = '0';
		} else {
			buffer[bi] = ' ';
		}
		bi++;
		width--;
	}

	/* no padding */
	if (width == 0) {
		int c = 0;
		int started = 0;

		/* output from first digit */
		while (c < 10) {
			if (pb[c] != '0') {
				started = 1;
			}
			if (started) {
				buffer[bi] = pb[c];
				bi++;
			}
			c++;
		}
		/* special case - zero */
		if (started == 0) {
			buffer[bi] = '0';
			bi++;
		}
	} else {
		/* padding */
		int c = 10 - width;
		int started = 0;
		while (c < 10) {
			if (pb[c] != '0') {
				started = 1;
			}
			if (started) {
				buffer[bi] = pb[c];
			} else if (zeropad == 1) {
				buffer[bi] = '0';
			} else {
				buffer[bi] = ' ';
			}
			bi++;
			c++;
		}
	}
	return bi;
}

void printf(char *str, ...)
{
	int *var_args = &str - 4;
	char buffer[200];
	int si = 0, bi = 0, pi = 0;

	while (str[si] != 0) {
		if (str[si] != '%') {
			buffer[bi] = str[si];
			bi++;
			si++;
		} else {
			int w = 0, zp = 0, pp = 0;

			si++;
			if (str[si] == '#') {
				pp = 1;
				si++;
			}
			if (str[si] == '0') {
				zp = 1;
				si++;
			}
			if (str[si] >= '1' && str[si] <= '9') {
				w = str[si] - '0';
				si++;
				if (str[si] >= '0' && str[si] <= '9') {
					w = w * 10;
					w += str[si] - '0';
					si++;
				}
			}
			if (str[si] == 's') {
				/* append param pi as string */
				int l = strlen(var_args[pi]);
				strcpy(buffer + bi, var_args[pi]);
				bi += l;
			} else if (str[si] == 'd') {
				/* append param as decimal */
				int v = var_args[pi];
				bi += _format(buffer + bi, v, w, zp, 10, 0);
			} else if (str[si] == 'x') {
				/* append param as hex */
				int v = var_args[pi];
				bi += _format(buffer + bi, v, w, zp, 16, pp);
			}
			pi--;
			si++;
		}
	}
	buffer[bi] = 0;
	syscall(64, 0, buffer, bi);
}

char *memcpy(char *dest, char *src, int count)
{
	if (count > 0)
		do {
			count--;
			dest[count] = src[count];
		} while (count > 0);
	return dest;
}

void exit(int exit_code)
{
	syscall(93, exit_code);
}

void abort()
{
	printf("Abnormal program termination\n");
	/* trigger SEGV - must be a better way */
	int *z = 0;
	z[0] = 0;
	exit(-1);
}

FILE *fopen(char *filename, char *mode)
{
	if (strcmp(mode, "wb") == 0) {
		return syscall(1024, filename, 65, 0x1c0);
	} else if (strcmp(mode, "rb") == 0) {
		return syscall(1024, filename, 0, 0);
	}
	abort();
}

int fclose(FILE *stream)
{
	syscall(57, stream);
	return 0;
}

int fgetc(FILE *stream)
{
	char buf;
	int r = syscall(63, stream, &buf, 1);
	if (r <= 0) {
		return -1;
	}
	return buf;
}

char *fgets(char *str, int n, FILE *stream)
{
	int i = 0;
	char c;
	do {
		c = fgetc(stream);
		if (c == -1) {
			if (i == 0) {
				/* EOF on first char */
				return NULL;
			} else {
				/* EOF in the middle */
				str[i] = 0;
				return str;
			}
		}
		str[i] = c;
		i++;
	} while (str[i - 1] != '\n');
	str[i] = 0;
	return str;
}

int fputc(int c, FILE *stream)
{
	char buf[1];
	buf[0] = c;
	syscall(64, stream, &buf, 1);
	return 0;
}

int fputs(char *str, FILE *stream)
{
	int i = 0;
	while (str[i] != 0) {
		fputc(str[i], stream);
		i++;
	}
	return i;
}

void *malloc(int size)
{
	int brk = syscall(214, 0); /* read current break */
	syscall(214, brk + size); /* increment */
	return brk; /* return previous location, now extended by size */
}
