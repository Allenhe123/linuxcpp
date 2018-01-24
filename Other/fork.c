#include <sys/wait.h>

int main()
{
	printf("A\n");

	pid_t pid;
	pid = fork();
	if (pid == 0)
	{
		printf("B\n");
	}
	else
	{
		printf("C\n");
	}
	printf("D\n");
	return 0;
}
