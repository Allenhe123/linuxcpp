#include <stdio.h>
#include <unistd.h>

int main()
{
	char buf[1024];
	int ret = gethostname(buf, sizeof(buf));
	printf("%s\n", buf);
	return 0;
}
