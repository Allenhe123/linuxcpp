#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>

int main(void)
{
    int epfd,nfds;
    struct epoll_event ev,events[5];                    //ev用于注册事件，数组用于返回要处理的事件
    epfd = epoll_create(1);                                //只需要监听一个描述符——标准输入
    ev.data.fd = STDOUT_FILENO;
    ev.events = EPOLLOUT;                                //监听读状态同时设置LT模式
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDOUT_FILENO, &ev);    //注册epoll事件
    for(;;)
    {
        nfds = epoll_wait(epfd, events, 5, -1);
        for(int i = 0; i < nfds; i++)
        {
            if(events[i].data.fd==STDOUT_FILENO)
            {
                printf("welcome to epoll's word!");
            }            
        }
    }
}

/*
程序六相对程序五仅仅是修改ET模式为默认的LT模式，我们发现程序再次死循环。
这时候原因已经很清楚了，因为当向buffer写入”welcome to epoll's world！”后，
虽然buffer没有输出清空，但是LT模式下只有buffer有写空间就返回写就绪，所以会一直
输出”welcome to epoll's world！”,当buffer满的时候，buffer会自动刷清输出，
同样会造成epoll_wait返回写就绪。
*/