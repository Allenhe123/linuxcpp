#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>

int main(void)
{
    int epfd,nfds;
    struct epoll_event ev,events[5];                    //ev用于注册事件，数组用于返回要处理的事件
    epfd = epoll_create(1);                                //只需要监听一个描述符——标准输入
    ev.data.fd = STDIN_FILENO;
    ev.events = EPOLLIN|EPOLLET;                        //监听读状态同时设置ET模式
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);    //注册epoll事件
    for(;;)
    {
        nfds = epoll_wait(epfd, events, 5, -1);
        for(int i = 0; i < nfds; i++)
        {
            if(events[i].data.fd==STDIN_FILENO)
            {
                printf("welcome to epoll's word!\n");
                ev.data.fd = STDIN_FILENO;
                ev.events = EPOLLIN|EPOLLET;                          //设置ET模式
                // epoll_ctl(epfd, EPOLL_CTL_MOD, STDIN_FILENO, &ev);    //重置！重置epoll事件！重置后下次循环又可以被触发
                // epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);   //ADD不起任何左右，相当于这一行没有增加一样

                // 先DEL再ADD就相当于MOD，下次又可被触发
                epoll_ctl(epfd, EPOLL_CTL_DEL, STDIN_FILENO, &ev);
                epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
            }            
        }
    }
}

/*
epoll_ctl(epfd, EPOLL_CTL_MOD, STDIN_FILENO, &ev); -> 死循环
epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev); -> 正常

  程序依然使用ET，但是每次读就绪后都主动的再次MOD IN事件，我们发现程序再次出现死循环，
  也就是每次返回读就绪。但是注意，如果我们将MOD改为ADD，将不会产生任何影响。别忘了
  每次ADD一个描述符都会在epitem组成的红黑树中添加一个项，我们之前已经ADD过一次，再次ADD将阻止添加，
  所以在次调用ADD IN事件不会有任何影响。
*/
