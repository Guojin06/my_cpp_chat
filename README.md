# my-chat-system

一个从零开始的C++网络编程项目。

## 项目动机

最近在准备后端开发岗位面试，发现epoll是高频考点，但光看理论理解不深。决定自己动手实现一个echo服务器，通过实战加深对Linux网络编程的理解。

## 当前进度

- [x] **Day 1 (10.24)**: 实现基于epoll的echo服务器
  - 学习了Socket基本API
  - 学习了epoll的ET/LT模式区别
  - 封装了Socket和Epoll类
  - 解决了ET模式下数据丢失的坑

## 技术栈

- C++11
- Linux Socket API
- epoll (ET模式)

## 快速开始

**编译：**
```bash
make
```

**运行服务器：**
```bash
./server
```

**测试（另开终端）：**
```bash
./client
```

或者用telnet测试：
```bash
telnet 127.0.0.1 8080
```

## 项目结构

```
my-chat-system/
├── src/
│   ├── Socket.h        # Socket封装类
│   ├── Epoll.h         # Epoll封装类
│   └── server.cpp      # 服务器主程序
├── test/
│   └── test_client.cpp # 测试客户端
├── docs/
│   └── Day1-学习笔记.md
├── Makefile
└── README.md
```

## 学习笔记

- [Day 1: Socket与Epoll学习笔记](docs/Day1-学习笔记.md)

## 踩过的坑

### 1. bind: Address already in use

**问题：** 服务器重启时，端口被占用。

**原因：** TCP四次挥手后，主动关闭方会进入TIME_WAIT状态（2MSL），期间端口不能复用。

**解决：** 设置`SO_REUSEADDR`选项。

```cpp
int on = 1;
setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
```

### 2. ET模式下数据丢失

**问题：** 客户端发送大量数据时，服务器只收到部分数据就卡住了。

**原因：** ET模式下，如果一次`recv()`没读完所有数据，`epoll_wait()`不会再通知。

**解决：** 
1. 设置socket为非阻塞模式
2. 循环`recv()`直到返回`EAGAIN`

```cpp
while (1) {
    int len = recv(fd, buf, sizeof(buf), 0);
    if (len > 0) {
        // 处理数据
    } else if (len == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        break;  // 数据读完了
    }
}
```

## 下一步计划

明天打算：
- 重构代码结构，把Socket和Epoll的实现分离到.cpp文件
- 可能加入简单的线程池处理客户端请求

具体怎么做，边做边看。

---

这是我的学习项目，用于深入理解Linux网络编程和C++后端开发。

