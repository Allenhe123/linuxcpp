/*
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

void handler(int signo)
{
    switch(signo) {
    case SIGUSR1: //处理信号 SIGUSR1
        printf("Parent : catch SIGUSR1\n");
		break;
    case SIGUSR2: //处理信号 SIGUSR2
        printf("Child : catch SIGUSR2\n");
		break;
    default:      //本例不支持
        printf("Should not be here\n");
        break;
    }
}

int main(void)
{
    pid_t ppid, cpid;
    //为两个信号设置信号处理函数
    if(signal(SIGUSR1, handler) == SIG_ERR) 
	{ //设置出错
        perror("Can't set handler for SIGUSR1\n");
        exit(1);
    }

    if(signal(SIGUSR2, handler) == SIG_ERR) 
	{ //设置出错
        perror("Can't set handler for SIGUSR2\n");
        exit(1);
    }

    ppid = getpid();//得到父进程ID

    if((cpid = fork()) < 0) 
	{
        perror("fail to fork\n");
        exit(1);
    } 
	else if(cpid == 0) 
	{
		// 子进程内向父进程发送信号SIGUSER1
        if(kill(ppid, SIGUSR1) == -1) 
		{
            perror("fail to send signal\n");
            exit(1);
        }

        while(1);//死循环，等待父进程的信号
    } 
	else 
	{
        sleep(1);//休眠，保证子进程先运行，并且发送SIGUSR1信号
		// 父进程向自己发送SIGUSER2信号
        if(kill(cpid, SIGUSR2) == -1)
		{
            perror("fail to send signal\n");
            exit(1);
        }
		
		// 必须sleep一下，否则子进程捕获不到SIGUSER2信号
		sleep(1);

        printf("will kill child\n");//输出提示
        if(kill(cpid, SIGKILL) == -1) 
		{ //发送SIGKILL信号，杀死子进程
            perror("fail to send signal\n");
            exit(1);
        }

        if(wait(NULL) ==-1) 
		{ //回收子进程状态，避免僵尸进程
            perror("fail to wait\n");
            exit(1);
        }
		printf("child has been killed.\n");
    }
    return;
}

*/


#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static void sig_usr(int);
int main(void)
{
        if(signal(SIGUSR1, sig_usr) == SIG_ERR)
            printf("can not catch SIGUSR1\n");
        if(signal(SIGUSR2, sig_usr) == SIG_ERR)
            printf("can not catch SIGUSR2\n");
        for(;;)
                pause();
}

static void sig_usr(int signo)
{
        if(signo == SIGUSR1)
            printf("received SIGUSR1\n");
        else if(signo == SIGUSR2)
            printf("received SIGUSR2\n");
        else
            printf("received signal %d\n", signo);
}

