#include "Socket.h"
#include "Epoll.h"
#include <errno.h>

// ============================================
// Socket类实现
// ============================================

Socket::Socket() : fd_(-1) {}

Socket::~Socket() {
    Close();  // RAII：析构时自动关闭socket，不会忘记释放资源
}

bool Socket::Create() {
    // 创建TCP socket
    // AF_INET: IPv4协议族
    // SOCK_STREAM: TCP流式socket
    // 0: 默认协议
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) {
        perror("socket");
        return false;
    }
    return true;
}

bool Socket::Bind(int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);  // 主机字节序转网络字节序（大端）
    addr.sin_addr.s_addr = INADDR_ANY;  // 监听所有网卡：0.0.0.0
    
    int ret = bind(fd_, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        perror("bind");
        return false;
    }
    return true;
}

bool Socket::Listen(int backlog) {
    // backlog: 全连接队列的长度
    // 这里用128，一般够用了（内核会向上取整到2的幂次）
    int ret = listen(fd_, backlog);
    if (ret < 0) {
        perror("listen");
        return false;
    }
    return true;
}

int Socket::Accept() {
    int client_fd = accept(fd_, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
    }
    return client_fd;
}

bool Socket::SetReuseAddr() {
    // 重要！解决TIME_WAIT状态导致的端口占用问题
    // 踩坑记录：一开始没加这个，服务器Ctrl+C后重启就报"Address already in use"
    // 原因：TCP四次挥手后，主动关闭方会进入TIME_WAIT状态（2MSL，大概2分钟）
    // 这期间端口被占用，无法重新bind
    // 解决：设置SO_REUSEADDR，允许重用处于TIME_WAIT的端口
    int on = 1;
    int ret = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret < 0) {
        perror("setsockopt");
        return false;
    }
    return true;
}

bool Socket::SetNonBlocking() {
    // 设置非阻塞模式（ET模式必须配合非阻塞I/O）
    // 1. 先获取当前flags
    int flags = fcntl(fd_, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl F_GETFL");
        return false;
    }
    
    // 2. 添加O_NONBLOCK标志
    flags |= O_NONBLOCK;
    int ret = fcntl(fd_, F_SETFL, flags);
    if (ret < 0) {
        perror("fcntl F_SETFL");
        return false;
    }
    return true;
}

int Socket::Send(const char* buf, int len) {
    return send(fd_, buf, len, 0);
}

int Socket::Recv(char* buf, int len) {
    return recv(fd_, buf, len, 0);
}

void Socket::Close() {
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }
}

// ============================================
// Epoll类实现
// ============================================

Epoll::Epoll() : epfd_(-1) {}

Epoll::~Epoll() {
    Close();
}

bool Epoll::Create() {
    epfd_ = epoll_create(1);
    if (epfd_ < 0) {
        perror("epoll_create");
        return false;
    }
    return true;
}

bool Epoll::AddFd(int fd) {
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;  // EPOLLIN: 可读事件  EPOLLET: 边缘触发模式
    ev.data.fd = fd;  // 把fd保存到data联合体中，epoll_wait返回时用
    
    int ret = epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
    if (ret < 0) {
        perror("epoll_ctl ADD");
        return false;
    }
    return true;
}

bool Epoll::DelFd(int fd) {
    int ret = epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, NULL);
    if (ret < 0) {
        perror("epoll_ctl DEL");
        return false;
    }
    return true;
}

int Epoll::Wait(int timeout) {
    return epoll_wait(epfd_, events_, MAX_EVENTS, timeout);
}

int Epoll::GetEventFd(int i) const {
    return events_[i].data.fd;
}

void Epoll::Close() {
    if (epfd_ != -1) {
        close(epfd_);
        epfd_ = -1;
    }
}

// ============================================
// 主程序
// ============================================

int main() {
    // ============================================
    // 1. 创建并初始化监听socket
    // ============================================
    Socket listen_sock;
    listen_sock.Create();
    listen_sock.SetReuseAddr();  // 重要！解决TIME_WAIT端口占用
    listen_sock.Bind(8080);
    listen_sock.Listen(128);
    listen_sock.SetNonBlocking();  // ET模式需要非阻塞
    
    printf("Server started on port 8080\n");
    
    // ============================================
    // 2. 创建epoll实例，监听listen_sock
    // ============================================
    Epoll epoll_obj;
    epoll_obj.Create();
    epoll_obj.AddFd(listen_sock.GetFd());
    
    // ============================================
    // 3. 事件循环（Reactor模式核心）
    // ============================================
    while (1) {
        // 等待事件（-1表示无限等待）
        int n = epoll_obj.Wait(-1);
        
        // 遍历所有就绪的事件
        for (int i = 0; i < n; i++) {
            int fd = epoll_obj.GetEventFd(i);
            
            if (fd == listen_sock.GetFd()) {
                // ================================
                // 新连接到来
                // ================================
                // 注意：ET模式下必须循环accept，直到返回EAGAIN
                // 原因：ET只通知一次，如果有多个连接同时到来，只accept一个会漏掉其他连接
                while (1) {
                    int client_fd = listen_sock.Accept();
                    if (client_fd < 0) {
                        // EAGAIN表示没有新连接了（非阻塞模式下的正常返回）
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;  // 正常退出循环
                        }
                        break;  // 其他错误也退出
                    }
                    
                    printf("New client: fd=%d\n", client_fd);
                    
                    // 重要！客户端socket也要设置非阻塞
                    // 原因：ET模式下recv必须循环读取，如果是阻塞模式会卡住
                    int flags = fcntl(client_fd, F_GETFL, 0);
                    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
                    
                    // 把客户端socket加入epoll监听
                    epoll_obj.AddFd(client_fd);
                }
            } else {
                // ================================
                // 客户端数据到来
                // ================================
                // 注意：ET模式下必须循环recv，直到返回EAGAIN
                // 踩坑记录：一开始只recv一次，导致大数据包只收到一部分就卡住了
                // 原因：ET模式只通知一次，如果没读完，epoll_wait不会再通知
                // 解决：循环读取直到EAGAIN
                char buf[1024];
                
                while (1) {
                    int len = recv(fd, buf, sizeof(buf), 0);
                    
                    if (len > 0) {
                        // 收到数据，echo回去
                        printf("Recv %d bytes from fd=%d\n", len, fd);
                        send(fd, buf, len, 0);
                    } else if (len == 0) {
                        // 对端关闭连接
                        printf("Client fd=%d closed\n", fd);
                        epoll_obj.DelFd(fd);
                        close(fd);
                        break;
                    } else {
                        // len == -1，出错了
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            // 数据读完了（非阻塞模式下的正常返回）
                            break;  // 正常退出循环
                        } else {
                            // 真的出错了
                            perror("recv error");
                            epoll_obj.DelFd(fd);
                            close(fd);
                            break;
                        }
                    }
                }
            }
        }
    }
    
    return 0;
}

