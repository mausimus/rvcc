/* rvcc C compiler - misc helpers */

void str_trim(char *s)
{
	int i = 0;
	while (s[i] != 0)
		i++;
	i--;
	while (i >= 0 && s[i] == ' ') {
		s[i] = 0;
		i--;
	}
}

void str_copy(char *src, char *dst, int len)
{
	int i;
	for (i = 0; i < len; i++)
		dst[i] = src[i];
	dst[len] = 0;
}

int is_whitespace(char c)
{
	if (c == ' ' || c == '\r' || c == '\n' || c == '\t')
		return 1;
	return 0;
}

int is_alnum(char c)
{
	if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '_'))
		return 1;
	return 0;
}

int is_digit(char c)
{
	if (c >= '0' && c <= '9')
		return 1;
	return 0;
}

int is_hex(char c)
{
	if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || c == 'x' || (c >= 'A' && c <= 'F'))
		return 1;
	return 0;
}
