#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>

int main(void)
{
    int epfd,nfds;
    struct epoll_event ev, events[5];    // ev用于注册事件，数组用于返回要处理的事件
    epfd = epoll_create(1);              // 只需要监听一个描述符——标准输入
    ev.data.fd = STDIN_FILENO;
    ev.events = EPOLLIN|EPOLLET;         // 监听读状态同时设置ET模式
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
1.当用户输入一组字符，这组字符被送入buffer，字符停留在buffer中，又因为buffer由空变为不空，
所以ET返回读就绪，输出”welcome to epoll's world！”。
2.之后程序再次执行epoll_wait，此时虽然buffer中有内容可读，但是根据我们上节的分析，ET并不返回就绪，导致epoll_wait阻塞。（底层原因是ET下就绪fd的epitem只被放入rdlist一次）。
3.用户再次输入一组字符，导致buffer中的内容增多，根据我们上节的分析这将导致fd状态的改变，是对应的epitem再次加入rdlist，从而使epoll_wait返回读就绪，再次输出“Welcome to epoll's world！”。

*/
