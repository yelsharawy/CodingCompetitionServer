#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "common.h"
#include "games.h"
#include "containers.h"
#include "submissions.h"
#include "reranker.h"
#include "users.h"

void swapfds(int fd1, int fd2) {
    int origfd1 = dup(fd1);
    CHECKNEG1(dup2(fd2, fd1));
    CHECKNEG1(dup2(origfd1, fd2));
    CHECKNEG1(close(origfd1));
}

void printhelp() {
    /* Commands:
     *  --setup                 sets up `server_data` folder in current directory
     *  -i file name            install game from .tar under `name`
     *  -a username             add user, prints generated id
     *  -f username             find user, prints id
     *  -s user file game       submit program for game under user (either id or name accepted)
     *  -p game [user ...]      play game between users
     *  -l game                 list lovely leaderboard
     *  -r game numplayers n    run `n` random matches of `game` with `numplayers` players each
     */
    int ret = puts(
"Commands:\n"
" --setup                 sets up `server_data` folder in current directory\n"
" -i file name            install game from .tar under `name`\n"
" -a username             add user, prints generated id\n"
" -f username             find user, prints id\n"
" -s user file game       submit program for game under user (either id or name accepted)\n"
" -p game [user ...]      play game between users\n"
" -l game                 list lovely leaderboard\n"
" -r game numplayers n    run `n` random matches of `game` with `numplayers` players each\n"
"Example:\n"
" ./ccs --setup\n"
" ./ccs -i tic-tac-toe.tar ttt\n"
" ./ccs -a player1\n"
" ./ccs -s player1 random_player.tar ttt\n"
" ./ccs -a player2\n"
" ./ccs -s player2 random_player.tar ttt\n"
" ./ccs -l ttt\n"
" ./ccs -p ttt player1 player2\n"
" ./ccs -l ttt\n"
" ./ccs -r ttt 2 100 2> /dev/null\n"
" ./ccs -l ttt\n"
"Failure to use a command properly will result in an error.\n"
"Not all errors have \"nice\" messages, especially not PEBMAC errors. (sarcasm)"
    );
    CHECKNEG1(ret);
    exit(0);
}

