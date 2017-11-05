#include <stdio.h>

int main()
{
	const int max_name = 81;
	char name[max_name];

	char fmt[10];
	sprintf(fmt, "%%%ds", max_name - 1);
	scanf(fmt, name);
	printf("%s\n", name);

	return 0;
}