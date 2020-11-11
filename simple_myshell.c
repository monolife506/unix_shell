#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#define MAX_CMD_ARG 10
#define BUFSIZ 256

const char *prompt = "myshell> ";
char *cmdvector[MAX_CMD_ARG];
char cmdline[BUFSIZ];

void fatal(char *str)
{
  perror(str);
  exit(1);
}

int makelist(char *s, const char *delimiters, char **list, int MAX_LIST)
{
  int i = 0;
  int numtokens = 0;
  char *snew = NULL;

  if ((s == NULL) || (delimiters == NULL))
    return -1;

  snew = s + strspn(s, delimiters); /* Skip delimiters */
  if ((list[numtokens] = strtok(snew, delimiters)) == NULL)
    return numtokens;

  numtokens = 1;

  while (1)
  {
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

int main(int argc, char **argv)
{
  int i = 0; // argument의 개수
  pid_t pid, child_pid;

  while (1)
  {
    fputs(prompt, stdout);
    fgets(cmdline, BUFSIZ, stdin);
    cmdline[strlen(cmdline) - 1] = '\0';
    i = makelist(cmdline, " \t", cmdvector, MAX_CMD_ARG);

    // 명령어가 입력되지 않은 경우
    if (i == 0)
      continue;

    // cd 명령어는 fork()하지 않음
    if (strcmp(cmdvector[0], "cd") == 0)
    {
      cd();
      continue;
    }

    // exit 명령어는 fork()하지 않음
    if (strcmp(cmdvector[0], "exit") == 0)
      break;

    switch (pid = fork())
    {
    case 0:
      if (strcmp(cmdvector[i - 1], "&") == 0) // Background Process
      {
        cmdvector[i-- - 1] = '\0';
        child_pid = fork();
        if (child_pid != 0)
        {
          waitpid(-1, NULL, WNOHANG);
          exit(0);
        }
      }

      execvp(cmdvector[0], cmdvector);
      fatal("main()");
    case -1:
      fatal("main()");
    default:
      wait(NULL);
    }
  }

  return 0;
}
