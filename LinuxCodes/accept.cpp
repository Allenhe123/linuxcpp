#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define BUFFSIZE 512

// ./accept 127.0.0.1 8888
int main( int argc, char* argv[] )
{
    if( argc <= 2 )
    {
        printf( "usage: %s ip_address port_number\n", basename( argv[0] ) );
        return 1;
    }
	
	char buf[BUFFSIZE];
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
    
	while (1)
	{
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
			printf( "connected cliet's ip: %s and port: %d\n", 
				inet_ntop( AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN ), ntohs( client.sin_port ) );
        
			//
			printf("call getsockname ...\n");
			struct sockaddr_in local_address;
			
			//MUST init length to right size the first time to call getpeername or getsockname !!!!
			socklen_t length = sizeof(local_address);
			int ret = 0;

			bzero( &local_address, sizeof( local_address ) );
			ret = getpeername(connfd, ( struct sockaddr* )&local_address, &length);
			if (ret == 0)
			{
				char local1[INET_ADDRSTRLEN ];
				printf( "(client's ip)remote ip: %s and port: %d\n", inet_ntop( AF_INET, &local_address.sin_addr, local1, INET_ADDRSTRLEN ), ntohs( local_address.sin_port ) );

			}
			else
			{
				printf("getpeername on connfd fail...retcode: %d, errno: %d\n", ret, errno);
				perror("getpeername(): ");
			}

			bzero( &local_address, sizeof( local_address ) );
			ret = getsockname(connfd, ( struct sockaddr* )&local_address, &length);
			if (ret == 0)
			{
				char locall[INET_ADDRSTRLEN ];
				printf( "session server side's local connfd ip: %s and port: %d\n", inet_ntop( AF_INET, &local_address.sin_addr, locall, INET_ADDRSTRLEN ), ntohs( local_address.sin_port ) );
			}
			else
			{
				printf("getsockname on connfd fail...retcode: %d, errno: %d\n", ret, errno);
				perror("getsockname(): ");
			}
		
			//recv data 
			if (recv(connfd, buf, BUFFSIZE, 0) < 0)
			{
				perror("recv client data failed.\n");
				exit(-1);
			}
			printf("recv from client: %s\n", buf);
		
			//send data
			if (send(connfd, "server has received your msg\n", BUFFSIZE, 0) < 0)
			{
				perror("srv send to client failed.");
				exit(-1);
			}

			close( connfd );
		}
	}


    close( sock );
    return 0;
}

