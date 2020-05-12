#include "Server.h"
#include <cassert>
#include <vector>
#include <thread>
#include <chrono>


Server::Server(const char* ip, int32_t port, bool nblock, int32_t backlog): 
    ip_(ip), port_(port), backlog_(backlog), nblock_(nblock)
{   
    listenfd_ = socket(AF_INET,SOCK_STREAM, 0);
    if (listenfd_ == -1)
    {
        perror("socket error:");
        exit(1);
    }

    int optval = 0;
    if (setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(optval)) < 0) {
        perror("setsocketopt for SO_REUSEADDR failed:");
    }

    if (nblock) {
        if (setsockopt(listenfd_, SOL_SOCKET, SOCK_NONBLOCK, (const void*)&optval, sizeof(optval)) < 0)
            perror("setsocketopt for SOCK_NONBLOCK failed:");
    }

    poller_.reset(new Epoller());
    header_len_ = sizeof(int32_t) * 2;
    header_buf_.reset(new char[header_len_]);
}

int Server::listen() {
    assert(listenfd_ != -1 && poller_ != nullptr);
    return ::listen(listenfd_, backlog_);
}

int Server::bind() {
    assert(listenfd_ != -1 && poller_ != nullptr);
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip_.c_str(), &servaddr.sin_addr);
    servaddr.sin_port = htons(port_);
    return ::bind(listenfd_, (struct sockaddr*)&servaddr,sizeof(servaddr));
}

void Server::loop() {
    poller_->add_event(listenfd_, EPOLLIN, -1);

    for (;;)
    {
        int num = poller_->wait();
        const std::vector<Response>& resp = poller_->resp();
        for (const auto& r : resp) {
            if (r.fd == listenfd_)
                handle_accept();
            else if (r.event & EPOLLIN)
                handle_read(r.fd);
            else if (r.event & EPOLLOUT)
                handle_write(r.fd);
        }
    }
}

void Server::handle_accept()
{
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int sock_fd = accept4(listenfd_, (struct sockaddr*)&client_addr, &len, SOCK_NONBLOCK);
    while (sock_fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));  // block?
        sock_fd = accept4(listenfd_, (struct sockaddr*)&client_addr, &len, SOCK_NONBLOCK);
    }
    if (sock_fd == -1) {
        perror("accpet error:");
    }
    else
    {
        char address[128] = {0};
        snprintf(address, 128, "%s:%d", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
        connect_clients_.insert(std::make_pair(sock_fd, address));
        printf("accept a new client: %s:%d\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);

        poller_->add_event(sock_fd, EPOLLIN, -1);
    }
}


void Server::handle_read(int fd) 
{
    static const int32_t MAXSIZE = 1024;
    static char buf[1024] = {0};
    int32_t header_len = header_len_;
    // read msg header(msgtype and msglen)
    bool read_error = false;
    char* header_buf = header_buf_.get();
    char* header_buf_origin = header_buf;
    while (header_len > 0)
    {
        int nread = read(fd, buf, header_len);
        if (nread == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            else
            {
                perror("read header error:");
                read_error = true;
                break;
            }
        }
        else if (nread == 0)
        {
            handle_client_close(fd);
            read_error = true;
            break;
        }
        else
        {
            if (nread < header_len)
            {
                memcpy(header_buf, buf, nread);
                header_len -= nread;
                header_buf += nread;
            }
            else
            {
                memcpy(header_buf, buf, nread);
                break;
            }
        }
    }

    // read msg data
    if (!read_error)
    {
        int32_t len = *(int32_t*)header_buf_origin;
        int32_t msgtype = *(int32_t*)(header_buf_origin + sizeof(int32_t));
        printf("recv msglen: %d, msgtype: %d\n", len, msgtype);

        auto uptr = std::make_unique<char []>(len);
        char* pbuffer = uptr.get();
        char* buffer_origin = pbuffer;
        int readed = 0;
        while (len > MAXSIZE)
        {
            readed = read(fd, buf, MAXSIZE);
            if (readed == 0)
            {
                perror("read msg error:");
                read_error = true;
                break;
            }
            else if (readed == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    continue;
                }
                else
                {
                    perror("read msg error:");
                    read_error = true;
                    break;
                }
            }
            memcpy(pbuffer, buf, readed);
            pbuffer += readed;
            len -= readed;
        }
        while (!read_error)
        {
            readed = read(fd, buf, len);
            if (readed == 0)
            {
                handle_client_close(fd);
                read_error = true;
                break;
            }
            else if (readed == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    continue;
                else
                {
                    perror("read msg error:");
                    read_error = true;
                    break;
                }
            }
            memcpy(pbuffer, buf, readed);
            pbuffer += readed;
            len  -= readed;
            if (len <= 0) break;
        }

        handle_msg(fd, msgtype, len, uptr);

        //修改描述符对应的事件，由读改为写(因为socket默认既可读也可写)
        if (!read_error)
            poller_->modify_event(fd, EPOLLOUT);
    }
    else
    {
        poller_->delete_event(fd, EPOLLIN);
        close(fd);
    }
}

