#include <stdio.h>

#define MAXLINE 256

int main() 
{
	char line[MAXLINE];
	while (fgets(line, MAXLINE, stdin) != NULL)
	{
		printf("%s\n", line);
	}
	return 0;
}
