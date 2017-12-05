#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>

#define BUFFER_SIZE 512

int setnonblocking( int fd )
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}

//./connect 127.0.0.1 8888
int main( int argc, char* argv[] )
{
    if( argc < 3 )
    {
        printf( "usage: %s ip_address port_number send_bufer_size\n", basename( argv[0] ) );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );

    struct sockaddr_in server_address;
    bzero( &server_address, sizeof( server_address ) );
    server_address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &server_address.sin_addr );
    server_address.sin_port = htons( port );

    int sock = socket( PF_INET, SOCK_STREAM, 0 );
    assert( sock >= 0 );
	
	const char* clientlocalip = "127.0.0.1";
	int clientlocalport = 6666;
	struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, clientlocalip, &address.sin_addr );
    address.sin_port = htons( clientlocalport );
	
//	int retcode = bind( sock, ( struct sockaddr* )&address, sizeof( address ) );
//    assert( retcode != -1 );
//    printf("AFTER bind...\n");

	/*
	//Linux从2.4开始支持接收缓冲和发送缓冲的动态调整。不用手动调整。
	int defaultsize = 0;
    int sendbuf = 32 * 1024; //32k
	int changedsize = 0;
    int len = sizeof( sendbuf );
    getsockopt( sock, SOL_SOCKET, SO_SNDBUF, &defaultsize, ( socklen_t* )&len );
    printf( "default tcp send buffer size before setting is %d\n", defaultsize );
	printf("set the sendbuffer size to %d\n", sendbuf);
	setsockopt( sock, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof( int ) );
	getsockopt( sock, SOL_SOCKET, SO_SNDBUF, &changedsize, ( socklen_t* )&len );
	printf( "after setting tcp send buffer size is %d\n", changedsize );
*/
    int old_option = fcntl( sock, F_GETFL );
    printf("noblock: %d\n", old_option & O_NONBLOCK); //0-->block mode

    //int oldopt = setnonblocking(sock); set nonblock mode!

    struct  timeval tv1, tv2;
    gettimeofday(&tv1, NULL);

	int ret = connect( sock, ( struct sockaddr* )&server_address, sizeof( server_address ) );
	printf("connect ret code is: %d\n", ret);
    if (  ret == 0 )
    {
        //
        printf("connect successful! call getsockname ...\n");
        struct sockaddr_in local_address;
        bzero(&local_address, sizeof(local_address));
        //MUST init length to right size the first time to call getpeername or getsockname !!!!
        socklen_t length = sizeof(local_address);
        int ret = getsockname(sock, ( struct sockaddr* )&local_address, &length);
       // assert(ret == 0);
        char local[INET_ADDRSTRLEN ];
        printf( "client's local with ip: %s and port: %d\n",
            inet_ntop( AF_INET, &local_address.sin_addr, local, INET_ADDRSTRLEN ), ntohs( local_address.sin_port ) );
        //
        bzero(&local_address, sizeof(local_address));
        ret = getpeername(sock, ( struct sockaddr* )&local_address, &length);
        char peer[INET_ADDRSTRLEN ];
        printf( "server's ip: %s and port: %d\n",
            inet_ntop( AF_INET, &local_address.sin_addr, peer, INET_ADDRSTRLEN ), ntohs( local_address.sin_port ) );

        //
        char buffer[ BUFFER_SIZE ];
        memset( buffer, '##############', BUFFER_SIZE );
		
		//send data
		if (send(sock, buffer, BUFFER_SIZE, 0) < 0)
		{
			perror("client send to srv failed.");
			exit(-1);
		}
		
		//recv data
		if (recv(sock, buffer, BUFFER_SIZE, 0) < 0)
		{
			perror("recv srv data failed.\n");
			exit(-1);
		}
		printf("recv from srv: %s\n", buffer);
		
        //send( sock, buffer, BUFFER_SIZE, 0 );
    }
    else if (ret == -1)
    {
    	gettimeofday(&tv2, NULL);
    	suseconds_t msec = tv2.tv_usec - tv1.tv_usec;
    	time_t sec = tv2.tv_sec - tv1.tv_sec;
    	printf("time used:%d.%fs\n", sec, (double)msec / 1000000 );

    	printf("connect failed...\n");
    	if (errno == EINPROGRESS)
    	{
    		printf("unblock mode ret code...\n");
    	}
    }
    else
    {
    	printf("ret code is: %d\n", ret);
    }

    printf("after connected, send...!\n");

    close( sock );
    return 0;
}
