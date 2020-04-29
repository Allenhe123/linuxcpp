#ifndef SERVER_H_
#define SERVER_H_

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

class Server
{
public:
    Server(const char* ip, int32_t port, bool nblock = true, int32_t backlog = 5);
    virtual ~Server() { close(listenfd_);}

    int listen();
    int bind();
    void loop();

    const std::string& ip() const { return ip_;}
    int32_t port() const { return port_;}
    bool isblock() const { return !nblock_; }

private:
    void handle_accept();
    void handle_read(int fd);
    void handle_write(int fd);
    void handle_client_close(int fd);

private:
    std::string ip_ = "";
    int32_t port_;
    int32_t backlog_;
    int listenfd_ = -1;
    bool nblock_ = true;
    std::unique_ptr<Epoller> poller_ = nullptr;
    std::unordered_map<int, std::string> connect_clients_;
};

#endif
