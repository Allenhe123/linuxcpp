#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>

int main(void)
{
    int epfd,nfds;
    struct epoll_event ev,events[5];                    //ev用于注册事件，数组用于返回要处理的事件
    epfd = epoll_create(1);                                //只需要监听一个描述符——标准输入
    ev.data.fd = STDOUT_FILENO;
    ev.events = EPOLLOUT|EPOLLET;                        //监听读状态同时设置ET模式
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDOUT_FILENO, &ev);    //注册epoll事件
    for(;;)
    {
        nfds = epoll_wait(epfd, events, 5, -1);
        for(int i = 0; i < nfds; i++)
        {
            if(events[i].data.fd==STDOUT_FILENO)
            {
                printf("welcome to epoll's word!\n");
            }            
        }
    }
}

/*
这个程序的功能是只要标准输出写就绪，就输出“welcome to epoll's world”。
我们发现这将是一个死循环。下面具体分析一下这个程序的执行过程：

1.首先初始buffer为空，buffer中有空间可写，这时无论是ET还是LT都会将对应的epitem加入rdlist，
   导致epoll_wait就返回写就绪。
2.程序想标准输出输出”welcome to epoll's world”和换行符，因为标准输出为控制台的时候缓冲是“行缓冲”,
   所以换行符导致buffer中的内容清空，这就对应第二节中ET模式下写就绪的第二种情况——当有旧数据被发送走时，
   即buffer中待写的内容变少得时候会触发fd状态的改变。所以下次epoll_wait会返回写就绪。如此循环往复。
*/