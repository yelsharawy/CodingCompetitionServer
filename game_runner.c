// #define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <signal.h>
// #include <ftw.h>

#define TEMPLATE "running_XXXXXX"

void playgame(char *gameProg, int numplayers, char** playerProgs) {
    int ppid = getpid();
    int pid;
    const int templatelen = sizeof(TEMPLATE);
    char gameDir[PATH_MAX] = TEMPLATE;
    printf("created temporary dir: %s\n", mkdtemp(gameDir));
    gameDir[templatelen-1] = '/';
    char bufIn[PATH_MAX];
    char bufOut[PATH_MAX];
    strncpy(bufIn, gameDir, templatelen); // double check arg order
    strncpy(bufOut, gameDir, templatelen); // double check arg order
    char *curIn = bufIn + templatelen;
    strcpy(curIn, "in"); curIn += 2;
    char *curOut = bufOut + templatelen;
    strcpy(curOut, "out"); curOut += 3;
    for (int i = 0; i < numplayers; i++) {
        sprintf(curIn, "%d", i); // double check arg order
        sprintf(curOut, "%d", i); // double check arg order
        printf("creating: %s\n", bufIn);
        mkfifo(bufIn, 0600);
        printf("creating: %s\n", bufOut);
        mkfifo(bufOut, 0600);
        if (!(pid = fork())) { // CHILD
            int r = prctl(PR_SET_PDEATHSIG, SIGTERM); // kill child when parent exits
            if (r == -1) { perror("prctl"); exit(1); }
            if (getppid() != ppid) exit(1); // in case the parent already exited
            
            int in = open(bufIn, O_RDONLY | O_NONBLOCK);
            fcntl(in, F_SETFL, O_RDONLY);
            int out = open(bufOut, O_WRONLY);
            printf("starting player %d!\n", i);
            dup2(in, STDIN_FILENO);
            dup2(out, STDOUT_FILENO);
            close(in);
            close(out);
            execlp(playerProgs[i], playerProgs[i], NULL);
            perror("execlp"); exit(1); // should never be reached
        }
    }
    
    int fromgame[2];
    int togame[2];
    pipe(fromgame);
    pipe(togame);
    if (!(pid = fork())) { // CHILD
        fprintf(stderr, "got here!\n");
        int r = prctl(PR_SET_PDEATHSIG, SIGTERM); // kill child when parent exits
        if (r == -1) { perror("prctl"); exit(1); }
        if (getppid() != ppid) exit(1); // in case the parent already exited
        fprintf(stderr, "got here 2!\n");
        
        dup2(togame[0], STDIN_FILENO);    // game reads from `togame` as stdin
        // dup2(fromgame[1], STDOUT_FILENO); // game writes to `fromgame` as stdout
        // ^ for debugging purposes, stdout is not redirected
        close(togame[0]);
        close(togame[1]);
        close(fromgame[0]);
        close(fromgame[1]);
        fprintf(stderr, "got here 3!\n");
        
        execlp(gameProg, gameProg, NULL);
        perror("execlp"); exit(1); // should never be reached
    }
    close(togame[0]);
    close(fromgame[1]);
    
    dprintf(togame[1], "%d\n", numplayers);
    printf("wrote it!\n");
    for (int i = 0; i < numplayers; i++) {
        dprintf(togame[1], "%sin%d\n", gameDir, i);
        dprintf(togame[1], "%sout%d\n", gameDir, i);
    }
    // dprintf(togame[1], "you should not see this.\n");
    printf("done writing pipes too!\n");
    // char c;
    // while (read(fromgame[0], &c, 1) == 1 && c != 0 && c != EOF) {
    //     putchar(c);
    // }
    int wstatus;
    waitpid(pid, &wstatus, 0);
    printf("wstatus: %x\n", wstatus);
    
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("invalid args\n");
        exit(-1);
    }
    int numplayers = argc - 2;
    playgame(argv[1], numplayers, argv+2);
}