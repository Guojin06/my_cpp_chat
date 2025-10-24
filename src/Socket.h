#ifndef SOCKET_H
#define SOCKET_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

class Socket {
private:
    int fd_;  // socket文件描述符
    
public:
    Socket();
    ~Socket();
    
    bool Create();
    bool Bind(int port);
    bool Listen(int backlog = 128);
    int Accept();
    bool SetReuseAddr();
    bool SetNonBlocking();
    int Send(const char* buf, int len);
    int Recv(char* buf, int len);
    void Close();
    
    int GetFd() const { return fd_; }
};

#endif

