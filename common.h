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
#define CHECKZERO(value) CHECKZEROMSG(value, "expected 0: "#value)
#define CHECKTRUE(value) CHECKTRUEMSG(value, "failed assertion: "#value)
#define CHECKNEG1(value) CHECKNEG1MSG(value, #value)

// Macro to easily open a file, print contents inside, and close it
// I could've made this a function with `vprintf`,
// but this way I get warnings if arguments don't match the format
#define filename_printf(filename, ...) do {         \
    int ___fd;                                                              \
    CHECKNEG1(___fd = open(filename, O_WRONLY));                            \
    CHECKTRUEMSG(dprintf(___fd, __VA_ARGS__) >= 0, "dprintf failed: %m");   \
    CHECKNEG1(close(___fd));                                                \
} while (0)

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
typedef long long unsigned int UserID;


// A xor C, B xor D  -(xor)-> A xor B xor C xor D
// (xor B xor C to both)
// A xor B, C xor D
// (xor A xor B xor C xor D to both)
// ->
// C xor D, A xor B