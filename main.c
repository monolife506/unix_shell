#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "handler.h"

#define MAX_CMD_ARG 10

const char* prompt = "myshell> ";
char* cmdvector[MAX_CMD_ARG];
char cmdline[BUFSIZ];

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

void cd()
{
    if (chdir(cmdvector[1]) == -1)
        perror("cd"); // 에러 발생
}

int main(int argc, char** argv)
{
    int i = 0; // argument의 개수
    int background_flag = 0; // 현재 명령어의 백그라운드 실행 여부
    pid_t pid;

    sethandler();

    while (1) {
        fputs(prompt, stdout);
        fgets(cmdline, BUFSIZ, stdin);
        cmdline[strlen(cmdline) - 1] = '\0';
        i = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);

        // 명령어가 입력되지 않은 경우
        if (i == 0)
            continue;

        // cd 명령어는 fork()하지 않음
        if (strcmp(cmdvector[0], "cd") == 0) {
            cd();
            continue;
        }

        // exit 명령어는 fork()하지 않음
        if (strcmp(cmdvector[0], "exit") == 0)
            break;

        // Background 실행 여부 판단
        if (strcmp(cmdvector[i - 1], "&") == 0)
            background_flag = 1;
        else
            background_flag = 0;

        switch (pid = fork()) {
        case 0:
            if (background_flag == 1) // Background Process
                cmdvector[i-- - 1] = '\0';

            setpgid(0, 0); // 새 프로세스 그룹
            execvp(cmdvector[0], cmdvector);
            fatal("main()");
        case -1:
            fatal("main()");
        default:
            // Foreground process
            if (background_flag == 0) {
                foreground_pgid = pid;
                waitpid(foreground_pgid, NULL, 0);
                foreground_pgid = -1;
            }
        }
    }
    return 0;
}
