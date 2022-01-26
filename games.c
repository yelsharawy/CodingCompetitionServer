#define _GNU_SOURCE
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
#include <errno.h>
// #include <ftw.h>
#include "common.h"
#include "containers.h"
#include "users.h"
#include "submissions.h"
#include "games.h"

#define TEMPLATE "/tmp/running_XXXXXX"

#define starts_with(str, lit) (!strncmp(str, lit, sizeof(lit)-1))

// struct player {
//     UserID userID;
//     char *submission_dir;
// };

void playgame(char *game, PlayerCount numplayers, UserID *players) {
    int ppid = getpid();
    int playerpids[numplayers];
    int pid;
    const int templatelen = sizeof(TEMPLATE);
    char gameDir[PATH_MAX] = TEMPLATE;
    
    CHECKNEG1(mkdtemp(gameDir));
    // fprintf(stderr, "created temporary dir: %s\n", mkdtemp(gameDir));
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
        // fprintf(stderr, "creating: %s\n", bufIn);
        CHECKNEG1(mkfifo(bufIn, 0600));
        // fprintf(stderr, "creating: %s\n", bufOut);
        CHECKNEG1(mkfifo(bufOut, 0600));
        if (!(playerpids[i] = fork())) { // CHILD
            int in = open(bufIn, O_RDONLY | O_NONBLOCK);
            fcntl(in, F_SETFL, O_RDONLY);
            int out = open(bufOut, O_WRONLY);
            // fprintf(stderr, "starting player %d!\n", i);
            CHECKNEG1(dup2(in, STDIN_FILENO));
            CHECKNEG1(dup2(out, STDOUT_FILENO));
            CHECKNEG1(close(in));
            CHECKNEG1(close(out));
            char *submissionDir;
            asprintf(&submissionDir, USERS_DIR USERID_SPEC "/%s", players[i], game);
            run_submission(submissionDir);
            free(submissionDir);
            exit(0);
        }
        CHECKNEG1MSG(playerpids[i], "failed to clone player child %d", i);
    }
    
    int fromgame[2];
    int togame[2];
    CHECKNEG1(pipe(fromgame));
    CHECKNEG1(pipe(togame));
    if (!(pid = fork())) { // CHILD
        // fprintf(stderr, "got here!\n");
        CHECKNEG1(prctl(PR_SET_PDEATHSIG, SIGTERM)); // kill child when parent exits
        CHECKTRUE(getppid() == ppid); // in case the parent already exited
        // fprintf(stderr, "got here 2!\n");
        
        dup2(togame[0], STDIN_FILENO);    // game reads from `togame` as stdin
        dup2(fromgame[1], STDOUT_FILENO); // game writes to `fromgame` as stdout
        // ^ for debugging purposes, this line may be commented out
        close(togame[0]);
        close(togame[1]);
        close(fromgame[0]);
        close(fromgame[1]);
        // fprintf(stderr, "got here 3!\n");
        char *gamePath;
        asprintf(&gamePath, "./server_data/games/%s", game);
        // run_submission(gamePath); // no can do with current implementation
        // cuz named pipes outside of container are inaccessible
        chdir(gamePath);
        free(gamePath);
        exit(WEXITSTATUS(execlp_block("make", "--silent", "run")));
        // execlp(gameProg, gameProg, NULL);
        // perror("execlp"); exit(1); // should never be reached
    }
    close(togame[0]);
    close(fromgame[1]);
    
    dprintf(togame[1], "%d\n", numplayers);
    // fprintf(stderr, "wrote it!\n");
    for (int i = 0; i < numplayers; i++) {
        dprintf(togame[1], "%sin%d\n", gameDir, i);
        dprintf(togame[1], "%sout%d\n", gameDir, i);
    }
    // dprintf(togame[1], "you should not see this.\n");
    // fprintf(stderr, "done writing pipes too!\n");
    char c;
    // int ret;
    FILE *fg;
    CHECKTRUE(fg = fdopen(fromgame[0], "r"));
    char *line;
    size_t n;
    char foundEnd = 0;
    while (1) {
        line = NULL;
        n = 0;
        errno = 0;
        size_t len = getline(&line, &n, fg);
        CHECKZEROMSG(errno, "getline failed\n");
        if (len == -1) break;
        if (starts_with(line, "end")) {
            foundEnd = 1;
            break;
        }
        free(line);
    }
    for (int i = 0; i < numplayers; i++) kill(playerpids[i], SIGTERM);
    kill(pid, SIGTERM);
    CHECKNEG1(execlp_block("rm", "-rf", "--", gameDir));
    if (foundEnd) {
        char message[4096]; // i assume this'll always be big enough
        char *cur = message;
        unsigned long long results[numplayers];
        for (int i = 0; i < numplayers; i++) {
            if (fscanf(fg, "%llu", &results[i]) != 1) CHECKZEROMSG(1, "scanf failed: %m");
            // fprintf(stderr, "results[%d] = %llu\n", i, results[i]);
        }
        cur += sprintf(cur, "%d\n", numplayers);
        for (int i = 0; i < numplayers; i++) {
            cur += sprintf(cur, USERID_SPEC" %llu\n", players[i], results[i]); // TODO: REPLACE WITH ACTUAL USER ID
        }
        write(STDOUT_FILENO, message, cur - message);
    } else {
        fprintf(stderr, "game exited without reporting end of game\n");
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

// int main(int argc, char** argv) {
//     if (argc < 2) {
//         printf("invalid args\n");
//         exit(-1);
//     }
//     setup_perms();
//     int numplayers = argc - 2;
//     playgame(argv[1], numplayers, argv+2);
// }