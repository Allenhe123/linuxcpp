#include "Epoller.h"
#include <iostream>

void Epoller::init() {
    epollfd_ = epoll_create(epoll_size_);
    if (epollfd_ == -1) {
        perror("epoll_create failed:");
    }
}

void Epoller::add_event(int fd, uint32_t event, uint32_t timeout) {
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    int err = epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev);
    if (err == -1) {
        perror("epoll_ctl failed:");
        return;
    }
    requests_.insert(std::make_pair(fd, ev));
}

void Epoller::delete_event(int fd, uint32_t event) {
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    int err = epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &ev);
    if (err == -1) {
        perror("epoll_ctl failed:");
        return;
    }

    auto ite = requests_.find(fd);
    if (ite == requests_.end()) {
        std::cout << "can not find the fd to delete: " << fd << std::endl;
        return;
    }
    requests_.erase(ite);
}

void Epoller::modify_event(int fd, uint32_t event) {
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    int err = epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &ev);
    if (err == -1) {
        perror("epoll_ctl failed:");
        return;
    }
    requests_[fd] = ev;
}

std::vector<Response>&& Epoller::wait() const {
    struct epoll_event events[max_event_size_];
    std::vector<Response> vec;
    int num = epoll_wait(epollfd_, events, max_event_size_, -1);
    if (num == -1) {
        perror("epoll_wait failed:");
        return std::move(vec);
    }
    for (int i = 0; i < num;i++)
    {
        vec.emplace_back(events[i].data.fd, events[i].events);
    }
    return std::move(vec);
}
