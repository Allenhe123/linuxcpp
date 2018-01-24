#include<iostream>  
#include<stdlib.h>  
#include<stdio.h>  
#include<string.h>  
#include<unistd.h>  
#include<sys/socket.h>  
#include<netinet/in.h>  
#include<arpa/inet.h>  
#include<sys/select.h>  
using namespace std;

#define BUFSZ 1024
#define PORT  8888
#define SERVER_IP "127.0.0.1"

int main(int argc, char *argv[])
{
    struct sockaddr_in svr_addr, cli_addr;
    int ret;
	int sock;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    char buf[BUFSZ] = {};

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
	
	//如果服务器程序就绪后一上来就要发送数据给客户端，那么服务器就需要知道客户端的地址信息和端口，那么就不能让客户端的地址信息和端口号由客户端所在操作系统分配，
	//而是要在客户端程序指定了。怎么指定，那就是用bind()函数.
	//绑定client地址信息
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(9693);
    cli_addr.sin_addr.s_addr = 0;
    if ((ret = bind(sock, (struct sockaddr* )&cli_addr, addrlen)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    //sendto()函数需要指定目的端口/地址
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_port = htons(PORT);
    svr_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    while (1)
    {   
        memset(buf, 0, BUFSZ);
        printf("please input: ");
        fgets(buf, BUFSZ, stdin);
        sendto(sock, buf, BUFSZ, 0, (struct sockaddr* )&svr_addr, addrlen);

        ret = recvfrom(sock, buf, BUFSZ, 0, (struct sockaddr* )&svr_addr, &addrlen);
        printf("client: IPAddr = %s, Port = %d, recvfrom-buf = %s\n", inet_ntoa(svr_addr.sin_addr), ntohs(svr_addr.sin_port), buf);  
    }

    close(sock);  
    return 0;
}

//connect()函数可以用来指明套接字的目的地址/端口号，那么若udp服务器可以使用connect，将导致服务器只接受这特定一个主机的请求。

