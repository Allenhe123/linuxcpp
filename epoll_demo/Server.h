#ifndef SERVER_H_
#define SERVER_H_

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <memory>
#include <string>

#include "Epoller.h"

class Server
{
public:
    Server(const char* ip, int32_t port, int32_t backlog = 5);
    virtual ~Server() { close(sockfd_);}

    void loop();

  // timeout_ms < 0, keep trying until the operation is successfully
  // timeout_ms == 0, try once
  // timeout_ms > 0, keep trying while there is still time left
  ssize_t Recv(void *buf, size_t len, int flags, int timeout_ms = -1);
  ssize_t Send(const void *buf, size_t len, int flags, int timeout_ms = -1);

private:
    int listen();
    int bind();

private:
    std::string ip_;
    int32_t port_;
    int32_t backlog_;
    int listenfd_ = -1;
    std::unique_ptr<Epoller> poller_ = nullptr;
    bool peer_closed_ = false;
};

#endif
