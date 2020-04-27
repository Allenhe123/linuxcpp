#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>

int main(void)
{
    int epfd,nfds;
    struct epoll_event ev, events[5];    // ev用于注册事件，数组用于返回要处理的事件
    epfd = epoll_create(1);              // 只需要监听一个描述符——标准输入
    ev.data.fd = STDIN_FILENO;
    ev.events = EPOLLIN;         // 默认LT模式
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev); // 注册epoll事件
    for(;;)
    {
        nfds = epoll_wait(epfd, events, 5, -1);
        for(int i = 0; i < nfds; i++)
        {
            if(events[i].data.fd==STDIN_FILENO)
            printf("welcome to epoll's word!\n");
        }
    }
}

/*
程序陷入死循环，因为用户输入任意数据后，数据被送入buffer且没有被读出，
所以LT模式下每次epoll_wait都认为buffer可读返回读就绪。
导致每次都会输出”welcome to epoll's world！”。
*/
