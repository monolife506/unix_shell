// handler.h: Signal Handler 구현

// 현재 포어그라운드에서 실행중인 프로세스의 pid
pid_t foreground_pid = -1;

void catchchild(int signo)
{
    pid_t pid;

    // wait되지 않은 프로세스들 모두 종료
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
        fprintf(stderr, "\nProcess terminated: %d\n", pid);
}

void catchsig(int signo)
{
    fprintf(stderr, "\n");

    // foreground에 실행중인 프로세스가 없다면 시그널을 무시한다.
    if (foreground_pid == -1) {
        fprintf(stderr, "myshell> ");
        return;
    }

    kill(foreground_pid, signo);
    foreground_pid = -1;
}

// 쉘의 시그널 핸들러 설정
void sethandler()
{
    // SIGCHLD 시그널
    static struct sigaction actchild;
    sigfillset(&(actchild.sa_mask));
    actchild.sa_handler = catchchild;
    actchild.sa_flags |= SA_RESTART;

    sigaction(SIGCHLD, &actchild, NULL);

    // SIGINT, SIGQUIT 시그널
    static struct sigaction actsig;
    sigfillset(&(actsig.sa_mask));
    actsig.sa_handler = catchsig;
    actsig.sa_flags |= SA_RESTART;

    sigaction(SIGINT, &actsig, NULL);
    sigaction(SIGQUIT, &actsig, NULL);
}