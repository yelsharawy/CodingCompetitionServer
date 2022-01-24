#pragma once

#include <sys/types.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// my error-checking macros
#define CHECKZEROMSG(value, msg, ...) do {                          \
    if (value) {                                                    \
        fprintf(stderr, "error on "__FILE__ ":"TOSTRING(__LINE__)   \
            " (%s): "msg"\n", __PRETTY_FUNCTION__, ##__VA_ARGS__);  \
        exit(-1);                                                   \
    }                                                               \
} while (0)
#define CHECKTRUEMSG(value, msg, ...) CHECKZEROMSG(!(value), msg, ##__VA_ARGS__)
#define CHECKNEG1MSG(value, msg, ...) CHECKZEROMSG((value) == (typeof(value))-1, msg": %m", ##__VA_ARGS__)
#define CHECKZERO(value) CHECKZEROMSG(value, #value)
#define CHECKTRUE(value) CHECKTRUEMSG(value, #value)
#define CHECKNEG1(value) CHECKNEG1MSG(value, #value)

// executes given `pathname` with `pathname` as first argument, the rest given as varargs
// automatically includes ending `NULL` as well, so you don't have to
// blocks until the program finishes, then returns the `wstatus` yielded by `waitpid`
// uses statement expression GNU extension: https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html
#define execlp_block(pathname, ...) ({                          \
    int pid;                                                    \
    if (!(pid = fork()))                                        \
        exit(execlp(pathname, pathname, ##__VA_ARGS__, NULL));  \
                                                                \
    int wstatus;                                                \
    waitpid(pid, &wstatus, 0);                                  \
    wstatus;                                                    \
})


#define MAX_PLAYERS 255

typedef u_int8_t PlayerCount;
// typedef long long Score;
typedef long double Score;
typedef unsigned long long UserID;

typedef struct UserRankingData {
    UserID userID;
    Score score;
} UserRankingData;

typedef struct UserRanking {
    UserRankingData data;
    struct UserRanking *prev; // `prev` has higher score
    struct UserRanking *next; // `next` has lower score
} UserRanking;


// A xor C, B xor D  -(xor)-> A xor B xor C xor D
// (xor B xor C to both)
// A xor B, C xor D
// (xor A xor B xor C xor D to both)
// ->
// C xor D, A xor B