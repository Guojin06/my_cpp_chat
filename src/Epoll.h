#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#include <unistd.h>
#include <stdio.h>

#define MAX_EVENTS 10

class Epoll {
private:
    int epfd_;  // epoll文件描述符
    struct epoll_event events_[MAX_EVENTS];  // 事件数组
    
public:
    Epoll();
    ~Epoll();
    
    bool Create();
    bool AddFd(int fd);
    bool DelFd(int fd);
    int Wait(int timeout = -1);
    int GetEventFd(int i) const;
    void Close();
};

#endif

