#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define PORT_NUM "59999"
#define BACKLOG 50
#define INT_LEN 30

typedef struct Node{
    char module[64];
    int runned;
    pid_t pid;
    struct Node *pNext;
}Node;

const char *pid_file = "/home/btit/sc_bvt/bvt_server.pid";
const char *module_file = "/home/btit/sc_bvt/modules.txt";
/*const char *pid_file = "/Users/gbyukg/work/ci/sc_bvt/bvt_server.pid";*/
/*const char *module_file = "/Users/gbyukg/work/ci/sc_bvt/modules.txt";*/
Node *modNode = NULL;
volatile sig_atomic_t moduleCount = 0;
volatile sig_atomic_t seqNum = 0;

void errExit(char *);

ssize_t
readLine(int, void *, size_t);

int
becomeDeamon();

void
getModules();

void
server_exit(void);

void
write_pid(void);

static void
sigusr1Handler(int);

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

int
becomeDeamon()
{
    int fd;

    switch (fork()) {
        case -1: return -1;
        case 0: break;
        default: _exit(EXIT_SUCCESS);
    }

    if (setsid() == -1)
        return -1;

    switch (fork()) {
        case -1: return -1;
        case 0: break;
        default: _exit(EXIT_SUCCESS);
    }

    umask(0);
    chdir("/");

    // 写入 pid 文件
    write_pid();

    close(STDIN_FILENO);

    fd = open("/dev/null", O_RDWR);
    if (fd != STDIN_FILENO)
        return -1;
    if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
        return -1;
    if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
        return -1;

    return 0;
}

void
write_pid(void)
{
    int pid_fd;
    char pid_buf[16];
    memset(pid_buf, '\0', sizeof(pid_buf));

    if ((pid_fd = open(pid_file, O_RDWR|O_TRUNC|O_CLOEXEC|O_CREAT, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)) == -1)
        errExit("Open pid file failed");

    snprintf(pid_buf, 16, "%d\n", getpid());

    if ((write(pid_fd, pid_buf, 16)) < 0)
        errExit("Write pid file wrong");

    close(pid_fd);

    // 注册退出函数
    if (atexit(server_exit) != 0)
        errExit("atexit wrong");
}

void
server_exit(void)
{
    unlink(pid_file);
}

void
getModules()
{
    FILE *fp;
    char *module= NULL;
    size_t len = 0;
    ssize_t read;
    // 重新初始化
    modNode = NULL;
    Node *new_node = NULL, *current = NULL;
    moduleCount = seqNum = 0; // 初始化读取的模块数

    // 打开模块文件读取所有模块
    if ((fp = fopen(module_file, "r")) == NULL)
        errExit("Open modules file wrong!");

    // 开始读取文件获取所有模块
    while ((read = getline(&module, &len, fp)) != -1) {
        // 去掉每一行结尾的换行符
        module[read - 1] = '\0';

        if ((new_node = (Node*)malloc(sizeof(Node))) == NULL)
            errExit("malloc wrong!");

        strncpy(new_node->module, module, 63);
        new_node->runned = 0;
        new_node->pid = 0;
        new_node->pNext = NULL;

        if (modNode == NULL) {
            modNode = new_node;
            current = new_node;
        } else {
            current->pNext = new_node;
            current = new_node;
        }
        moduleCount++;
    }

    free(module);
    fclose(fp);
}

static void
sigusr1Handler(int sig)
{
    getModules();
}

int main(int argc, char **argv)
{
    struct addrinfo hints, *result, *rp;
    struct sockaddr_storage claddr;
    int lfd = 0, cfd, optval, reqLen, curMod = 0;
    socklen_t addrlen;
#define ADDRSTRLEN (NI_MAXHOST + NI_MAXSERV + 10)
    char reqLenStr[INT_LEN];
    char seqNumStr[INT_LEN];
    char addrStr[ADDRSTRLEN];
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    // 阻塞 USR1 信号
    sigset_t usr1_set, emp_set;
    sigemptyset(&usr1_set);
    sigemptyset(&emp_set);
    sigaddset(&usr1_set, SIGUSR1);

    // usr1 信号
    struct sigaction usr1sa;

    // 设置 USR1 信号
    sigemptyset(&usr1sa.sa_mask);
    usr1sa.sa_flags = 0;
    usr1sa.sa_handler = sigusr1Handler;

    if (sigaction(SIGUSR1, &usr1sa, NULL) == -1)
        errExit("sigaction SIGUSR1 wrong");

    // 转换成守护进程
    if (becomeDeamon() != 0)
        errExit("become deamon wrong!");

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

    while (1) {
        addrlen = sizeof(struct sockaddr_storage);
        cfd = accept(lfd, (struct sockaddr *)&claddr, &addrlen);
        if (cfd == -1) {
            if (errno != EINTR)
                perror("accept wrong");
            continue;
        }

        // 需要阻塞 USR1 信号, 防止正在处理 modules 时获取到 USR1 信号, 重置获取到的 module
        if (sigprocmask(SIG_SETMASK, &usr1_set, NULL) == -1)
            errExit("sigprocmask wrong");

        if (getnameinfo((struct sockaddr *)&claddr, addrlen, host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
            snprintf(addrStr, ADDRSTRLEN, "(%s, %s)", host, service);
        else
            snprintf(addrStr, ADDRSTRLEN, "(?NUKNOWN?)");

        // 需要写 log
        /*printf("Connection from %s\n", addrStr);*/
        if (readLine(cfd, reqLenStr, INT_LEN) <= 0) {
            close(cfd);
            continue;
        }
        // 获取客户端需要读取的模块数
        reqLen = atoi(reqLenStr);
        if (reqLen <= 0) {
            close(cfd);
            continue;
        }
        /*snprintf(seqNumStr, INT_LEN, "%d\n", seqNum);*/

        for (curMod = seqNum; ((curMod < seqNum+reqLen) && (curMod < moduleCount) && modNode != NULL); curMod++) {
            snprintf(seqNumStr, INT_LEN, "%s ", modNode->module);
            // 切记需要释放资源
            free(modNode);

            if (write(cfd, &seqNumStr, strlen(seqNumStr)) != strlen(seqNumStr))
                fprintf(stderr, "Error on write");

            // 指向下一节点
            modNode = modNode->pNext;

            // 需要写 log
            /*printf("%s\n", seqNumStr);*/
        }
        seqNum += reqLen;

        if (seqNum > moduleCount ) {
            // 当所有模块全部都去完毕后, 恢复 USR1 信号的阻塞状态
            if (sigprocmask(SIG_SETMASK, &emp_set, NULL) == -1)
                errExit("sigprocmask wrong");

            snprintf(seqNumStr, INT_LEN, "%c", '\n');
            if (write(cfd, &seqNumStr, strlen(seqNumStr)) != strlen(seqNumStr))
                fprintf(stderr, "Error on write");
        }
        if (close(cfd) == -1)
            errExit("close wrong");
    }

    return EXIT_SUCCESS;
}
