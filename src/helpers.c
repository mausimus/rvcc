/* rvcc C compiler - misc helpers */

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
