#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

int main( int argc, char *argv[] )
{
	//assert( argc == 2 );
	//char *host = argv[1];

	struct hostent *host;   //存放主机信息
	//char addr_p[NET_ADDR_STR_LEN]; //用于存放点分十进制IP地址的字符串
    if((host = gethostent()) == NULL)
    {
        perror("fail to get host's information\n");
        return -1;
    }
    printf("hostName: %s\n" , host->h_name);

	//用域名或主机名获取IP地址
	struct hostent* hostinfo = gethostbyname( host->h_name );
	assert( hostinfo );
	struct servent* servinfo = getservbyname( "daytime", "tcp" );
	assert( servinfo );
	printf( "daytime port is %d\n", ntohs( servinfo->s_port ) );

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = servinfo->s_port;
	address.sin_addr = *( struct in_addr* )*hostinfo->h_addr_list;

	char remote[INET_ADDRSTRLEN ];
	printf( "connected with ip: %s and port: %d\n", 
            inet_ntop( AF_INET, &address.sin_addr, remote, INET_ADDRSTRLEN ), ntohs( address.sin_port ) );

	int sockfd = socket( AF_INET, SOCK_STREAM, 0 );
	//int reuse = 1;
    //setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse ) );
	int result = connect( sockfd, (struct sockaddr* )&address, sizeof( address ) );
	printf( "errno is: %d\n", errno );
	perror("connect error:");
	assert( result != -1 );

	char buffer[128];
	result = read( sockfd, buffer, sizeof( buffer ) );
	assert( result > 0 );
	buffer[ result ] = '\0';
	printf( "the day tiem is: %s", buffer );
	close( sockfd );
    return 0;
}