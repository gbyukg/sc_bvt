#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <getopt.h>

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

static void
sigchldHandler(int sig)
{
    int status, savedErrno;
    pid_t childPid;

    savedErrno = errno;

    // 当执行到次函数中时, 有可能会有其他子进程同时结束执行,
    // 因此在此需要捕获一下是否有子进程已经结束执行.
    while ((childPid = waitpid(-1, &status, WNOHANG)) > 0) {
        proc += 1;

        // 获取 module 名
        // modNode->module;
        if (WIFEXITED(status))
            printf("[%d] exited, status = %d\n", childPid, status);
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
}

void
parse_param(int argc, char **argv, Node **modNode) {
    int opt, indexptr, maxProc_tmp;
    char *const short_opt = "p:m:";
    Node *new_node = NULL, *current = NULL;
    struct option long_options[] =
        {
            {"proc",         required_argument, 0, 'p'},
            {"modules",      required_argument, 0, 'm'},
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
            case 'm':
                // -m module1 -m module2 -m module3 ...
                if ((new_node = (Node*)malloc(sizeof(Node))) == NULL)
                    errExit("malloc wrong!");

                strncpy(new_node->module, optarg, 63);
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

int main(int argc, char **argv)
{
    errno = 0;
    Node *curNode = NULL;
    struct sigaction sa;
    sigset_t block_sig, child_sig, orign_sig;

    parse_param(argc, argv, &modNode);

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

    // 模块
    for (curNode = modNode; curNode != NULL; curNode = curNode->pNext) {
        // 阻塞 SIGCHLD 信号
        if ((sigprocmask(SIG_SETMASK, &child_sig, &orign_sig)) == -1)
            errExit("sigprocmask wrong");

        pid_t childPid;

        // 设置要执行的命令
        char bvt_com[512] = "";
        snprintf(bvt_com, 511,
                "mvn test -Dtest=%s -Duser.timezone=Asia/Shanghai -P ci > %s 2>&1",
                curNode->module,
                curNode->module);

        switch(childPid = fork()) {
            case -1:
                errExit("Fork wrong!");
            case 0:
                // 子进程开始执行 bvt 脚本
                // 设置子进程信号处理为默认
                signal(SIGCHLD, SIG_DFL);

                // 更改执行目录
                if (chdir(getenv("VOODOO_PATH")) == -1)
                    errExit("chdir wrong!");

                printf("Start run module [%s] ...\n", curNode->module);

                // 为了 jenkins 能正确输出
                fflush(NULL);

                // 开始执行 BVT 脚本
                execlp("sh", "sh", "-c", bvt_com, (char *)0);

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

                    // 解除并开始捕获信号
                    if (sigsuspend(&block_sig) == -1 && errno != EINTR)
                        perror("sigsuspend wrong!");
                }
                break;
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
