#include <stdio.h>

#define MAKESTR(x)  (#x)

#define doit(x) print(#x)

void print(char* name)
{
	printf("%s\n", name);
}

int main()
{
	int a = 123456;
	char* p = MAKESTR(a);
	printf("%s\n", p);
	doit(123456);
	return 0;
}
