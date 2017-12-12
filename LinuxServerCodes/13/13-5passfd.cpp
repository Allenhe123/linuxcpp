#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
`
////////////////////////////// 进程间传递描述符/////////////////////////////////////////////
/*
发送进程调用 sendmsg 向通道发送一个特殊的消息，内核将对这个消息做特殊处理，从而将打开的描述符传递到接收进程。
然后接收方调用 recvmsg 从通道接收消息，从而得到打开的描述符。
1 需要注意的是传递描述符并不是传递一个 int 型的描述符编号，而是在接收进程中创建一个新的描述符，并且在内核的文件表中，它与发送进程发送的描述符指向相同的项。
2 在进程之间可以传递任意类型的描述符，比如可以是 pipe ， open ， mkfifo 或 socket ， accept 等函数返回的描述符，而不限于套接字。
3 一个描述符在传递过程中（从调用 sendmsg 发送到调用 recvmsg 接收），内核会将其标记为“在飞行中”（ in flight ）。在这段时间内，即使发送方试图关闭该描述符，内核仍会为接收进程保持打开状态。发送描述符会使其引用计数加 1 。
4 描述符是通过辅助数据发送的（结构体 msghdr 的 msg_control 成员），在发送和接收描述符时，总是发送至少 1 个字节的数据，即使这个数据没有任何实际意义。否则当接收返回 0 时，接收方将不能区分这意味着“没有数据”（但辅助数据可能有套接字）还是“文件结束符”。
5 具体实现时， msghdr 的 msg_control 缓冲区必须与 cmghdr 结构对齐，可以看到后面代码的实现使用了一个 union 结构来保证这一点。


struct msghdr {  
    void       *msg_name;  
	socklen_t    msg_namelen;  
	struct iovec  *msg_iov;  
	size_t       msg_iovlen;  
	void       *msg_control;  
	size_t       msg_controllen;  
	int          msg_flags;  
};

1 套接口地址成员 msg_name 与 msg_namelen ；
只有当通道是数据报套接口时才需要； msg_name 指向要发送或是接收信息的套接口地址。 msg_namelen 指明了这个套接口地址的长度。
msg_name 在调用 recvmsg 时指向接收地址，在调用 sendmsg 时指向目的地址。注意， msg_name 定义为一个 (void *) 数据类型，因此并不需要将套接口地址显示转换为 (struct sockaddr *) 。
2 I/O 向量引用 msg_iov 与 msg_iovlen
它是实际的数据缓冲区，从下面的代码能看到，我们的 1 个字节就交给了它；这个 msg_iovlen 是 msg_iov 的个数，不是什么长度。
msg_iov 成员指向一个 struct iovec 数组， iovc 结构体在 sys/uio.h 头文件定义

struct iovec {  
     ptr_t iov_base; // Starting address   
     size_t iov_len; // Length in bytes  
	 }; 

	 有了 iovec ，就可以使用 readv 和 writev 函数在一次函数调用中读取或是写入多个缓冲区，显然比多次 read ， write 更有效率。

	 3 附属数据缓冲区成员 msg_control 与 msg_controllen ，描述符就是通过它发送的，后面将会看到， msg_control 指向附属数据缓冲区，而 msg_controllen 指明了缓冲区大小。
	 4 接收信息标记位 msg_flags ；忽略

轮到 cmsghdr 结构了，附属信息可以包括若干个单独的附属数据对象。在每一个对象之前都有一个 struct cmsghdr 结构。头部之后是填充字节，然后是对象本身。最后，附属数据对象之后，下一个 cmsghdr 之前也许要有更多的填充字节。
struct cmsghdr {  
    socklen_t cmsg_len;  
	int       cmsg_level;  
	int       cmsg_type;  
	// u_char     cmsg_data[];  
};  

cmsg_len   附属数据的字节数，这包含结构头的尺寸，这个值是由 CMSG_LEN() 宏计算的；
cmsg_level  表明了原始的协议级别 ( 例如， SOL_SOCKET) ；
cmsg_type  表明了控制信息类型 ( 例如， SCM_RIGHTS ，附属数据对象是文件描述符； SCM_CREDENTIALS ，附属数据对象是一个包含证书信息的结构 ) ；
被注释的 cmsg_data 用来指明实际的附属数据的位置，帮助理解。
对于 cmsg_level 和 cmsg_type ，当下我们只关心 SOL_SOCKET 和 SCM_RIGHTS 。

CMSG_LEN() 宏
输入参数：附属数据缓冲区中的对象大小；
计算 cmsghdr 头结构加上附属数据大小，包括必要的对其字段，这个值用来设置 cmsghdr 对象的 cmsg_len 成员。

CMSG_DATA() 宏
输入参数：指向 cmsghdr 结构的指针 ;
返回跟随在头部以及填充字节之后的附属数据的第一个字节 ( 如果存在 ) 的地址，比如传递描述符时.

函数 sendmsg 和 recvmsg

函数原型如下：
[cpp] view plain copy
#include <sys/types.h>  
#include <sys/socket.h>  
int sendmsg(int s, const struct msghdr *msg, unsigned int flags);  
int recvmsg(int s, struct msghdr *msg, unsigned int flags);  
二者的参数说明如下：
s， 套接字通道，对于 sendmsg 是发送套接字，对于 recvmsg 则对应于接收套接字；
msg ，信息头结构指针；
flags ， 可选的标记位， 这与 send 或是 sendto 函数调用的标记相同。
函数的返回值为实际发送 / 接收的字节数。否则返回 -1 表明发生了错误。
具体参考 APUE 的高级 I/O 部分，介绍的很详细。
*/

// CMSG_LEN()返回cmsghdr结构的cmsg_len成员的值，考虑到任何必要的对齐。它取出参数的长度。这是一个常量表达式。
static const int CONTROL_LEN = CMSG_LEN( sizeof(int) );

void send_fd( int fd, int fd_to_send )
{
    struct iovec iov[1];
    struct msghdr msg;
    char buf[0];

    iov[0].iov_base = buf;
    iov[0].iov_len = 1;
    msg.msg_name    = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov     = iov;
    msg.msg_iovlen = 1;

    cmsghdr cm;
    cm.cmsg_len = CONTROL_LEN;  // data byte count, including header
    cm.cmsg_level = SOL_SOCKET; // originating protocal
    cm.cmsg_type = SCM_RIGHTS;  // protocol-specific type
    *(int *)CMSG_DATA( &cm ) = fd_to_send;
    msg.msg_control = &cm;
    msg.msg_controllen = CONTROL_LEN;

    sendmsg( fd, &msg, 0 );
}

int recv_fd( int fd )
{
    struct iovec iov[1];
    struct msghdr msg;
    char buf[0];

    iov[0].iov_base = buf;
    iov[0].iov_len = 1;
    msg.msg_name    = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov     = iov;
    msg.msg_iovlen = 1;

    cmsghdr cm;
    msg.msg_control = &cm;
    msg.msg_controllen = CONTROL_LEN;

    recvmsg( fd, &msg, 0 );

    int fd_to_read = *(int *)CMSG_DATA( &cm );
    return fd_to_read;
}

int main()
{
    int pipefd[2];
    int fd_to_pass = 0;

    int ret = socketpair( PF_UNIX, SOCK_DGRAM, 0, pipefd );
    assert( ret != -1 );

    pid_t pid = fork();
    assert( pid >= 0 );

    if ( pid == 0 )
    {
        close( pipefd[0] );
        fd_to_pass = open( "test.txt", O_RDWR, 0666 );
        send_fd( pipefd[1], ( fd_to_pass > 0 ) ? fd_to_pass : 0 );
        close( fd_to_pass );
        exit( 0 );
    }

    close( pipefd[1] );
    fd_to_pass = recv_fd( pipefd[0] );
    char buf[1024];
    memset( buf, '\0', 1024 );
    read( fd_to_pass, buf, 1024 );
    printf( "I got fd %d and data %s\n", fd_to_pass, buf );
    close( fd_to_pass );
}
