#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/un.h>
#include <arpa/inet.h>

#define INT_LEN 30
#define MIN_PROC 1  // 允许的最小进程数
#define MAX_PROC 15 // 允许的最大进程数

typedef struct Node{
    char module[64];
    int runned;
    pid_t pid;
    struct Node *pNext;
}Node;

extern int errno;
int maxProc = 5; // 最大进程数, 默认5
volatile sig_atomic_t proc = 5;
char voodoo_path[512];
Node *modNode = NULL;

static void
errExit(char *);

static void
sigchldHandler(int);

static void
usage(void);

void
parse_param(int, char **, Node **);

ssize_t
readLine(int, void *, size_t);

static void
errExit(char *msg)
{
    perror(msg);
    exit(errno);
}

static void
usage()
{
    fprintf(stderr,
            "bvt [option]... \n"
            "  -p|--proc <number>         程序执行时子进程数.\n"
            "  -m|--modules <module name> 指定要跑的模块名.\n"
            "  -?|-h|--help               帮助信息.\n"
            );
};

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

static void
sigchldHandler(int sig)
{
    int status, savedErrno;
    Node *curNode = NULL;
    pid_t childPid;

    savedErrno = errno;

    // 当执行到次函数中时, 有可能会有其他子进程同时结束执行,
    // 因此在此需要捕获一下是否有子进程已经结束执行.
    while ((childPid = waitpid(-1, &status, WNOHANG)) > 0) {
        curNode = NULL;
        proc += 1;

        for (curNode = modNode; curNode != NULL; curNode = curNode->pNext) {
            if (curNode->pid == childPid) {
                break;
            }
        }

        // 获取 module 名
        // modNode->module;
        if (WIFEXITED(status))
            printf("[%d] %s exited, status = %d\n", childPid, curNode->module, status);
        else if (WIFSIGNALED(status))
            printf("child killed by signal %d (%s)",
                   WTERMSIG(status), strsignal(WTERMSIG(status)));
        else
            printf("What happened to this child? (status=%x)\n",
                   (unsigned int) status);
    }

    if (childPid == -1 && errno != ECHILD)
        errExit("waitpid wrong");

    errno = savedErrno;
    fflush(NULL);
}

void
parse_param(int argc, char **argv, Node **modNode) {
    int opt, indexptr, maxProc_tmp;
    char *const short_opt = "p:m:";
    Node *new_node = NULL, *current = NULL;
    struct option long_options[] =
    {
        {"proc",         required_argument, 0, 'p'},
        /*{"modules",      required_argument, 0, 'm'},*/
        {"command",      required_argument, 0, 'c'},
        {"help",         no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, (char *const*)argv, short_opt, long_options, &indexptr)) != -1) {
        switch(opt) {
            case 'p':
                maxProc_tmp = atoi(optarg);
                if (maxProc_tmp > MIN_PROC && maxProc_tmp <= MAX_PROC)
                    maxProc = maxProc_tmp;
                break;
            case 'c':
                break;
            case 'h':
                usage();
                exit(0);
                break;
            default:
                break;
        }
    }
}

int analisis_mods(char *modules, Node **modNode)
{
    int modCounts = 0;
    char *module;
    Node *new_node = NULL, *current = NULL;

    module = strtok(modules, " ");
    while(module != NULL) {
        modCounts++;

        if ((new_node = (Node*)malloc(sizeof(Node))) == NULL)
            errExit("malloc wrong!");

        strncpy(new_node->module, module, 63);
        new_node->runned = 0;
        new_node->pid = 0;
        new_node->pNext = NULL;

        if (*modNode == NULL) {
            *modNode = new_node;
            current = new_node;
        } else {
            current->pNext = new_node;
            current = new_node;
        }

        module = strtok(NULL, " ");
    }

    return modCounts;
}

