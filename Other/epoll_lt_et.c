/*
LT(level trigger)：此行为被 epoll 默认支持，不必设置。在 epoll_wait 得到一个事件时，如果应用程序不处理此事件，在 level trigger 模式下，epoll_wait 会持续触发此事件，直到事件被程序处理；这种模式编程出错误可能性要小一点。
传统的select/poll都是这种模型的代表．

ET(edge trigger)：在 edge trigger 模式下，事件只会被 epoll_wait 触发一次，如果用户不处理此事件，不会在下次 epoll_wait 再次触发。在处理得当的情况下，此模式无疑是高效的。需要注意的是此模式需求 socket 处理非阻塞模式，下面会实现此模式。
但是请注意，如果一直不对这个fd作IO操作(从而导致它再次变成未就绪)，内核不会发送更多的通知

ET模式在很大程度上减少了epoll事件被重复触发的次数，因此效率要比LT模式高。epoll工作在ET模式的时候，必须使用非阻塞套接口，以避免由于一个文件句柄的阻塞读/阻塞写操作把处理多个文件描述符的任务饿死。

I/O 多路复用：简单的说就是由一个进程来管理多个 socket，即将多个 socket 放入一个
表中，在其中有 socket 可操作时，通知进程来处理， I/O 多路复用的实现方式有 select、poll 和 epoll。


1.对于监听的sockfd，最好使用水平触发模式，边缘触发模式会导致高并发情况下，有的客户端会连接不上。如果非要使用边缘触发，网上有的方案是用while来循环accept()。

2.对于读写的connfd，水平触发模式下，阻塞和非阻塞效果都一样，不过为了防止特殊情况，还是建议设置非阻塞。

3.对于读写的connfd，边缘触发模式下，必须使用非阻塞IO，并要一次性全部读写完数据。
-------------------------------------------------------------------------------------------------------------------
在许多测试中我们会看到如果没有大量的idle -connection或者dead-connection，epoll的效率并
不会比select/poll高很多，但是当我们遇到大量的idle-connection(例如WAN环境中存在大量的慢速连接)，就会发现epoll的效率大大高于select/poll。


当使用epoll的ET模型来工作时，当产生了一个EPOLLIN事件后，读数据的时候需要考虑的是当recv()返回的大小如果等于请求的大小，
那么很有可能是缓冲区还有数据未读完，也意味着该次事件还没有处理完，所以还需要再次读取：
while(rs)
{
  buflen = recv(activeevents[i].data.fd, buf, sizeof(buf), 0);
  if(buflen < 0)
  {
    // 由于是非阻塞的模式,所以当errno为EAGAIN时,表示当前缓冲区已无数据可读
    // 在这里就当作是该次事件已处理处.
    if(errno == EAGAIN)
     break;
    else
     return;
   }
   else if(buflen == 0)
   {
     // 这里表示对端的socket已正常关闭.
   }
   if(buflen == sizeof(buf)
     rs = 1;   // 需要再次读取
   else
     rs = 0;
}



假如发送端流量大于接收端的流量(意思是epoll所在的程序读比转发的socket要快),由于是非阻塞的socket,那么send()函数虽然返回,但实际缓冲区的数据并未真正发给接收端,这样不断的读和发，当缓冲区满后会产生EAGAIN错误(参考man send),同时,不理会这次请求发送的数据.所以,需要封装socket_send()的函数用来处理这种情况,该函数会尽量将数据写完再返回，返回-1表示出错。在socket_send()内部,当写缓冲已满(send()返回-1,且errno为EAGAIN),那么会等待后再重试.这种方式并不很完美,在理论上可能会长时间的阻塞在socket_send()内部,但暂没有更好的办法.

ssize_t socket_send(int sockfd, const char* buffer, size_t buflen)
{
  ssize_t tmp;
  size_t total = buflen;
  const char *p = buffer;

  while(1)
  {
    tmp = send(sockfd, p, total, 0);
    if(tmp < 0)
    {
      // 当send收到信号时,可以继续写,但这里返回-1.
      if(errno == EINTR)
        return -1;

      // 当socket是非阻塞时,如返回此错误,表示写缓冲队列已满,
      // 在这里做延时后再重试.
      if(errno == EAGAIN)
      {
        usleep(1000);
        continue;
      }

      return -1;
    }

    if((size_t)tmp == total)
      return buflen;

    total -= tmp;
    p += tmp;
  }

  return tmp;
}

*/


