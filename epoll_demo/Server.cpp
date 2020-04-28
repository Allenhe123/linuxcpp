#include "Server.h"
#include <cassert>
#include <vector>
#include <thread>
#include <chrono>


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

    poller_.reset(new Epoller());

    // if (bind() == -1)
    // {
    //     perror("bind error: ");
    //     exit(1);
    // }

    // if (listen() ==  -1) {
    //     perror("bind error: ");
    //     exit(1);
    // }

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

int Server::bind() {
    assert(listenfd_ != -1 && poller_ != nullptr);
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip_.c_str(), &servaddr.sin_addr);
    servaddr.sin_port = htons(port_);
    return bind(listenfd_, (struct sockaddr*)&servaddr,sizeof(servaddr));
}

void Server::loop() {

    for (;;)
    {
        if (peer_closed_) {
            break;
        }
        std::vector<Response> resp = poller_->wait();
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


void Server::handle_accpet()
{
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int sock_fd = accept4(listenfd_, (struct sockaddr*)&client_addr, &len, SOCK_NONBLOCK);
    while (sock_fd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // block?
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


void  Server::handle_read(int fd) {
    static const uint32_t MAXSIZE = 1024;
    static const char buf[MAXSIZE] = {0};
    // read msg header
    bool read_error = false;
    int32_t header_len = sizeof(int32_t);
    char* header_buf = new char[header_len];
    char* header_buf_origin = header_buf;
    while (header_len > 0)
    {
        int nread = read(fd, buf, header_len);
        if (nread == -1)
        {
            perror("read error would close session:");
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            else
            {
                read_error = true;
                break;
            }
        }
        else if (nread == 0)
        {
            auto ite = connect_clients_.find(fd);
            assert(ite != connect_clients_.end());
            fprintf(stderr, "client %s close.\n", ite->second.c_str());
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
                read_error = true;
                break;
            }
            else if (readed == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    continue;
                else
                {
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
                read_error = true;
                break;
            }
            else if (readed == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    continue;
                else
                {
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
            modify_event(epollfd, fd, EPOLLOUT);
    }
    delete []header_buf_origin;

    if (read_error)
    {
        delete_event(epollfd, fd, EPOLLIN);
        close(fd);
    }
}


void Server::handle_write(int fd) {
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
    poller_->modify_event(fd, EPOLLIN);
    // modify_event(epollfd, fd, EPOLLIN);
     
    memset(buf, 0, MAXSIZE);
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
