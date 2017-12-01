#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>


//在使用daemon函数的程序中，可在最初调用打开文件函数或创建socket函数前，使用如下函数，确保0，1，2三个fd不会被用于标准输出、输入、错误外的其他用途。

static inline void sanitize_fds(void)
{
    int zero;
    if ((zero = open("/dev/null", O_RDWR, 0)) < 0) return;
    while (zero < 3) zero = dup(zero);
    close(zero);
}


int daemon_allen( int nochdir,  int noclose )
{
   pid_t pid;
   if ( !nochdir && chdir("/") != 0 ) //如果nochdir=0,那么改变到"/"根目录
       return -1;
   
   if ( !noclose ) //如果没有noclose标志
   {
        int fd = open("/dev/null", O_RDWR); 
        if ( fd  <  0 )
            return -1;

    	//重定向标准输入、输出、错误到/dev/null，键盘的输入将对进程无任何影响，进程的输出也不会输出到终端
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);     
		close(fd);
	}

   pid = fork();  //创建子进程.
   if (pid  <  0)  //失败
      return -1;
   if (pid > 0)
       _exit(0); //返回执行的是父进程,那么父进程退出,让子进程变成真正的孤儿进程.

	//创建的 daemon子进程执行到这里了
   if ( setsid()  < 0 )   //创建新的会话，并使得子进程成为新会话的领头进程
      return -1;

   return 0;  //成功创建daemon子进程
}

int main(void)
{
    int fd1, fd2, fd3;
    close(0);
    close(1);
    close(2);
    fd1 = open("/tmp/tmp", O_RDWR, 0);
    fd2 = open("/tmp/tmp", O_RDWR, 0);
    fd3 = open("/tmp/tmp", O_RDWR, 0);

    syslog(LOG_USER, "Allenhe: %d,%d,%d\n", fd1, fd2, fd3);

    //printf("%d,%d,%d\n", fd1, fd2, fd3);

    daemon(0, 1);
    sleep(100);

    return 0;
}

/*

linux提供了daemon函数用于创建守护进程，实现原理如下：

#include <unistd.h>

int daemon(int nochdir, int noclose);

1． daemon()函数主要用于希望脱离控制台，以守护进程形式在后台运行的程序。

2． 当nochdir为0时，daemon将更改进城的根目录为root(“/”)。否则（非0）时保持当前执行目录不变。

3． 当noclose为0是，daemon将进城的STDIN, STDOUT, STDERR都重定向到/dev/null。否则保持原有标准输入(0)，标准输出(1)，标准错误(2)不变。


无论noclose 是否为0，daemon函数都不会关闭之前打开的大于等于3的fd。但是如果noclose值为0，需要确保0，1，2三个fd没有用于打开其他文件。下面一段程序就有问题，

int main(void)
{
    int fd1, fd2, fd3;
    close(0);
    close(1);
    close(2);
    fd1 = open("/tmp/tmp", O_RDWR, 0);
    fd2 = open("/tmp/tmp", O_RDWR, 0);
    fd3 = open("/tmp/tmp", O_RDWR, 0);
    daemon(0, 1);
    sleep(100);
}
用户将0，1，2三个fd用于打开文件而不是标准输入(0)，标准输出(1)，标准错误(2)，但是daemon函数依旧会将0，1，2三个fd重定向到/dev/null(可通过lsof命令查看打开文件情况)，
所有后面的daemon进程并不能通过0，1，2三个fd访问到文件。
*/