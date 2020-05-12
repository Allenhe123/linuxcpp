#ifndef CLIENT_H_
#define CLIENT_H_

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>

#include <memory>
#include <string>
#include <cstring>

#include "MSG.h"

class Client
{
public:
    Client(bool nblock = true);
    virtual ~Client() { close(sockfd_);}

    int connect(const char* ip, int32_t port);
    void loop();

    const std::string& ip() const noexcept { return ip_;}
    int32_t port() const noexcept { return port_;}
    bool isblock() const noexcept { return !nblock_; }

    // user protobuf to serialize a to char*
    void send(char* buf, int32_t len, MSG_TYPE msg_type);
    std::unique_ptr<char[]> recv();

private:
    std::unique_ptr<char[]> handle_read(int fd);
    void handle_write(int fd, const std::unique_ptr<char []>& buf, int32_t msglen);

private:
    std::string ip_ = "";
    int32_t port_;
    int sockfd_ = -1;
    bool nblock_ = true;
    bool server_closed_ = false;
    std::unique_ptr<char []> header_buf_ = nullptr;
    int32_t header_len_ = 8;
};

#endif