void Server::handle_msg(int fd, int32_t msgtype, int32_t msglen, const std::unique_ptr<char []>& msg)
{
    switch (msgtype)
    {
    case MSG_TYPE::MSG_REQ_NAME:
        {
            //parse msg, then package the reply msg
            char* buf = "allenhe";
            int32_t msglen = strlen(buf);
            int32_t msgtype = (int32_t)MSG_TYPE::MSG_RES_NAME;
            // auto uptr = std::make_shared<char []>(msglen + header_len_);
            std::shared_ptr<char[]> uptr(new char[msglen + header_len_]);
            char* buffer = uptr.get();
            memcpy(buffer, &msglen , sizeof(int32_t));
            buffer += sizeof(int32_t);
            memcpy(buffer, &msgtype , sizeof(int32_t));
            buffer += sizeof(int32_t);
            memcpy(buffer, buf, msglen);

            connect_msgs_[fd] = Msg(uptr, msglen, MSG_TYPE::MSG_RES_NAME);

            break;
        }
    
    default:
        break;
    }
}

void Server::handle_write(int fd) {
    auto ite = connect_msgs_.find(fd);
    if (ite == connect_msgs_.end()) {
        printf("can't find write for %d \n", fd);
        return;
    }

    bool write_error = false;
    int32_t msglen = ite->second.msglen;
    int32_t len = msglen + header_len_;
    char* buffer = ite->second.msg.get();
    while (len > 0)
    {
        int nwrite = write(fd, buffer, len);
        if (nwrite == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            else {
                perror("write error would close session:");
                close(fd);
                write_error = true;
                break;
            }
        }
        len -= nwrite;
        buffer += nwrite;
    }

    if (write_error)
        poller_->delete_event(fd, EPOLLOUT);
    else
        poller_->modify_event(fd, EPOLLIN);
}

void Server::handle_client_close(int fd)
{
    auto ite = connect_clients_.find(fd);
    assert(ite != connect_clients_.end());
    fprintf(stderr, "client %s close.\n", ite->second.c_str());
    connect_clients_.erase(ite);
}

/*
write返回-1：
EAGAIN or EWOULDBLOCK：fd被设定为非阻塞，并且write将会被阻塞，立即返回-1，errno为EAGAIN
EINTR：a、阻塞fd：被一个信号打断，但是需要强调的是，在信号打断前没有写入一个字节，才会返回-1，errno设定为EINTR。
       如果有写入，返回已经写入的字节数。这其实很好理解，如果写入了部分数据依然返回-1，errno设定为EINTR，
       处理完中断后，由于不知道被打断时写到了什么地方，也就不知道该从哪一个地方继续写入。
       b、非阻塞fd：调用非阻塞write，即使write被信号打断，write会会继续执行未完成的任务而不会去响应信号。
       因为在非阻塞调用中，没有任何理由阻止read或者wirte的执行。
EPIPE：fd是一个pipe或者socket，而对端的读端关闭。但是一般而言，写进程会收到SIGPIPE信号。
      （注意：和read不一样，read对端关闭使返回0）
————————————————

read返回-1：
返回值<0（-1）
查看errno：

EAGAIN or EWOULDBLOCK：fd设定为非阻塞，且要被阻塞时立即返回-1，设定相应的errno
EINTR：a、阻塞fd：如果在读入任何数据之前被打断，返回-1。如果被打断前读入了部分数据，返回已经读入的数据字节数。
      b、非阻塞fd：调用非阻塞read，即使read被信号打断，read会会继续执行未完成的
      任务而不会去响应信号。因为在非阻塞调用中，没有任何理由阻止read或者wirte的执行。
————————————————

*/