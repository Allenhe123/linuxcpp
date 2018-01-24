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
    struct sockaddr_in svr_addr;
    int ret;
	int sock;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    char buf[BUFSZ] = {};

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
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

