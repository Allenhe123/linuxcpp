#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <unistd.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <assert.h>
  
int main(int argc, char *argv[])  
{  
    int sockfd_one;  
    int err_log;  
    sockfd_one = socket(AF_INET, SOCK_STREAM, 0); //创建UDP套接字one  
    if(sockfd_one < 0)  
    {  
    perror("sockfd_one");  
    exit(-1);  
    }  
  
    // 设置本地网络信息  
    struct sockaddr_in my_addr;  
    bzero(&my_addr, sizeof(my_addr));  
    my_addr.sin_family = AF_INET;  
    my_addr.sin_port = htons(8000);     // 端口为8000  
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
  
    int reuse = 1;
    setsockopt( sockfd_one, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );
    // 绑定，端口为8000  
    err_log = bind(sockfd_one, (struct sockaddr*)&my_addr, sizeof(my_addr));  
    if(err_log != 0)  
    {  
        perror("bind sockfd_one");  
        close(sockfd_one);        
        exit(-1);  
    }  
    printf("socket 1 bind success\n");

    int ret = listen( sockfd_one, 5 );
    assert( ret != -1 );
    printf("AFTER listen...\n");

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof( client );
    int connfd = accept( sockfd_one, ( struct sockaddr* )&client, &client_addrlength ); //accept is a block function
    printf("AFTER accept...\n");
    
    if ( connfd < 0 )
    {
        perror("accept failed!");  
    }
    else
    {
        printf( "accept success! %d\n" );
    }
  
    int sockfd_two;  
    sockfd_two = socket(AF_INET, SOCK_DGRAM, 0);  //创建UDP套接字two  
    if(sockfd_two < 0)  
    {  
        perror("sockfd_two");  
        exit(-1);  
    }  
  
    // 新套接字sockfd_two，继续绑定8000端口，绑定失败  
    // 因为8000端口已被占用，默认情况下，端口没有释放，无法绑定  
    int reuse1 = 1;
    setsockopt( sockfd_two, SOL_SOCKET, SO_REUSEADDR, &reuse1, sizeof( reuse1 ) );
    err_log = bind(sockfd_two, (struct sockaddr*)&my_addr, sizeof(my_addr));  
    if(err_log != 0)  
    {  
        perror("bind sockfd_two");  
        close(sockfd_two);        
        exit(-1);  
    }  
  
   printf("socket 2 bind success\n");
   close(sockfd_one);  
   close(sockfd_two);  
  
    return 0;  
}  