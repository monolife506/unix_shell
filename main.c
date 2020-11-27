#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "execute.h"
#include "handler.h"

#define MAX_CMD_ARG 20

struct Mark {
    int idx;
    char chr;
};

const char* prompt = "myshell> ";
char cmdline[BUFSIZ];
char* cmdvector[MAX_CMD_ARG];

int pipe_cnt = 0;
int mark_cnt = 0;
struct Mark markloc[MAX_CMD_ARG];
int pipefd[MAX_CMD_ARG][2];

void fatal(char* str)
{
    perror(str);
    exit(1);
}

int makelist(char* s, const char* delimiters, char** list, int MAX_LIST)
{
    int i = 0;
    int numtokens = 0;
    char* snew = NULL;

    if ((s == NULL) || (delimiters == NULL))
        return -1;

    snew = s + strspn(s, delimiters); /* Skip delimiters */
    if ((list[numtokens] = strtok(snew, delimiters)) == NULL)
        return numtokens;

    numtokens = 1;

    while (1) {
        if ((list[numtokens] = strtok(NULL, delimiters)) == NULL)
            break;
        if (numtokens == (MAX_LIST - 1))
            return -1;
        numtokens++;
    }

    return numtokens;
}

void setmark(int cmd_length)
{
    mark_cnt = 0;
    pipe_cnt = 0;

    for (size_t i = 0; i < cmd_length; i++) {
        // pipe와 redirection 기호의 위치 확인
        if (cmdvector[i][0] == '|' || cmdvector[i][0] == '<' || cmdvector[i][0] == '>') {
            mark_cnt++;
            markloc[mark_cnt].chr = cmdvector[i][0];
            markloc[mark_cnt].idx = i;
            if (cmdvector[i][0] == '|')
                pipe_cnt++;
        }
    }

    // markloc 초기화
    markloc[0].chr = '\0';
    markloc[0].idx = -1;
    mark_cnt++;
    markloc[mark_cnt].chr = '\0';
    markloc[mark_cnt].idx = cmd_length;
}

int main(int argc, char** argv)
{
    sethandler();

    while (1) {
        fputs(prompt, stdout);
        fgets(cmdline, BUFSIZ, stdin);
        cmdline[strlen(cmdline) - 1] = '\0';

        // argument의 개수
        int cmd_length = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);

        // 파이프를 위한 스트링 나누기
        setmark(cmd_length);

        // 명령어가 입력되지 않은 경우
        if (cmd_length == 0)
            continue;

        // cd 명령어는 fork()하지 않음
        if (strcmp(cmdvector[0], "cd") == 0) {
            if (chdir(cmdvector[1]) == -1)
                perror("cd"); // 에러 발생
            continue;
        }

        // exit 명령어는 fork()하지 않음
        if (strcmp(cmdvector[0], "exit") == 0)
            break;

        execute(cmd_length);
    }

    return 0;
}

void shell_pipe(int in, int out, char* args[])
{
    pid_t pid;

    switch (pid = fork()) {
    case -1:
        fatal("fork() on pipe");

    case 0:
        break;

    default:
        waitpid(pid, NULL, 0);
        return;
    }

    int fd1, fd2;

    if (in != -1) {
        dup2(pipefd[in][0], 0);
        close(pipefd[in][0]);
        close(pipefd[in][1]);
    } else if (markloc[1].chr == '<') {
        if ((fd1 = open(cmdvector[markloc[1].idx + 1], O_RDONLY)) == -1)
            fatal("input redirection");
        dup2(fd1, STDIN_FILENO);
        close(fd1);
    }

    if (out != -1) {
        dup2(pipefd[out][1], 1);
        close(pipefd[out][0]);
        close(pipefd[out][1]);
    } else if (markloc[mark_cnt - 1].chr == '>') {
        freopen(cmdvector[markloc[mark_cnt - 1].idx + 1], "w", stdout);
    }

    execvp(args[0], args);
    fatal("shell_pipe()");
}

void execute(int cmd_length)
{
    pid_t pid; // Fork한 프로세스의 pid

    // 현재 명령어의 백그라운드 실행 여부
    int background_flag = 0;
    if (strcmp(cmdvector[cmd_length - 1], "&") == 0)
        background_flag = 1;

    switch (pid = fork()) {
    case -1:
        fatal("fork() on execute()");

    case 0:
        // Background Process
        if (background_flag == 1)
            cmdvector[cmd_length-- - 1] = '\0';

        // Pipe
        if (mark_cnt > 0) {

            int curpipe = 0;
            int in = -1;
            int out = -1;

            for (size_t i = 1; i <= pipe_cnt; i++)
                pipe(pipefd[i]);

            for (size_t i = 0; i < mark_cnt; i++) {
                int cmdsize;
                char* cmd[MAX_CMD_ARG];

                in = -1;
                out = -1;

                cmdsize = markloc[i + 1].idx - markloc[i].idx - 1;
                for (size_t j = 0; j < cmdsize; j++)
                    cmd[j] = cmdvector[markloc[i].idx + j + 1];
                cmd[cmdsize] = '\0';

                if (markloc[i].chr == '|' || i == 0) {
                    if (curpipe != 0)
                        in = curpipe;
                    if (curpipe != pipe_cnt)
                        out = curpipe + 1;

                    shell_pipe(in, out, cmd);
                    curpipe++;

                    if (in != -1)
                        close(pipefd[in][0]);
                    if (out != -1)
                        close(pipefd[out][1]);
                }
            }
            exit(0);
        }

        // 일반적인 실행
        execvp(cmdvector[0], cmdvector);
        fatal("execvp()");

    default:
        // Foreground process
        if (background_flag == 0) {
            foreground_pid = pid;
            waitpid(pid, NULL, 0);
            foreground_pid = -1;
        }
    }
}
