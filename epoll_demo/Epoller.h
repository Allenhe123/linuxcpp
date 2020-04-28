#ifndef SESSION_H_
#define SESSION_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#include <unordered_map>
#include <vector>


struct Response {
    int fd;
    uint32_t event;
};

//LT , ET ?

class Epoller
{
public:
    explicit Epoller(uint32_t fdsize = 1024, uint32_t max_event = 20): 
        fd_size_(fdsize), max_event_size_(max_event) {}
    ~Epoller() { close(epollfd_); }

    Epoller(const Epoller& poller) = delete;
    Epoller& operator = (const Epoller& poller) = delete;

    void add_event(int fd, uint32_t event, uint32_t timeout);
    void delete_event(int fd, uint32_t event);
    void modify_event(int fd, uint32_t event);

    int fd() const noexcept {
        return epollfd_;
    }

    std::vector<Response>&& wait() const;

private:
    void init();

private:
    int epollfd_ = -1;
    uint32_t fd_size_ = 0;
    uint32_t max_event_size_ = 0;

    std::unordered_map<int, epoll_event> requests_;
};

#endif