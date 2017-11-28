#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main( int argc, char* argv[] )
{
    if( argc <= 2 )
    {
        printf( "usage: %s ip_address port_number\n", basename( argv[0] ) );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );

    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    int sock = socket( PF_INET, SOCK_STREAM, 0 );
    assert( sock >= 0 );

    int ret = bind( sock, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret != -1 );

    ret = listen( sock, 5 );
    assert( ret != -1 );

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof( client );
    int connfd = accept( sock, ( struct sockaddr* )&client, &client_addrlength );
    if ( connfd < 0 )
    {
        printf( "errno is: %d\n", errno );
    }
    else
    {
        close( STDOUT_FILENO );
        dup( connfd );
        printf( "abcd\n" );
        close( connfd );
    }

    close( sock );
    return 0;
}

/*
dup和dup2也是两个非常有用的调用，它们的作用都是用来复制一个文件的描述符。它们经常用来重定向进程的stdin、stdout和stderr。
输入重定向：关闭标准输入设备，打开（或复制）某普通文件，使其文件描述符为0.
输出重定向：关闭标准输出设备，打开（或复制）某普通文件，使其文件描述符为1.
错误输出重定向：关闭标准错误输入设备，打开（或复制）某普通文件，使其文件描述符为2.

由dup函数返回的新文件描述符一定是当前可用文件描述符中的最小值。
dup2用来复制参数oldfd所指的文件描述符，并将oldfd拷贝到参数newfd后一起返回。若参数newfd为一个打开的文件描述符，则newfd所指的文件会先被关闭，若newfd等于oldfd，则返回newfd,而不关闭newfd所指的文件。
*/