int main(int argc, char **argv) {
    srand(time(NULL));
    
    if (argc <= 1) printhelp();
    
    if (!strcmp(argv[1], "--setup")) {
        CHECKZERO(mkdir("./server_data", 0755) && errno != EEXIST);
        CHECKZERO(mkdir("./server_data/users", 0755) && errno != EEXIST);
        CHECKZERO(mkdir("./server_data/games", 0755) && errno != EEXIST);
        printf("successfully set up server_data\n");
        exit(0);
    }
    if (!strcmp(argv[1], "-i")) {
        CHECKTRUE(argc > 3);
        char install_path[PATH_MAX] = "./server_data/games/";
        strcat(install_path, argv[3]);
        CHECKZERO(mkdir(install_path, 0755) == -1 && errno != EEXIST);
        // printf("installing in \"%s\"...\n", install_path);
        setup_perms();
        install_tar(argv[2], install_path);
        printf("successfully installed %s\n", argv[3]);
        exit(0);
    }
    if (!strcmp(argv[1], "-a")) {
        CHECKTRUE(argc > 2);
        CHECKZEROMSG(argv[2][0] == 0, "username cannot be empty");
        CHECKZEROMSG(argv[2][0] >= '0' && argv[2][0] <= '9', "username cannot start with number");
        UserID id;
        if (find_username(argv[2], &id)) {
            fprintf(stderr, "\"%s\" already registered with id "USERID_SPEC"\n", argv[2], id);
            exit(1);
        }
        id = new_user();
        UserData data;
        strcpy(data.username, argv[2]);
        set_userdata(id, &data);
        printf(USERID_SPEC"\n", id);
        // printf("successfully registered user %s\n", argv[2]);
        exit(0);
    }
    if (!strcmp(argv[1], "-f")) {
        CHECKTRUE(argc > 2);
        UserID id;
        if (find_username(argv[2], &id)) {
            printf(USERID_SPEC"\n", id);
            exit(0);
        }
        fprintf(stderr, "username not found\n");
        exit(1);
    }
    if (!strcmp(argv[1], "-s")) {
        CHECKTRUE(argc > 4);
        UserID id;
        if (!find_id_or_name(argv[2], &id)) {
            fprintf(stderr, "user \"%s\" does not exist\n", argv[2]);
            exit(1);
        }
        char *file = argv[3];
        char *game = argv[4];
        char *fullPath;
        // TODO: CHECK THAT GAME EXISTS
        asprintf(&fullPath, USERS_DIR USERID_SPEC "/%s", id, game);
        CHECKZERO(mkdir(fullPath, 0755) && errno != EEXIST);
        setup_perms();
        install_tar(file, fullPath);
        
        // add to rankings with default score of 400
        char *gameRankings;
        asprintf(&gameRankings, "./server_data/games/%s/rankings", game);
        // printf("gonna load\n");
        load_rankings(gameRankings);
        // printf("loaded: %d\n", getpid());
        add_ranking_sorted((UserRankingData){.score = 400, .userID = id});
        // printf("added!\n");
        save_rankings(gameRankings);
        // printf("saved!\n");
        free_rankings();
        // printf("freed!\n");
        
        printf("successfully submitted program!\n");
        exit(0);
    }
    if (!strcmp(argv[1], "-p")) {
        // TODO: CHECK THAT GAME EXISTS
        CHECKTRUE(argc > 2);
        PlayerCount playercount = argc - 3;
        UserID ids[playercount];
        for (int i = 0; i < playercount; i++) {
            if (!find_id_or_name(argv[3 + i], &ids[i])) {
                fprintf(stderr, "user \"%s\" does not exist\n", argv[3+i]);
                exit(1);
            }
            for (int j = 0; j < i; j++) {
                if (ids[i] == ids[j]) {
                    fprintf(stderr, "a user cannot compete against itself\n");
                    exit(1);
                }
            }
        }
        setup_perms();
        
        int p[2];
        CHECKNEG1(pipe(p));
        // close(p[0]);
        swapfds(STDOUT_FILENO, p[1]);
        playgame(argv[2], playercount, ids);
        fflush(stdout); // to ensure any printed stuff is actually printed before swap
        fsync(STDOUT_FILENO);
        swapfds(STDOUT_FILENO, p[1]);
        
        // TODO: UPDATE GAME LEADERBOARD
        swapfds(STDIN_FILENO, p[0]);
        char *gameRankings;
        asprintf(&gameRankings, "./server_data/games/%s/rankings", argv[2]);
        load_rankings(gameRankings);
        rankonce();
        save_rankings(gameRankings);
        free_rankings();
        
        swapfds(STDIN_FILENO, p[0]);
        
        close(p[0]);
        close(p[1]);
        
        printf("completed!\n");
        exit(0);
    }
    if (!strcmp(argv[1], "-l")) {
        CHECKTRUE(argc > 2);
        char *gameRankings;
        asprintf(&gameRankings, "./server_data/games/%s/rankings", argv[2]);
        load_rankings(gameRankings);
        print_rankings();
        free_rankings();
        exit(0);
    }
    if (!strcmp(argv[1], "-r")) {
        CHECKTRUE(argc > 4);
        int p[2];
        CHECKNEG1(pipe(p));
        
        setup_perms();
        char *gameRankings;
        asprintf(&gameRankings, "./server_data/games/%s/rankings", argv[2]);
        
        int numSubmissions = load_rankings(gameRankings);
        UserID ids[numSubmissions]; // GNU extension
        { // get list of players submitted to this game
            UserRanking *cur = ranker_root;
            for (int i = 0; i < numSubmissions; i++) {
                ids[i] = cur->data.userID;
                cur = cur->next;
            }
        }
        free_rankings(gameRankings);
        
        int rerankerpid;
        if (!(rerankerpid = fork())) {
            CHECKNEG1(close(p[1]));
            swapfds(STDIN_FILENO, p[0]);
            rankerloop(gameRankings);
            exit(0);
        }
        CHECKNEG1MSG(rerankerpid, "fork failed");
        CHECKNEG1(close(p[0]));
        
        swapfds(STDOUT_FILENO, p[1]);
        PlayerCount playercount = atoi(argv[3]);
        int gameCount = atoi(argv[4]);
        for (int i = 0; i < gameCount; i++) {
            /*
            int pid;
            if (!(pid = fork())) {
                */
                // pick 2 random players from `ids`
                fprintf(stderr, "chosen players:\n");
                UserID chosen[playercount]; // GNU extension
                for (int j = 0; j < playercount; j++) {
                    chooseagain:
                    chosen[j] = ids[rand() % numSubmissions];
                    for (int k = 0; k < j; k++) {
                        if (chosen[j] == chosen[k]) goto chooseagain;
                    }
                    fprintf(stderr, " "USERID_SPEC"\n", chosen[j]);
                }
                playgame(argv[2], playercount, chosen);
                fflush(stdout);
                fsync(STDOUT_FILENO);
                /*
                exit(0);
            }
            CHECKNEG1MSG(pid, "fork failed");
            */ // cannot fork, because children can eternally block on `flock`
            // and i don't have the time to solve this
        }
        swapfds(STDOUT_FILENO, p[1]);
        CHECKNEG1(close(p[1]));
        // printf("waiting for reranker...\n");
        CHECKNEG1(waitpid(rerankerpid, 0, 0));
        printf("complete!\n");
        exit(0);
    }
    fprintf(stderr, "unknown command \"%s\"\n", argv[1]);
    printhelp();
    exit(1);
}