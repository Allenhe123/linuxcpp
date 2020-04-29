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

#include "Epoller.h"

class Client
{
public:
    Client(bool nblock = true);
    virtual ~Client() { close(sockfd_);}

    int connect(const char* ip, int32_t port);
    void loop();

    const std::string& ip() const { return ip_;}
    int32_t port() const { return port_;}
    bool isblock() const { return !nblock_; }

    void send(char* buf);

private:
    void handle_read(int fd);
    void handle_write(int fd);

private:
    std::string ip_ = "";
    int32_t port_;
    int sockfd_ = -1;
    bool nblock_ = true;
    std::unique_ptr<Epoller> poller_ = nullptr;
    bool server_closed_ = false;
};

#endif
