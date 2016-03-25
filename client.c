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
#define INT_LEN 30
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
void errExit(char *msg)
{
    extern int errno;
    perror(msg);
    exit(errno);
}
int main(int argc, char **argv)
{
    char *reqLenStr;
    char seqNumStr[INT_LEN];
    int cfd;
    ssize_t numRead;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    if (argc < 2 || strcmp(argv[1], "--help") == 0) {
        printf("Usage");
        exit(0);
    }
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    if (getaddrinfo(argv[1], PORT_NUM, &hints, &result) != 0)
        errExit("getaddrinfo wrong");
    reqLenStr = (argc > 2) ? argv[2] : "1";

    while (1) {
        cfd = 0;
        for (rp = result; rp != NULL; rp = rp->ai_next) {
            cfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (cfd == -1)
                continue;
            if (connect(cfd, rp->ai_addr, rp->ai_addrlen) != -1)
                break;
            close(cfd);
        }
        if (rp == NULL) {
            printf("Could not connect socket to any address");
            exit(EXIT_FAILURE);
        }

        if (write(cfd, reqLenStr, strlen(reqLenStr)) != strlen(reqLenStr))
            errExit("write wrong");
        if (write(cfd, "\n", 1) != 1)
            errExit("write wrong2");
        numRead = readLine(cfd, seqNumStr, INT_LEN);
        if (numRead == -1)
            errExit("readLine wrong");
        if (numRead == 0)
            errExit("Unexpected EOF from server");

        // 调用 BVT 测试脚本

        if (seqNumStr[numRead - 1] == '\n') {
            // 删掉最后的换行符, 难受, 真想回家睡觉去...
            seqNumStr[numRead - 2] = '\0';

            if (numRead != 1)
                printf("Sequence number: %s\n", seqNumStr);
            printf("no more modules, numRead is: %ld\n", numRead);
            close(cfd);
            break;
        }
        printf("Sequence number: %s\n", seqNumStr);

        close(cfd);
    }

    return 0;
}
