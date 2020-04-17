#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>

#define IPADDRESS   "127.0.0.1"
#define PORT        8787
#define MAXSIZE     1024
#define LISTENQ     5
#define FDSIZE      1000
#define EPOLLEVENTS 100

//创建套接字并进行绑定
static int socket_bind(const char* ip,int port);
//IO多路复用epoll
static void do_epoll(int listenfd);
//事件处理函数
static void handle_events(int epollfd,struct epoll_event *events,int num,int listenfd,char *buf);
//处理接收到的连接
static void handle_accpet(int epollfd,int listenfd);
//读处理
static void do_read(int epollfd,int fd,char *buf);
//写处理
static void do_write(int epollfd,int fd,char *buf);
//添加事件
static void add_event(int epollfd,int fd,int event);
//修改事件
static void modify_event(int epollfd,int fd,int event);
//删除事件
static void delete_event(int epollfd,int fd,int event);

int main(int argc, char *argv[])
{
    int  listenfd;
    listenfd = socket_bind(IPADDRESS, PORT);
    if (listen(listenfd, LISTENQ) == -1) {
        perror("listen error");
        return -1;
    }
    do_epoll(listenfd);
    return 0;
}

static int socket_bind(const char* ip,int port)
{
    int  listenfd;
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    if (listenfd == -1)
    {
        perror("socket error:");
        exit(1);
    }

    int optval = 0;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)& optval, sizeof(int)) < 0) {
        perror("setsocketopt for SO_REUSEADDR failed:");
    }

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&servaddr.sin_addr);
    servaddr.sin_port = htons(port);
    if (bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        perror("bind error: ");
        exit(1);
    }
    return listenfd;
}

static void do_epoll(int listenfd)
{
    int epollfd;
    struct epoll_event events[EPOLLEVENTS];
    int ret;
    char buf[MAXSIZE];
    memset(buf,0,MAXSIZE);
    // 创建一个描述符，其实是申请一个内核空间，用来存放你想关注的socket fd上是否发生以及发生了什么事件。
    // size就是你在这个Epoll fd上能关注的最大socket fd数，大小自定，只要内存足够。
    epollfd = epoll_create(FDSIZE);
    //添加监听描述符事件
    add_event(epollfd,listenfd,EPOLLIN);
    for (;;)
    {
        //获取已经准备好的描述符事件
        ret = epoll_wait(epollfd, events, EPOLLEVENTS, -1);
        if (ret == -1) {
            perror("epoll_wait failed.");
            continue;
        }
        handle_events(epollfd, events, ret, listenfd, buf);
    }
    close(epollfd);
}

static void handle_events(int epollfd, struct epoll_event *events, int num, int listenfd, char *buf)
{
    //进行选好遍历 根据描述符的类型和事件类型进行处理
    for (int i = 0;i < num;i++)
    {
        int fd = events[i].data.fd;
        if ((fd == listenfd) &&(events[i].events & EPOLLIN))
            handle_accpet(epollfd, listenfd);
        else if (events[i].events & EPOLLIN)
            do_read(epollfd, fd, buf);
        else if (events[i].events & EPOLLOUT)
            do_write(epollfd, fd, buf);
    }
}
static void handle_accpet(int epollfd,int listenfd)
{
    printf("listenfd: %d\n");
    int clifd;
    struct sockaddr_in client_addr;
    socklen_t  client_addrlen;
    clifd = accept(listenfd, (struct sockaddr*)&client_addr, &client_addrlen);
    if (clifd == -1) {
        perror("accpet error:");
        

    }
    else
    {
        printf("accept a new client: %s:%d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
        //添加一个客户描述符和事件
        add_event(epollfd, clifd, EPOLLIN);
    }
}

static void do_read(int epollfd, int fd, char *buf)
{
    int nread;
    nread = read(fd, buf, MAXSIZE);
    if (nread == -1)
    {
        perror("read error would close this session:");
        close(fd);
        delete_event(epollfd, fd, EPOLLIN);
    }
    else if (nread == 0)
    {
        fprintf(stderr, "client close.\n");
        delete_event(epollfd, fd, EPOLLIN);
        close(fd);
    }
    else
    {
        printf("read: %s",buf);
        //修改描述符对应的事件，由读改为写(为何每次都要修改???)
        modify_event(epollfd, fd, EPOLLOUT);
    }
}

static void do_write(int epollfd,int fd,char *buf)
{
    int nwrite = write(fd,buf,strlen(buf));
    if (nwrite == -1)
    {
        perror("write error would close this session:");
        close(fd);
        delete_event(epollfd, fd, EPOLLOUT);
    }
    else
        modify_event(epollfd, fd, EPOLLIN);
    memset(buf, 0, MAXSIZE);
}

static void add_event(int epollfd,int fd,int event)
{
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev) == -1) {
        perror("epoll_ctl add event failed.");
    }
}

static void delete_event(int epollfd,int fd,int event)
{
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,&ev) == -1) {
        perror("epoll_ctl delete event failed.");
    }
}

static void modify_event(int epollfd,int fd,int event)
{
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    if (epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev)  == -1) {
        perror("epoll_ctl modify event failed.");
    }
}
