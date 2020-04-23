#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>

int main(void)
{
    int epfd,nfds;
    struct epoll_event ev,events[5];                    //ev用于注册事件，数组用于返回要处理的事件
    epfd = epoll_create(1);                                //只需要监听一个描述符——标准输入
    ev.data.fd = STDIN_FILENO;
    ev.events = EPOLLIN;                                //监听读状态同时设置LT模式
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);    //注册epoll事件
    for(;;)
    {
        nfds = epoll_wait(epfd, events, 5, -1);
        for(int i = 0; i < nfds; i++)
        {
            if(events[i].data.fd==STDIN_FILENO)
            {
                char buf[1024] = {0};
                read(STDIN_FILENO, buf, sizeof(buf));
                printf("welcome to epoll's word!\n");
            }            
        }
    }
}

/*
  本程序依然使用LT模式，但是每次epoll_wait返回读就绪的时候我们都将buffer（缓冲）中的内容read出来，
  所以导致buffer再次清空，下次调用epoll_wait就会阻塞。所以能够实现我们所想要的功能——当用户从控制台
  有任何输入操作时，输出”welcome to epoll's world！”
*/