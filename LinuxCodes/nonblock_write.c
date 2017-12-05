#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

char buf[500000];

void set_fl(int fd, int flags) {
	int val;
	if ((val = fcntl(fd, F_GETFL, 0)) < 0) {
		printf("call fcntl error\n");
	}

	val |= flags;
	if (fcntl(fd, F_SETFL, 0) < 0) {
		printf("call fcntl error\n");	
	}
}

void clr_fl(int fd, int flags) {
	int val = fcntl(fd, F_GETFL, 0);
	if (val < 0) {
		printf("call fcntl error\n");
	}

	val &= ~flags;
	if (fcntl(fd, F_SETFL, 0) < 0) {
		printf("call fcntl error\n");	
	}
}

int main() {
	int ntowrite, nwrite;
	char *ptr;

	ntowrite = read(STDIN_FILENO, buf, sizeof(buf));
	fprintf(stderr, "read %d bytes\n", ntowrite);

	set_fl(STDOUT_FILENO, O_NONBLOCK);

	ptr = buf;
	while (ntowrite > 0) {
		errno = 0;
		nwrite = write(STDOUT_FILENO, ptr, ntowrite);
		fprintf(stderr, "nwrite=%d, errno=%d\n", nwrite, errno);
		if (nwrite > 0) {
			ptr += nwrite;
			ntowrite -= nwrite;
		}
	}

	clr_fl(STDOUT_FILENO, O_NONBLOCK);

	exit(0);

	return 0;
}
