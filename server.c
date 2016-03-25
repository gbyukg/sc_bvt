#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/un.h>
#include <arpa/inet.h>
#define PORT_NUM "59999"
#define BACKLOG 50
#define INT_LEN 30
void errExit(char *msg)
{
    extern int errno;
    perror(msg);
    exit(errno);
}
ssize_t
readLine(int fd, void *buffer, size_t n)
{
    ssize_t numRead;
    size_t totRead;
    char *buf;
    char ch;
    if (n <= 0 || buffer == NULL) {
        errno = EINVAL;
        return -1;
    }
    buf = buffer;
    totRead = 0;
    for (;;) {
        numRead = read(fd, &ch, 1);
        if (numRead == -1) {
            if (errno == EINTR)
                continue;
            else
                return -1;
        } else if (numRead == 0) {
            if (totRead == 0)
                return 0;
            else
                break;
        } else {
            if (totRead < n -1) {
                totRead++;
                *buf++ = ch;
            }
            if (ch == '\n')
                break;
        }
    }
    *buf = '\0';
    return totRead;
}
int main(int argc, char **argv)
{
    uint32_t seqNum;
    struct addrinfo hints, *result, *rp;
    struct sockaddr_storage claddr;
    int lfd, cfd, optval, reqLen, curMod;
    socklen_t addrlen;
#define ADDRSTRLEN (NI_MAXHOST + NI_MAXSERV + 10)
    char reqLenStr[INT_LEN];
    char seqNumStr[INT_LEN];
    char addrStr[ADDRSTRLEN];
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    seqNum = 1, curMod = 0;
    // 忽略信号
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        errExit("signal");
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // 使用 IPv4
    hints.ai_socktype = SOCK_STREAM; // 流 socket
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    if (getaddrinfo(NULL, PORT_NUM, &hints, &result) != 0)
        errExit("getaddrinfo wrong");
    optval = 1;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (lfd == -1)
            // 如果获取 socket 失败, 则继续尝试获取下一个
            continue;
        if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))
                == -1)
            errExit("setsockopt wrong");
        if (bind(lfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
        // bind 失败
        close(lfd);
    }
    if (rp == NULL) {
        printf("Could not bind socket to any address");
        exit(EXIT_FAILURE);
    }
    if (listen(lfd, BACKLOG) == -1)
        errExit("listen wrong");
    // 释放 result
    freeaddrinfo(result);

    // 当全部参数读取完毕后, 关闭服务端程序
    /*while (seqNum < argc) {*/
    while (1) {
        addrlen = sizeof(struct sockaddr_storage);
        cfd = accept(lfd, (struct sockaddr *)&claddr, &addrlen);
        if (cfd == -1) {
            perror("accept wrong");
            continue;
        }
        if (getnameinfo((struct sockaddr *)&claddr, addrlen, host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
            snprintf(addrStr, ADDRSTRLEN, "(%s, %s)", host, service);
        else
            snprintf(addrStr, ADDRSTRLEN, "(?NUKNOWN?)");
        printf("Connection from %s\n", addrStr);
        if (readLine(cfd, reqLenStr, INT_LEN) <= 0) {
            close(cfd);
            continue;
        }
        reqLen = atoi(reqLenStr);
        if (reqLen <= 0) {
             close(cfd);
             continue;
        }
        /*snprintf(seqNumStr, INT_LEN, "%d\n", seqNum);*/

        for (curMod = seqNum; ((curMod < seqNum+reqLen) && (curMod < argc)); curMod++) {
            sleep(1);
            snprintf(seqNumStr, INT_LEN, "%s ", argv[curMod]);
            if (write(cfd, &seqNumStr, strlen(seqNumStr)) != strlen(seqNumStr))
                fprintf(stderr, "Error on write");
            printf("%s\n", seqNumStr);
        }
        seqNum += reqLen;

        if (seqNum >= argc ) {
            snprintf(seqNumStr, INT_LEN, "%c", '\n');
            if (write(cfd, &seqNumStr, strlen(seqNumStr)) != strlen(seqNumStr))
                fprintf(stderr, "Error on write");
        }
        if (close(cfd) == -1)
            errExit("close wrong");
    }
    /*if (listen(lfd, SOMAXCONN) == 0)*/
    return EXIT_SUCCESS;
}
