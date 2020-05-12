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

    header_len_ = sizeof(int32_t) * 2;
    header_buf_.reset(new char[header_len_]);
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

 void Client::send(char* buf, int32_t len, MSG_TYPE msg_type) {
    int32_t msglen = len;
    int32_t msgtype = (int32_t)msg_type;
    auto uptr = std::make_unique<char[]>(len + header_len_);
    char* buffer = uptr.get();
    memcpy(buffer, &msglen, sizeof(int32_t));
    buffer += sizeof(int32_t);
    memcpy(buffer, &msgtype, sizeof(int32_t));
    buffer += sizeof(int32_t);
    memcpy(buffer, buf, msglen);

    handle_write(sockfd_, uptr, msglen);
 }

 std::unique_ptr<char[]> Client::recv() {
     return std::move(handle_read(sockfd_));
 }


void Client::handle_write(int fd, const std::unique_ptr<char []>& buf, int32_t msglen) {
    bool write_error = false;
    char* buffer = buf.get();
    int32_t len = msglen + header_len_;

    while (len > 0)
    {
        int nwrite = write(fd, buffer, len);
        if (nwrite == -1)
        {
            perror("write error would close session:");
            write_error = true;
            break;
        }
        len -= nwrite;
        buffer += nwrite;
    }
    if (write_error) {
        close(fd);
    }
}

std::unique_ptr<char[]> Client::handle_read(int fd) {
    static const int32_t MAXSIZE = 1024;
    static char buf[1024] = {0};
    // read header(msglen and msgtype)
    int32_t header_len = header_len_;
    bool read_error = false;
    char* header_buf = header_buf_.get();
    char* header_buf_origin = header_buf_.get();
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
            printf("peer closed\n");
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
        printf("msg len: %d\n", *(int32_t*)header_buf_origin);
        printf("msg type: %d\n", *(int32_t*)(header_buf_origin + sizeof(int32_t)));

        int32_t len = *(int32_t*)header_buf_origin;
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
                server_closed_ = true;
                printf("peer closed\n");
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
        return std::move(uptr);
    }
    else
    {
        close(fd);
        return nullptr;
    }
}
