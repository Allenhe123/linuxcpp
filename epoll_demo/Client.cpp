#include "Client.h"
#include <thread>
#include <chrono>

Client::Client(bool nblock): nblock_(nblock) {
    sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (nblock) {
        int optval;
        socklen_t optlen = sizeof(optval);
        if (setsockopt(sockfd_, SOL_SOCKET, SOCK_NONBLOCK, (const void*)&optval, sizeof(optval)) < 0)
            perror("setsocketopt for SOCK_NONBLOCK failed:");
    }

    poller_.reset(new Epoller());
}

int Client::connect(const char* ip, int32_t port) {
    struct sockaddr_in  servaddr;
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &servaddr.sin_addr);

    int optval;
    socklen_t optlen = sizeof(optval);
    int res = ::connect(sockfd_, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (res == -1 && errno == EINPROGRESS) {
        getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, reinterpret_cast<void *>(&optval), &optlen);
        if (optval == 0) res = 0;
        else  errno = optval;
        perror("connect error: ");
    }
    return res;
}

 void Client::send(char* buf) {
     (void*)buf;
     handle_write(sockfd_);
 }

void Client::loop() {
    poller_->add_event(sockfd_, EPOLLOUT, -1);

    for (;;) {
        if (server_closed_) {
            printf("server closed\n");
            break;
        }

        std::vector<Response> resp = poller_->wait();
        for (const auto& r : resp) {
            if (r.fd == sockfd_ && r.event & EPOLLIN)
                handle_read(r.fd);
            else if (r.fd == sockfd_ && r.event & EPOLLOUT)
                handle_write(r.fd);
        }
    }
}

void Client::handle_write(int fd) {
    static char buf[1024] = {0};
    snprintf(buf, 1024, "i am client %d! \n", fd);
    int32_t msg_len = strlen(buf);
    char* buffer = new char[msg_len + sizeof(int32_t)];
    char* buffer_ori = buffer;
    memcpy(buffer, &msg_len , sizeof(int32_t));
    buffer += sizeof(int32_t);
    memcpy(buffer, buf, msg_len);

    int32_t len = msg_len + sizeof(int32_t);
    while (len > 0)
    {
        int nwrite = write(fd, buffer_ori, len);
        if (nwrite == -1)
        {
            perror("write error would close session:");
            close(fd);
            break;
        }
        len -= nwrite;
        buffer_ori += nwrite;
    }
    poller_->modify_event(sockfd_, EPOLLIN);
}


void Client::handle_read(int fd) {
    static const int32_t MAXSIZE = 1024;
    static char buf[1024] = {0};
    int32_t header_len = sizeof(int32_t);
    // read msg header
    bool read_error = false;
    char* header_buf = new char[header_len];
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
            server_closed_ = true;
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
    printf("msg len: %d\n", *(int32_t*)header_buf_origin);

    // read msg data
    if (!read_error)
    {
        int32_t len = *(int32_t*)header_buf_origin;
        char* pbuffer = new char[len];
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
                server_closed_ = true;
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
        printf("recv msg: %s\n", buffer_origin);
        delete []buffer_origin;

        //修改描述符对应的事件，由读改为写(为何每次都要修改???)
        if (!read_error)
            poller_->modify_event(fd, EPOLLOUT);
    }
    delete []header_buf_origin;
    if (read_error)
    {
        poller_->delete_event(fd, EPOLLIN);
        close(fd);
    }

}