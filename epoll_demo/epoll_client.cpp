#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#define MAXSIZE     1024
#define IPADDRESS   "127.0.0.1"
#define SERV_PORT   8787
#define FDSIZE      1024
#define EPOLLEVENTS 20

bool g_server_closed = false;

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

    printf("client exit.\n");
    return 0;
}

static void handle_connection(int sockfd)
{
    int epollfd;
    struct epoll_event events[EPOLLEVENTS];
    char buf[MAXSIZE];
    memset(buf, 0, MAXSIZE);
    int ret;
    epollfd = epoll_create(FDSIZE);
    // 监视标准输入的可读事件
    add_event(epollfd, STDIN_FILENO, EPOLLIN);
    for (;;)
    {
        if (g_server_closed) {
             break;
        }
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

static void do_read(int epollfd, int fd, int sockfd, char *buf)
{
    if (fd == STDIN_FILENO)  // from stdin
    {
        while (1)
        {
            int nread = read(fd, buf, MAXSIZE);
            if (nread == -1)
            {
                perror("read error would close this session:");
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    continue;
                else
                {
                    close(fd);
                    break;
                }
            }
            else if (nread == 0)
            {
                fprintf(stderr, "server close.\n");
                close(fd);
                break;
            }
            else
            {
                printf("read %d bytes from stdin:%s", nread, buf);
                add_event(epollfd, sockfd, EPOLLOUT);
                break;
            }
        }
    }
    else    // from socket
    {
        // read msg header
        int32_t header_len = sizeof(int32_t);
        char* header_buf = new char[header_len];
        char* header_buf_origin = header_buf;
        while (header_len > 0)
        {
            int nread = read(fd, buf, header_len);
            if (nread == -1)
            {
                perror("read error would close session:");
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    continue;
                else
                {
                    g_server_closed = true;
                    break;
                }
            }
            else if (nread == 0)
            {
                fprintf(stderr,"server close.\n");
                g_server_closed = true;
                break;
            }
            else
            {
                if (nread < header_len)
                {
                    memcpy(header_buf, buf, nread);
                    header_len -= nread;
                    header_buf += nread;
                }
                else
                {
                    memcpy(header_buf, buf, nread);
                    break;
                }
            }
        }

        // read msg data
        if (!g_server_closed)
        {
            int32_t len = *(int32_t*)header_buf_origin;
            char* pbuffer = new char[len];
            char* buffer_origin = pbuffer;
            int readed = 0;
            while (len > MAXSIZE)
            {
                readed = read(fd, buf, MAXSIZE);
                if (readed == 0)
                {
                    g_server_closed = true;
                    break;
                }
                else if (readed == -1)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        continue;
                    else
                    {
                        g_server_closed = true;
                        break;
                    }
                }

                memcpy(pbuffer, buf, readed);
                pbuffer += readed;
                len -= readed;
            }
            while (!g_server_closed)
            {
                readed = read(fd, buf, len);
                if (readed == 0)
                {
                    g_server_closed = true;
                    break;
                }
                else if (readed == -1)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                        continue;
                    else
                    {
                        g_server_closed = true;
                        break;
                    }
                }

                memcpy(pbuffer, buf, readed);
                pbuffer += readed;
                len  -= readed;
                if (len <= 0) break;
            }

            printf("client recv: %s\n", buffer_origin);
            delete []buffer_origin;

            //为何每次都要delete，add，或者说用modify来修改。 难道不可以在一个fd上面同时监视2读和写事件吗？
            delete_event(epollfd, sockfd, EPOLLIN);
            add_event(epollfd, STDOUT_FILENO, EPOLLOUT);
        }

        delete []header_buf_origin;
    }

    if (g_server_closed)
    {
        close(fd);
    }
}

static void do_write(int epollfd, int fd, int sockfd, char *buf)
{
     if (fd == STDOUT_FILENO)   // to stdout
     {
        printf("do write to stdout\n");
        int nwrite = write(fd, buf, strlen(buf));
        if (nwrite == -1)
        {
            perror("write error would close session:");
            close(fd);
        }
        delete_event(epollfd, fd, EPOLLOUT);
     }
     else    // to socket
     {
        int32_t msg_len = strlen(buf);
        char* buffer = new char[msg_len + sizeof(int32_t)];
        char* buffer_ori = buffer;
        memcpy(buffer, &msg_len , sizeof(int32_t));
        buffer += sizeof(int32_t);
        memcpy(buffer, buf, msg_len);

        int32_t len = msg_len + sizeof(int32_t);
        while (len > 0)
        {
            int nwrite = write(fd, buffer_ori, len);
            if (nwrite == -1)
            {
                perror("write error would close session:");
                close(fd);
                break;
            }
            len -= nwrite;
            buffer_ori += nwrite;
        }
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