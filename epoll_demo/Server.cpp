#include "Server.h"
#include <cassert>
#include <vector>


Server::Server(const char* ip, int32_t port, int32_t backlog): ip_(ip), port_(port), backlog_(backlog)
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

    if (bind() == -1)
    {
        perror("bind error: ");
        exit(1);
    }

    if (listen() ==  -1) {
        perror("bind error: ");
        exit(1);
    }

}

// int Server::init(int domain, int type, int protocol, uint32_t fdsize, uint32_t maxevent)
// {
//     sockfd_ = socket(domain, type | SOCK_NONBLOCK, protocol);
//     poller_.reset(new Epoller(fdsize, maxevent));
// }

int Server::listen() {
    assert(listenfd_ != -1 && poller_ != nullptr);
    return listen(listenfd_, backlog_);
}

int Server::bind(const struct sockaddr *addr, socklen_t addrlen) {
    assert(listenfd_ != -1 && poller_ != nullptr);
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip_.c_str(), &servaddr.sin_addr);
    servaddr.sin_port = htons(port_);
    return bind(listenfd_, addr, addrlen);
}

void Server::loop() {

    for (;;)
    {
        if (peer_closed_) {
            break;
        }
        std::vector<Response> resp = poller_->wait();
        for (const auto& r : resp) {
            if (r.event & EPOLLIN)
                 do_read(epollfd, fd, sockfd, buf);
            else if (r.event & EPOLLOUT)
                do_write(epollfd, fd, sockfd, buf);
        }
    }
}

int Session::accept(struct sockaddr *addr, socklen_t *addrlen) -> SessionPtr {
  ACHECK(fd_ != -1);

  int sock_fd = accept4(fd_, addr, addrlen, SOCK_NONBLOCK);
  while (sock_fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    poll_handler_->Block(-1, true);
    sock_fd = accept4(fd_, addr, addrlen, SOCK_NONBLOCK);
  }

  if (sock_fd == -1) {
    return nullptr;
  }

  return std::make_shared<Session>(sock_fd);
}

int Session::Connect(const struct sockaddr *addr, socklen_t addrlen) {
  ACHECK(fd_ != -1);

  int optval;
  socklen_t optlen = sizeof(optval);
  int res = connect(fd_, addr, addrlen);
  if (res == -1 && errno == EINPROGRESS) {
    poll_handler_->Block(-1, false);
    getsockopt(fd_, SOL_SOCKET, SO_ERROR, reinterpret_cast<void *>(&optval),
               &optlen);
    if (optval == 0) {
      res = 0;
    } else {
      errno = optval;
    }
  }
  return res;
}

int Session::Close() {
  ACHECK(fd_ != -1);

  poll_handler_->Unblock();
  int res = close(fd_);
  fd_ = -1;
  return res;
}

ssize_t Session::Recv(void *buf, size_t len, int flags, int timeout_ms) {
  ACHECK(buf != nullptr);
  ACHECK(fd_ != -1);

  ssize_t nbytes = recv(fd_, buf, len, flags);
  if (timeout_ms == 0) {
    return nbytes;
  }

  while (nbytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    if (poll_handler_->Block(timeout_ms, true)) {
      nbytes = recv(fd_, buf, len, flags);
    }
    if (timeout_ms > 0) {
      break;
    }
  }
  return nbytes;
}

ssize_t Session::RecvFrom(void *buf, size_t len, int flags,
                          struct sockaddr *src_addr, socklen_t *addrlen,
                          int timeout_ms) {
  ACHECK(buf != nullptr);
  ACHECK(fd_ != -1);

  ssize_t nbytes = recvfrom(fd_, buf, len, flags, src_addr, addrlen);
  if (timeout_ms == 0) {
    return nbytes;
  }

  while (nbytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    if (poll_handler_->Block(timeout_ms, true)) {
      nbytes = recvfrom(fd_, buf, len, flags, src_addr, addrlen);
    }
    if (timeout_ms > 0) {
      break;
    }
  }
  return nbytes;
}

ssize_t Session::Send(const void *buf, size_t len, int flags, int timeout_ms) {
  ACHECK(buf != nullptr);
  ACHECK(fd_ != -1);

  ssize_t nbytes = send(fd_, buf, len, flags);
  if (timeout_ms == 0) {
    return nbytes;
  }

  while ((nbytes == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    if (poll_handler_->Block(timeout_ms, false)) {
      nbytes = send(fd_, buf, len, flags);
    }
    if (timeout_ms > 0) {
      break;
    }
  }
  return nbytes;
}

ssize_t Session::SendTo(const void *buf, size_t len, int flags,
                        const struct sockaddr *dest_addr, socklen_t addrlen,
                        int timeout_ms) {
  ACHECK(buf != nullptr);
  ACHECK(dest_addr != nullptr);
  ACHECK(fd_ != -1);

  ssize_t nbytes = sendto(fd_, buf, len, flags, dest_addr, addrlen);
  if (timeout_ms == 0) {
    return nbytes;
  }

  while ((nbytes == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    if (poll_handler_->Block(timeout_ms, false)) {
      nbytes = sendto(fd_, buf, len, flags, dest_addr, addrlen);
    }
    if (timeout_ms > 0) {
      break;
    }
  }
  return nbytes;
}

ssize_t Session::Read(void *buf, size_t count, int timeout_ms) {
  ACHECK(buf != nullptr);
  ACHECK(fd_ != -1);

  ssize_t nbytes = read(fd_, buf, count);
  if (timeout_ms == 0) {
    return nbytes;
  }

  while ((nbytes == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    if (poll_handler_->Block(timeout_ms, true)) {
      nbytes = read(fd_, buf, count);
    }
    if (timeout_ms > 0) {
      break;
    }
  }
  return nbytes;
}

ssize_t Session::Write(const void *buf, size_t count, int timeout_ms) {
  ACHECK(buf != nullptr);
  ACHECK(fd_ != -1);

  ssize_t nbytes = write(fd_, buf, count);
  if (timeout_ms == 0) {
    return nbytes;
  }

  while ((nbytes == -1) && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    if (poll_handler_->Block(timeout_ms, false)) {
      nbytes = write(fd_, buf, count);
    }
    if (timeout_ms > 0) {
      break;
    }
  }
  return nbytes;
}
