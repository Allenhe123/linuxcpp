#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define MAXSIZE     1024
#define IPADDRESS   "127.0.0.1"
#define SERV_PORT   8787
#define FDSIZE      1024
#define EPOLLEVENTS 20

static void handle_connection(int sockfd);
static void handle_events(int epollfd,struct epoll_event *events,int num,int sockfd,char *buf);
static void do_read(int epollfd,int fd,int sockfd,char *buf);
static void do_read(int epollfd,int fd,int sockfd,char *buf);
static void do_write(int epollfd,int fd,int sockfd,char *buf);
static void add_event(int epollfd,int fd,int event);
static void delete_event(int epollfd,int fd,int event);
static void modify_event(int epollfd,int fd,int event);

int main(int argc,char *argv[])
{
    int                 sockfd;
    struct sockaddr_in  servaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, IPADDRESS, &servaddr.sin_addr);
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        perror("connect error: ");
        return -1;
    }
    //处理连接
    handle_connection(sockfd);
    close(sockfd);
    return 0;
}

static void handle_connection(int sockfd)
{
    int epollfd;
    struct epoll_event events[EPOLLEVENTS];
    char buf[MAXSIZE];
    int ret;
    epollfd = epoll_create(FDSIZE);
    // 监视标准输入的可读事件
    add_event(epollfd, STDIN_FILENO, EPOLLIN);
    for (;;)
    {
        ret = epoll_wait(epollfd, events, EPOLLEVENTS, -1);
        if (ret == -1) {
            perror("epoll_wait failed.");
            continue;
        }
        handle_events(epollfd, events, ret, sockfd, buf);
    }
    close(epollfd);
}

static void handle_events(int epollfd,struct epoll_event *events,int num,int sockfd,char *buf)
{
    for (int i = 0;i < num;i++)
    {
        int fd = events[i].data.fd;
        if (events[i].events & EPOLLIN)
            do_read(epollfd, fd, sockfd, buf);
        else if (events[i].events & EPOLLOUT)
            do_write(epollfd, fd, sockfd, buf);
    }
}

static void do_read(int epollfd,int fd,int sockfd,char *buf)
{
    int nread = read(fd, buf, MAXSIZE);
    if (nread == -1)
    {
        perror("read error would close session:");
        close(fd);
    }
    else if (nread == 0)
    {
        fprintf(stderr,"server close.\n");
        close(fd);
    }
    else
    {
        if (fd == STDIN_FILENO)  // 读取的是来自键盘的输入
            add_event(epollfd, sockfd, EPOLLOUT);
        else                     // 读取的是对端数据
        {
            printf("client recv: %s\n", buf);

            //为何每次都要delete，add，或者说用modify来修改。 难道不可以在一个fd上面同时监视2读和写事件吗？
            delete_event(epollfd, sockfd, EPOLLIN);
            add_event(epollfd, STDOUT_FILENO, EPOLLOUT);
        }
    }
}

static void do_write(int epollfd, int fd, int sockfd, char *buf)
{
    int nwrite = write(fd, buf, strlen(buf));
    if (nwrite == -1)
    {
        perror("write error would close session:");
        close(fd);
    }
    else
    {
        // STDOUT_FILENO： 向屏幕输出
        // STDIN_FILENO： 接收键盘输入
        if (fd == STDOUT_FILENO)
            delete_event(epollfd, fd, EPOLLOUT);
        else
            modify_event(epollfd, fd, EPOLLIN);
    }
    memset(buf, 0, MAXSIZE);
}

static void add_event(int epollfd, int fd, int event)
{
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
}

static void delete_event(int epollfd,int fd,int event)
{
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &ev);
}

static void modify_event(int epollfd,int fd,int event)
{
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}