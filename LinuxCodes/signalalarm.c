#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void alarmhandler(int sig) {
	printf("recv sig %d\n", sig);
	printf("exit!\n");
	exit(0);
}

int main() {
	//signal()函数是个系统调用，该函数按signum设定一个新的信号处理句柄（函数）.新设定的处理函数可以是用户自定义的函数，也可以是系统指定的SIG_IGN 或 SIG_DFL
	signal(SIGALRM, alarmhandler);
	printf("begin...\n");
	alarm(5); // 定时器，用于在5秒后产生SIGALRM信号
	while(1);

	return 0;
}
