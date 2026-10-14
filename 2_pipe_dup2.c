/*
 * 2_pipe_dup2.c — 셸의 "ls | wc -l" 을 직접 만들어 본다
 *
 * [핵심 개념]
 *   dup2(a, b) 는 디스크립터 b 가 a 와 같은 곳을 가리키게 만든다.
 *   자식의 표준출력(1)을 파이프의 쓰기 끝으로 바꿔치기하면
 *   그 프로그램은 아무것도 몰라도 출력이 파이프로 흘러간다.
 *   출처: man 2 dup2, man 2 pipe
 *
 * [컴파일·실행]
 *   gcc -Wall -Wextra -o 2_pipe_dup2 2_pipe_dup2.c
 *   ./2_pipe_dup2 ls -l : wc -l
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    int fd[2];   /* fd[0] = 읽기 끝, fd[1] = 쓰기 끝 */
    int i;

    /* ':' 위치를 찾는다 */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == ':' && argv[i][1] == '\0')
            break;
    }

    /* 명령어가 올바르게 입력되었는지 확인한다 */
    if (i == argc || i == 1 || i == argc - 1) {
        fprintf(stderr, "사용법: %s 명령어1 인자... : 명령어2 인자...\n",
                argv[0]);
        return 1;
    }

    char **cmd2 = &argv[i + 1];  /* 두 번째 명령어의 시작 위치 */
    argv[i] = NULL;              /* ':' 위치에서 첫 번째 명령어를 끝낸다 */

    if (pipe(fd) == -1) { perror("pipe"); return 1; }

    /* 첫 번째 자식: 첫 번째 명령어를 실행하고 출력을 파이프로 보낸다 */
    if (fork() == 0) {
        close(fd[0]);                 /* 읽기 끝은 쓰지 않는다 */
        dup2(fd[1], STDOUT_FILENO);   /* 표준출력을 파이프 쓰기 끝으로 교체 */
        close(fd[1]);                 /* 원본 디스크립터는 이제 필요 없다 */
        execvp(argv[1], &argv[1]);    /* 입력받은 첫 번째 명령어 실행 */
        perror("execvp command1");    /* exec가 실패했을 때만 여기 도달 */
        _exit(127);
    }

    /* 두 번째 자식: 두 번째 명령어를 실행하고 입력을 파이프에서 받는다 */
    if (fork() == 0) {
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);    /* 표준입력을 파이프 읽기 끝으로 교체 */
        close(fd[0]);
        execvp(cmd2[0], cmd2);        /* 입력받은 두 번째 명령어 실행 */
        perror("execvp command2");
        _exit(127);
    }

    /* 부모는 양쪽 다 닫아야 한다. 안 닫으면 wc가 EOF를 못 봐서 끝나지 않을 수 있다. */
    close(fd[0]);
    close(fd[1]);

    wait(NULL);   /* 자식 두 개가 종료될 때까지 기다린다 */
    wait(NULL);
    return 0;
}
