/*
#include<stdio.h>
#include<signal.h>
#include<unistd.h>

int flag_sigusr1 = 0;
int flag_sigusr2 = 0;

void sig_usr1(int signo)
{
    fprintf(stdout, "caught SIGUSR1\n");
    flag_sigusr1 = 1;
    return;
}

void sig_usr2(int signo)
{
    fprintf(stdout, "caught SIGUSR2\n");
    flag_sigusr2 = 1;
    return;
}

int main(void){
    sigset_t newmask, oldmask;
    signal(SIGUSR1, sig_usr1);
    signal(SIGUSR2, sig_usr2);

    fprintf(stdout, "catch sigusr1 can break\n");
	
    while(1)
	{
        if(flag_sigusr1)
		{
            fprintf(stdout, "break\n");
            break;
        }
        sleep(5);
    }
	fprintf(stdout, "first while was broken\n");
	
	//重新设置为0
    flag_sigusr1 = 0;
	flag_sigusr2 = 0;

    // block SIGUSR1
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGUSR1);
    if(sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
	{
        perror("sigprocmask error");
    }

    fprintf(stdout, "only catch sigusr2 can break, because sigusr1 has been blocked\n");
    while(1)
	{
        if(flag_sigusr1 || flag_sigusr2)
		{
            fprintf(stdout, "break\n");
            break;
        }
        sleep(5);
    }
	fprintf(stdout, "second while was broken\n");
	
	fprintf(stdout, "after second while was broken, flag_sigusr1=%d, flag_sigusr2=%d\n", flag_sigusr1, flag_sigusr2);

    return 0;
}

*/

#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include<pthread.h>

int flag_sigusr1 = 0;
int flag_sigusr2 = 0;
int flag_sighup  = 0;

void sig_usr1(int signo)
{
    fprintf(stdout, "sig|caught SIGUSR1\n");
    flag_sigusr1 = 1;
    return;
}

void sig_usr2(int signo)
{
    fprintf(stdout, "sig|caught SIGUSR2\n");
    flag_sigusr2 = 1;
    return;
}

void sig_hup(int signo)
{
    fprintf(stdout, "sig|caught SIGHUP\n");
    flag_sighup = 1;
    return;
}

void *thread_control_signal(void *arg)
{
    sigset_t newmask, oldmask;
    sigemptyset(&newmask);

    //thread block sighup
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGHUP);
    if(pthread_sigmask(SIG_BLOCK, &newmask, &oldmask) < 0)
	{ 
        perror("sigprocmask error"); 
    } 

    fprintf(stdout, "thread|first while. catch sigusr1 or sigusr2 can break\n");
    while(1)
	{
        if(flag_sigusr1 || flag_sigusr2)
		{
            fprintf(stdout, "thread|break\n");
            break;
        }
        sleep(5);
    }
    flag_sigusr1 = 0;

    //thread block SIGUSR1
    sigaddset(&newmask, SIGUSR1);
    if(pthread_sigmask(SIG_BLOCK, &newmask, &oldmask) < 0)
	{
        perror("sigprocmask error");
    }

    fprintf(stdout, "thread|first while. catch sigusr2 can break\n");
    while(1)
	{
        if(flag_sigusr1 || flag_sigusr2)
		{
            fprintf(stdout, "break\n");
            break;
        }
        sleep(10);
    }
    fprintf(stdout, "thread|thread exit\n");
    return (void *)0;
}

int main()
{
    sigset_t    newmask;
    pthread_t  tid;
    int        signo;

    //signal action
    signal(SIGUSR1, sig_usr1);
    signal(SIGUSR2, sig_usr2);
    signal(SIGHUP , sig_hup);

    if(pthread_create(&tid, NULL, thread_control_signal, NULL) < 0)
	{
        perror("create pthread failed");
        return -1;
    }

    //main thread block sigusr1
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGUSR1);
    if(pthread_sigmask(SIG_BLOCK, &newmask, NULL) < 0)
	{
        perror("sigprocmask error");
    }

    //main thread wait sighup
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGHUP);
    if(sigwait(&newmask, &signo) < 0)
	{
        perror("sigwait failed");
        return -1;
    }
    fprintf(stdout, "main|get SIGHUP\n");

    pthread_kill(tid, SIGUSR2);
    pthread_kill(tid, SIGUSR2);
    pthread_join(tid, NULL);

    fprintf(stdout, "main|exit\n");
    return 0;
}