//1. 一个终端运行./epoll_test 127.0.0.1 7788 &
//2. 另一个终端运行5个：nc 127.0.0.1 7788 &
//3. jobs -l查看所有后台nc任务。fg %task_number将某个调到前台，然后可以输入数据发送。 完成后Ctrl+Z使其暂停并进入后台，此时这个是暂停的，让他继续运行：bg %task_number。

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>


/* 最大缓存区大小 */
#define MAX_BUFFER_SIZE 5
/* epoll最大监听数 */
#define MAX_EPOLL_EVENTS 20
/* LT模式 水平触发*/
#define EPOLL_LT 0
/* ET模式 边缘触发 */
#define EPOLL_ET 1
/* 文件描述符设置阻塞 */
#define FD_BLOCK 0
/* 文件描述符设置非阻塞 */
#define FD_NONBLOCK 1

/* 设置文件为非阻塞 */
int set_nonblock(int fd)
{
	int old_flags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, old_flags | O_NONBLOCK);
	return old_flags;
}

/* 注册文件描述符到epoll，并设置其事件为EPOLLIN(可读事件) */
void addfd_to_epoll(int epoll_fd, int fd, int epoll_type, int block_type)
{
	struct epoll_event ep_event;
	ep_event.data.fd = fd;
	ep_event.events = EPOLLIN;

	/* 如果是ET模式，设置EPOLLET */
	if (epoll_type == EPOLL_ET)      //EPOLL_LT是默认的行为，不必设置
	    ep_event.events |= EPOLLET;

	/* 设置是否阻塞 */
	if (block_type == FD_NONBLOCK)
	set_nonblock(fd);

	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ep_event);
}

/* LT处理流程 */
void epoll_lt(int sockfd)
{
	char buffer[MAX_BUFFER_SIZE];
	int ret;

	memset(buffer, 0, MAX_BUFFER_SIZE);
	printf("开始recv()...\n");
	ret = recv(sockfd, buffer, MAX_BUFFER_SIZE, 0);
	printf("ret = %d\n", ret);
	if (ret > 0)
	   printf("收到消息:%s, 共%d个字节\n", buffer, ret);
	else
	{
	    if (ret == 0)
	       printf("客户端主动关闭！！！\n");
		close(sockfd);															    
    }
	printf("LT处理结束！！！\n");
}

/* 带循环的ET处理流程 */
void epoll_et_loop(int sockfd)
{
    char buffer[MAX_BUFFER_SIZE];
	int ret;

	printf("带循环的ET读取数据开始...\n");
	while (1)
	{
	    memset(buffer, 0, MAX_BUFFER_SIZE);
		ret = recv(sockfd, buffer, MAX_BUFFER_SIZE, 0);
		if (ret == -1)
		{
		    if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
			    printf("循环读完所有数据！！！\n");
			    break;
			}
			close(sockfd);
			break;
		}
		else if (ret == 0)
		{
		    printf("客户端主动关闭请求！！！\n");
			close(sockfd);
			break;
		}
		else
		    printf("收到消息:%s, 共%d个字节\n", buffer, ret);
	}
	printf("带循环的ET处理结束！！！\n");
}


/* 不带循环的ET处理流程，比epoll_et_loop少了一个while循环 */
void epoll_et_nonloop(int sockfd)
{
	char buffer[MAX_BUFFER_SIZE];
	int ret;

	printf("不带循环的ET模式开始读取数据...\n");
	memset(buffer, 0, MAX_BUFFER_SIZE);
	ret = recv(sockfd, buffer, MAX_BUFFER_SIZE, 0);
	if (ret > 0)
	{
	    printf("收到消息:%s, 共%d个字节\n", buffer, ret);
	}
	else
	{
	    if (ret == 0)
		    printf("客户端主动关闭连接！！！\n");
		close(sockfd);
	}
    printf("不带循环的ET模式处理结束！！！\n");
}

