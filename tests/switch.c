int main(int argc, char *argv[])
{
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
	printf("End\n");
}
