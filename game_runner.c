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
#include "common.h"
#include "containers.h"
#include "submissions.h"

#define TEMPLATE "running_XXXXXX"

#define starts_with(str, lit) (!strncmp(str, lit, sizeof(lit)-1))

struct player {
    UserID userID;
    char *submission_dir;
};

void playgame(char *gameProg, int numplayers, char **playerProgs) {
    int ppid = getpid();
    int playerpids[numplayers];
    int pid;
    const int templatelen = sizeof(TEMPLATE);
    char gameDir[PATH_MAX] = TEMPLATE;
    fprintf(stderr, "created temporary dir: %s\n", mkdtemp(gameDir));
    gameDir[templatelen-1] = '/';
    char bufIn[PATH_MAX];
    char bufOut[PATH_MAX];
    strncpy(bufIn, gameDir, templatelen);
    strncpy(bufOut, gameDir, templatelen);
    char *curIn = bufIn + templatelen;
    strcpy(curIn, "in"); curIn += 2;
    char *curOut = bufOut + templatelen;
    strcpy(curOut, "out"); curOut += 3;
    for (int i = 0; i < numplayers; i++) {
        sprintf(curIn, "%d", i);
        sprintf(curOut, "%d", i);
        printf("creating: %s\n", bufIn);
        CHECKNEG1(mkfifo(bufIn, 0600));
        printf("creating: %s\n", bufOut);
        CHECKNEG1(mkfifo(bufOut, 0600));
        if (!(playerpids[i] = fork())) { // CHILD
            int in = open(bufIn, O_RDONLY | O_NONBLOCK);
            fcntl(in, F_SETFL, O_RDONLY);
            int out = open(bufOut, O_WRONLY);
            printf("starting player %d!\n", i);
            CHECKNEG1(dup2(in, STDIN_FILENO));
            CHECKNEG1(dup2(out, STDOUT_FILENO));
            CHECKNEG1(close(in));
            CHECKNEG1(close(out));
            run_submission(playerProgs[i]);
        }
        CHECKNEG1MSG(playerpids[i], "failed to clone player child %d", i);
        //     CHECKNEG1(prctl(PR_SET_PDEATHSIG, SIGTERM)); // kill child when parent exits
        //     CHECKTRUE(getppid() == ppid); // in case the parent already exited
        //     execlp(playerProgs[i], playerProgs[i], NULL);
        //     perror("execlp"); exit(1); // should never be reached
        // }
    }
    
    int fromgame[2];
    int togame[2];
    CHECKNEG1(pipe(fromgame));
    CHECKNEG1(pipe(togame));
    if (!(pid = fork())) { // CHILD
        fprintf(stderr, "got here!\n");
        CHECKNEG1(prctl(PR_SET_PDEATHSIG, SIGTERM)); // kill child when parent exits
        CHECKTRUE(getppid() == ppid); // in case the parent already exited
        fprintf(stderr, "got here 2!\n");
        
        dup2(togame[0], STDIN_FILENO);    // game reads from `togame` as stdin
        dup2(fromgame[1], STDOUT_FILENO); // game writes to `fromgame` as stdout
        // ^ for debugging purposes, this line may be commented out
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
    fprintf(stderr, "wrote it!\n");
    for (int i = 0; i < numplayers; i++) {
        dprintf(togame[1], "%sin%d\n", gameDir, i);
        dprintf(togame[1], "%sout%d\n", gameDir, i);
    }
    // dprintf(togame[1], "you should not see this.\n");
    fprintf(stderr, "done writing pipes too!\n");
    char c;
    // int ret;
    FILE *fg;
    CHECKTRUE(fg = fdopen(fromgame[0], "r"));
    char *line;
    size_t n;
    while (1) {
        line = NULL;
        n = 0;
        size_t len;
        CHECKNEG1(len = getline(&line, &n, fg));
        if (starts_with(line, "end")) break;
        free(line);
    }
    for (int i = 0; i < numplayers; i++) kill(playerpids[i], SIGKILL);
    kill(pid, SIGTERM);
    CHECKNEG1(execlp_block("rm", "-rf", "--", gameDir));
    
    unsigned long long results[numplayers];
    for (int i = 0; i < numplayers; i++) {
        if (fscanf(fg, "%llu", &results[i]) != 1) CHECKZEROMSG(1, "scanf failed: %m");
        fprintf(stderr, "results[%d] = %llu\n", i, results[i]);
    }
    // while (read(fromgame[0], &c, 1) == 1 && c != 0 && c != EOF) {
    //     char *line;
    //     scanf
    //     // fprintf(stderr, "got c: %c\n", c);
    //     // putchar(c);  // TIME LIMITS UNIMPLEMENTED
    // }
    // fflush(stdout);
    // fprintf(stderr, "ret: %d\n", ret);
    // fprintf(stderr, "c: %d\n", c);
    // int wstatus;
    // waitpid(pid, &wstatus, 0);
    // fprintf(stderr, "wstatus: %x\n", wstatus);
    
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("invalid args\n");
        exit(-1);
    }
    setup_perms();
    int numplayers = argc - 2;
    playgame(argv[1], numplayers, argv+2);
}