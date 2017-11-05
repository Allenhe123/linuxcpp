#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

// ./accept 127.0.0.1 8888
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

    int reuse = 1;
    setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );

    int ret = bind( sock, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret != -1 );
    printf("AFTER bind...\n");

    ret = listen( sock, 5 );
    assert( ret != -1 );
    printf("AFTER listen...\n");
    
    //the returned client is client's address
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof( client );
    int connfd = accept( sock, ( struct sockaddr* )&client, &client_addrlength ); //accept is a block function
    printf("AFTER accept...\n");
    
    if ( connfd < 0 )
    {
        printf( "errno is: %d\n", errno );
    }
    else
    {
        //#define INET_ADDRSTRLEN 16 , IPV4 address char array length, <netinet/in.h>
        char remote[INET_ADDRSTRLEN ];
        printf( "connected with ip: %s and port: %d\n", 
            inet_ntop( AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN ), ntohs( client.sin_port ) );
        
        //
        printf("call getsockname ...\n");
        struct sockaddr_in local_address;
        socklen_t length;
        int ret = getsockname(connfd, ( struct sockaddr* )&local_address, &length);
        if (ret == 0)
        {
            char local[INET_ADDRSTRLEN ];
            printf( "session server side's local connfd ip: %s and port: %d\n", inet_ntop( AF_INET, &local_address.sin_addr, local, INET_ADDRSTRLEN ), ntohs( local_address.sin_port ) );
        }
        else
            printf("getsockname on connfd fail...\n");

        bzero( &local_address, sizeof( local_address ) );
        ret = getpeername(connfd, ( struct sockaddr* )&local_address, &length);
        if (ret == 0)
        {
            char local1[INET_ADDRSTRLEN ];
            printf( "(client's ip)remote ip: %s and port: %d\n", inet_ntop( AF_INET, &local_address.sin_addr, local1, INET_ADDRSTRLEN ), ntohs( local_address.sin_port ) );

        }
        else
            printf("getpeername on connfd fail...\n");

        close( connfd );
    }

    close( sock );
    return 0;
}

