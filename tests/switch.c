char e_extract_byte(int v, int b)
{
	switch (b) {
	case 0:
		return v & 0xFF;
	case 1:
		return (v >> 8) & 0xFF;
	case 2:
		return (v >> 16) & 0xFF;
	case 3:
		return (v >> 24) & 0xFF;
	default:
		abort();
	}
}

int main(int argc, char *argv[])
{
	/*
	printf("Begin with %d\n", argc);
	switch (argc) {
	case 1:
		printf("1\n");
		break;
	default:
		printf(">3\n");
		break;
	case 2:
	case 3:
		printf("2 or 3\n");
		break;
	}
	printf("End\n");*/
}
