#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }
    
    printf("Connected to server\n");
    
    char buf[1024];
    while (1) {
        printf("Enter message: ");
        fgets(buf, sizeof(buf), stdin);
        
        send(sock, buf, strlen(buf), 0);
        
        int len = recv(sock, buf, sizeof(buf), 0);
        if (len > 0) {
            buf[len] = '\0';
            printf("Server reply: %s", buf);
        }
    }
    
    close(sock);
    return 0;
}