/* 处理epoll的返回结果 */
void epoll_process(int epollfd, struct epoll_event *events, int number, int sockfd, int epoll_type, int block_type)
{
	struct sockaddr_in client_addr;
	socklen_t client_addrlen;
	int newfd, connfd;
	int i;

	for (i = 0; i < number; i++)
	{
	    newfd = events[i].data.fd;
	    if (newfd == sockfd && (events[i].events & EPOLLIN))   //handle_accept
		{
		    printf("=================================新一轮accept()===================================\n");
		    printf("accept()开始...\n");

		    /* 休眠3秒，模拟一个繁忙的服务器，不能立即处理accept连接 */
			printf("开始休眠3秒...\n");
			sleep(3);
			printf("休眠3秒结束！！！\n");

			client_addrlen = sizeof(client_addr);
			connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addrlen);
            if (connfd == -1)
            {
                perror("accept error");
                continue;
            }
            
            addfd_to_epoll(epollfd, connfd, EPOLLIN, FD_NONBLOCK);   //将通信socket新加入到epoll
		}
		else if (events[i].events & EPOLLIN)    // handle_read
		{
		    /* 可读事件处理流程 */
		    if (epoll_type == EPOLL_LT)    
			{
				printf("============================>水平触发开始...\n");
				epoll_lt(newfd);
			}
			else if (epoll_type == EPOLL_ET)
			{
				printf("============================>边缘触发开始...\n");

				/* 带循环的ET模式 */
				epoll_et_loop(newfd);

				/* 不带循环的ET模式 */
				//epoll_et_nonloop(newfd);
			}
		}
        else if (events[i].events & EPOLLOUT)  //handle_write
        {
            printf("============================>handle_write...\n");
            
        }
		else
			printf("其他事件发生...\n");
	 }
}

/* 出错处理 */
void err_exit(char *msg)
{
	perror(msg);
	exit(1);
}

/* 创建socket */
int create_socket(const char *ip, const int port_number)
{
	struct sockaddr_in server_addr;
	int sockfd, reuse = 1;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_number);

	if (inet_pton(PF_INET, ip, &server_addr.sin_addr) == -1)
	err_exit("inet_pton() error");

	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	    err_exit("socket() error");

	/* 设置复用socket地址 */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
	    err_exit("setsockopt() error");

	if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	    err_exit("bind() error");

	if (listen(sockfd, 5) == -1)
	    err_exit("listen() error");

	return sockfd;
}

/* main函数 */
int main(int argc, const char *argv[])
{
	if (argc < 3)
	{
	    fprintf(stderr, "usage:%s ip_address port_number\n", argv[0]);
		exit(1);
	}

	int sockfd, epollfd, number;

    //创建listen监听套接字sockfd
	sockfd = create_socket(argv[1], atoi(argv[2]));
	struct epoll_event events[MAX_EPOLL_EVENTS];

	/* linux内核2.6.27版的新函数，和epoll_create(int size)一样的功能，并去掉了无用的size参数 */
	if ((epollfd = epoll_create1(0)) == -1)
	err_exit("epoll_create1() error");

	/* 以下设置是针对监听的sockfd，当epoll_wait返回时，必定有事件发生，
	* 所以这里我们忽略罕见的情况外设置阻塞IO没意义，我们设置为非阻塞IO */

	/* sockfd：非阻塞的LT模式 */
	addfd_to_epoll(epollfd, sockfd, EPOLL_LT, FD_NONBLOCK);

	/* sockfd：非阻塞的ET模式 */
	//addfd_to_epoll(epollfd, sockfd, EPOLL_ET, FD_NONBLOCK);

							   
	while (1)
	{
	    number = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, -1);
		if (number == -1)
		    err_exit("epoll_wait() error");
		else
		{
		    /* 以下的LT，ET，以及是否阻塞都是是针对accept()函数返回的文件描述符，即函数里面的connfd */

			/* connfd:阻塞的LT模式 */
			epoll_process(epollfd, events, number, sockfd, EPOLL_LT, FD_BLOCK);
           
           /* connfd:非阻塞的LT模式 */
		   //epoll_process(epollfd, events, number, sockfd, EPOLL_LT, FD_NONBLOCK);

			/* connfd:阻塞的ET模式 */
			//epoll_process(epollfd, events, number, sockfd, EPOLL_ET, FD_BLOCK);

			/* connfd:非阻塞的ET模式 */
			//epoll_process(epollfd, events, number, sockfd, EPOLL_ET, FD_NONBLOCK);
		}
	}

    close(epollfd);
	close(sockfd);
	return 0;
}
