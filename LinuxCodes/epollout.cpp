#include <sys/socket.h>  
#include <sys/epoll.h>  
#include <unistd.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <stdlib.h>  
#include <fcntl.h>  
#include <string.h>  
#include <stdio.h>  
  
#define SERV_PORT  10000  
#define MAX_EVENTS 10  
#define LEN 2  
  
int main(int argc, char **argv)  
{  
      
    if (argc != 2) {  
        perror("usage:exe port\n");  
        return -1;  
    }  
  
    struct sockaddr_in servaddr;  
    int sockfd;  
    memset(&servaddr, 0x00, sizeof(servaddr));  
    servaddr.sin_family = AF_INET;  
    servaddr.sin_port = htons(atoi(argv[1]));  
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  
      
    sockfd = socket(AF_INET, SOCK_STREAM, 0);  
    if (sockfd < 0) {  
        perror("socket error");  
        exit(-1);  
    }  
  
    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {  
        perror("bind error");  
        close(sockfd);  
        exit(-1);  
    }  
  
    if (listen(sockfd, 3) == -1) {  
        perror("listen error");  
        close(sockfd);  
        exit(-1);  
    }  
  
    int epollfd = epoll_create(MAX_EVENTS);  
    if (epollfd == -1) {  
        perror("epoll_create error");  
        close(sockfd);  
        exit(-1);  
    }  
  
    struct epoll_event ev, events[MAX_EVENTS];  
    ev.events = EPOLLIN;  
    ev.data.fd = sockfd;  
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev) == -1) {  
        perror("epoll_ctl error");  
        exit(-1);  
    }  
  
    int nfds, connfd;  
  
    for (;;) 
	{  
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);  
        if (nfds == -1) 
		{  
            perror("epoll_wait error");  
            exit(-1);  
        }  
        int i;        
        for (i = 0; i < nfds; i++) 
		{  
            if (events[i].data.fd == sockfd)
			{  
                connfd = accept(sockfd, NULL, NULL);  
                printf("connfd=%d\n", connfd);  
                if (connfd == -1) 
				{  
                    perror("accept error");  
                    exit(-1);  
                }     
                  
                int flag;  
                flag = fcntl(connfd, F_GETFL, 0);  
                fcntl(connfd, F_SETFL, flag & ~O_NONBLOCK);           
                ev.events = EPOLLOUT;  
                ev.data.fd = connfd;  
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1) 
				{  
                    perror("epoll_ctl error");  
                    exit(-1);  
                }  
            } 
			else 
			{  
                /* 
                char buf[LEN]; 
                int n = read(events[i].data.fd, buf, LEN); 
                buf[n] = 0; 
                printf("buf=%s\n", buf); 
                */ 
				// 每一次循环发送缓冲区都是空的，所以一直是可以写的。 EPOLLOUT一直触发。
                if (events[i].events & EPOLLOUT) 
				{  
                    printf("get EPOLLOUT event, fd=%d\n", events[i].data.fd);  
                }  
            }  
        }  
    }  
}  
