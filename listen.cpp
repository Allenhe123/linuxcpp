#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool stop = false;
static void handle_term( int sig ) // kill pid;  in another tty will triggle this signal
{
    stop = true;
    printf("signal SIGTERM catched...\n");
}

static void handle_int(int sig)  // ctrl+c; will triggle this signal
{
    printf("signal SIGINT catched...\n");
    stop = true;   
}


//./listen 127.0.0.1 8888 100

int main( int argc, char* argv[] )
{
    signal( SIGTERM, handle_term );
    signal(SIGINT, handle_int);

    if( argc <= 3 )
    {
        printf( "usage: %s ip_address port_number backlog\n", basename( argv[0] ) );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );
    int backlog = atoi( argv[3] );

    int sock = socket( PF_INET, SOCK_STREAM, 0 );
    assert( sock >= 0 );

    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    int ret = bind( sock, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret != -1 );

    printf("after bind...\n");

    //backlog is the max number of waitting connect in wait queue
    ret = listen( sock, backlog );   //listen is a none-block function
    assert( ret != -1 );
    printf("after listen...\n");

    while ( ! stop )
    {
        sleep( 1 );
    }

    close( sock );
    return 0;
}