int getMods(const char *ip, const char *port_num, int proc, Node **modNode)
{
    char reqLenStr[4];
    char seqNumStr[INT_LEN];
    int cfd, moduleCounts = 0;
    ssize_t numRead;
    struct addrinfo hints; struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    if (getaddrinfo(ip, port_num, &hints, &result) != 0)
        errExit("getaddrinfo wrong");

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

    sprintf(reqLenStr, "%d", proc);
    if (write(cfd, reqLenStr, strlen(reqLenStr)) != strlen(reqLenStr))
        errExit("write wrong");
    if (write(cfd, "\n", 1) != 1)
        errExit("write wrong2");
    numRead = readLine(cfd, seqNumStr, INT_LEN);
    if (numRead == -1)
        errExit("readLine wrong");
    if (numRead == 0)
        errExit("Unexpected EOF from server");

    if (seqNumStr[numRead - 1] == '\n') {
        // 删掉最后的换行符, 难受, 真想回家睡觉去...
        seqNumStr[numRead - 2] = '\0';

        if (numRead != 1)
            moduleCounts = analisis_mods(seqNumStr, modNode);
        printf("no more modules, numRead is: %ld\n", numRead);
        close(cfd);
    } else {
        moduleCounts = analisis_mods(seqNumStr, modNode);
    }


    close(cfd);

    return moduleCounts;
}

int main(int argc, char **argv)
{
    errno = 0;
    const char *ip = "127.0.0.1";
    const char *port_num = "59999";
    Node *curNode = NULL;
    struct sigaction sa;
    sigset_t block_sig, child_sig, orign_sig;

    /*parse_param(argc, argv, &modNode);*/

    proc = maxProc;

    // 阻塞除了 SIGCHLD 之外的所有信号
    sigfillset(&block_sig);
    sigdelset(&block_sig, SIGCHLD);

    // 设置阻塞 SIGCHLD 的信号集
    sigfillset(&child_sig);
    sigaddset(&child_sig, SIGCHLD);

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sigchldHandler;
    if ((sigaction(SIGCHLD, &sa, NULL)) == -1)
        errExit("sigaction wrong");

    // 读取模块
    while (1) {
        // 每次都要重置为 NULL
        modNode = NULL;

        // 获取 modules 链表
        if (getMods(ip, port_num, proc, &modNode) == 0)
            break;

        pid_t childPid;

        int i = 1;

        for (curNode = modNode; curNode != NULL; curNode = curNode->pNext) {
            i++;
            // 设置要执行的命令
            char bvt_com[512] = "";
            snprintf(bvt_com, 511,
                     "mvn test -Dtest=%s -Duser.timezone=Asia/Shanghai -P ci > %s 2>&1",
                     curNode->module,
                     curNode->module);

            // 阻塞 SIGCHLD 信号
            if ((sigprocmask(SIG_SETMASK, &child_sig, &orign_sig)) == -1)
                errExit("sigprocmask wrong");

            switch(childPid = fork()) {
                case -1:
                    errExit("Fork wrong!");
                case 0:
                    sleep(i*2);
                    // 子进程开始执行 bvt 脚本
                    // 设置子进程信号处理为默认
                    signal(SIGCHLD, SIG_DFL);

                    // 更改执行目录
                    /*if (chdir(getenv("VOODOO_PATH")) == -1)*/
                        /*errExit("chdir wrong!");*/

                    printf("Starting to run module [%s] ...\n", curNode->module);

                    // 为了 jenkins 能正确输出
                    fflush(NULL);

                    // 开始执行 BVT 脚本
                    /*execlp("sh", "sh", "-c", bvt_com, (char *)0);*/
                    execlp("sh", "sh", "-c", "pwd", (char *)0);

                    switch (errno) {
                        case EACCES:
                            printf("It's not a regular file, or the pathname "
                                   "cannot access.\n");
                            break;
                        case ENOENT:
                            printf("File not exist.\n");
                            break;
                        case ENOEXEC:
                            printf("Cannot parse the file\n");
                            break;
                        case E2BIG:
                            printf("Paramters or ENV arrived the max limit.\n");
                            break;
                        default:
                            printf("What happened to this file?\n");
                            break;
                    }
                    _exit(errno);
                default:
                    proc--;
                    curNode->runned = 1;
                    curNode->pid = childPid;

                    // 使用 while 循环来处理接收到非子进程结束信号的情况.
                    while (proc == 0) {
                        // 休眠, 延迟输出
                        sleep(1);
                        printf("Getting Max child process limit(%d), waiting...\n",
                               maxProc);

                        fflush(NULL);

                        // 解除并开始捕获信号
                        if (sigsuspend(&block_sig) == -1 && errno != EINTR)
                            perror("sigsuspend wrong!");
                    }
                    break;
            }
        }
    }

    // 等待所有子进程结束
    while(proc != maxProc) {
        /*printf("Current procs is : %d\n", proc);*/
        if (sigsuspend(&block_sig) == -1 && errno != EINTR)
            perror("sigsuspend wrong!");
    }

    return 0;
}
