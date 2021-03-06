#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>
 
#define USER_LIMIT 5
#define BUFFER_SIZE 64
#define FD_LIMIT 65535
 
struct client_data
{
    sockaddr_in address;
    char* write_buf;
    char buf[ BUFFER_SIZE ];

    client_data(): write_buf(0) {}
};
 
int setnonblocking( int fd )
{
    int old_option = fcntl( fd, F_GETFL );
    int new_option = old_option | O_NONBLOCK;
    fcntl( fd, F_SETFL, new_option );
    return old_option;
}
 
int main( int argc, char* argv[] )
{
    if( argc <= 2 )
    {
        printf( "usage: %s ip_address port_number\n", basename( argv[0] ) );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );
 
    int ret = 0;
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );
 
    int listenfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert( listenfd >= 0 );
 
    ret = bind( listenfd, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret != -1 );
 
    ret = listen( listenfd, 5 ); //backlog=5
    assert( ret != -1 );
 
    client_data* users = new client_data[FD_LIMIT];
    pollfd fds[USER_LIMIT+1]; //1 listen sock fd + 5 user sock fd
    int user_counter = 0;
    for( int i = 1; i <= USER_LIMIT; ++i )
    {
        fds[i].fd = -1;
        fds[i].events = 0;
    }
    fds[0].fd = listenfd;
    fds[0].events = POLLIN | POLLERR; //read & error event
    fds[0].revents = 0;
 
    while( 1 )
    {
        ret = poll( fds, user_counter+1, -1 );
        if ( ret < 0 )
        {
            printf( "poll failure\n" );
            break;
        }
     
        for( int i = 0; i < user_counter+1; ++i )
        {
            //client call connect to this server
            if( ( fds[i].fd == listenfd ) && ( fds[i].revents & POLLIN ) )
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof( client_address );
                //listenfd is block mode by default, but use poll we know the event is ready, accept will return right now
                int connfd = accept( listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
                if ( connfd < 0 )
                {
                    printf( "accept errno is: %d\n", errno );
                    continue;
                }
 
                if( user_counter >= USER_LIMIT )
                {
                    const char* info = "too many users, so close this connfd returned by accept.\n";
                    printf( "%s", info );
                    send( connfd, info, strlen( info ), 0 );
                    close( connfd );
                    continue;
                }
                user_counter++;
                users[connfd].address = client_address;
                setnonblocking( connfd );  //connfd is nonblock mode 
                fds[user_counter].fd = connfd;
                fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;// concern about read & hanguo & error event at start
                fds[user_counter].revents = 0;
                printf( "comes a new user, now have %d users\n", user_counter );
            }
            else if( fds[i].revents & POLLERR ) //fds[i] has error!
            {
                printf( "get an error from %d\n", fds[i].fd );
                char errors[ 100 ];
                memset( errors, '\0', 100 );
                socklen_t length = sizeof( errors );
                if( getsockopt( fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &length ) < 0 )
                {
                    printf( "get socket option failed\n" );
                }
                else
                    printf("%d got an error %s\n", fds[i].fd, errors);
                continue;
            }
            else if( fds[i].revents & POLLRDHUP ) //fds[i] was hang up by peer
            {
                users[fds[i].fd] = users[fds[user_counter].fd];
                close( fds[i].fd );
                fds[i] = fds[user_counter];
                i--;
                user_counter--;
                printf( "a client left\n" );
            }
            //got readable event
            else if( fds[i].revents & POLLIN )
            {
                int connfd = fds[i].fd;
                memset( users[connfd].buf, '\0', BUFFER_SIZE );
                ret = recv( connfd, users[connfd].buf, BUFFER_SIZE-1, 0 );
                printf( "get %d bytes of client data: %s   from connection: %d\n", ret, users[connfd].buf, connfd );
                if( ret < 0 ) //has error
                {
                    //NONBLOCK socket, read函数会返回一个错误EAGAIN，提示你的应用程序现在没有数据可读请稍后再试。
                    //so if it is a EAGAIN error we just ignore this error and wait for next POLLIN event
                    if( errno != EAGAIN )
                    {
                        close( connfd );
                        users[fds[i].fd] = users[fds[user_counter].fd];
                        fds[i] = fds[user_counter];
                        i--;
                        user_counter--;
                        printf( "recv data from a client ran into error, a client maybe left\n" );
                    }
                }
                //If the connection has been gracefully closed, the return value is zero.
                else if( ret == 0 )
                {
                    //printf( "code should not come to here\n" );
                    printf("recv function return 0 indicates the client has gracefully closed and exit.\n");

                    //need to remove it from fds??

                }
                else //server send the data recv from a client to all other clients 
                {
                    for( int j = 1; j <= user_counter; ++j )
                    {
                        if( fds[j].fd == connfd )
                        {
                            continue; //don't send to self
                        }
                         
                        //其他的client的socket此刻不能再读了(当前client继续发多个消息的话，这些消息会阻塞在socket的sendbuffer)，
                        //要设置为只可以写！因为如果可以再读的话，假如这个client连续发多个消息，其他client就会漏收后面的消息！
                        fds[j].events |= ~POLLIN;
                        fds[j].events |= POLLOUT;
                        users[fds[j].fd].write_buf = users[connfd].buf;
                    }
                }
            }
            else if( fds[i].revents & POLLOUT ) //got writable event
            {
                int connfd = fds[i].fd;
                if( ! users[connfd].write_buf )
                {
                    continue; //no data send to this client
                }
                ret = send( connfd, users[connfd].write_buf, strlen( users[connfd].write_buf ), 0 );
                users[connfd].write_buf = NULL;
                //reset back
                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
        }
    }
 
    delete [] users;
    close( listenfd );
    return 0;
}
