#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define FATAL(s) do {               \
    perror(TOSTRING(__LINE__) s);   \
    exit(-1);                       \
} while(0)

#define SCAN_PATH(s) scanf("%"TOSTRING(PATH_MAX)"s", s)

char p1stdin[PATH_MAX+1];
char p1stdout[PATH_MAX+1];
char p2stdin[PATH_MAX+1];
char p2stdout[PATH_MAX+1];

char board[3][3] = {
    "...",
    "...",
    "..."
};

void debug_printboard() {
    fprintf(stderr, "%.3s\n%.3s\n%.3s\n", board[0], board[1], board[2]);
}

// returns 'X' if X won, 'O' if 'O' won, 0 otherwise
char winner() {
    // check rows
    for (int r = 0; r < 3; r++) {
        if (board[r][0] == 'X' && board[r][1] == 'X' && board[r][2] == 'X') return 'X';
        if (board[r][0] == 'O' && board[r][1] == 'O' && board[r][2] == 'O') return 'O';
    }
    // check cols
    for (int c = 0; c < 3; c++) {
        if (board[0][c] == 'X' && board[1][c] == 'X' && board[2][c] == 'X') return 'X';
        if (board[0][c] == 'O' && board[1][c] == 'O' && board[2][c] == 'O') return 'O';
    }
    // check diagonals
    if (board[0][0] == 'X' && board[1][1] == 'X' && board[2][2] == 'X') return 'X';
    if (board[0][0] == 'O' && board[1][1] == 'O' && board[2][2] == 'O') return 'O';
    if (board[0][2] == 'X' && board[1][1] == 'X' && board[2][0] == 'X') return 'X';
    if (board[0][2] == 'O' && board[1][1] == 'O' && board[2][0] == 'O') return 'O';
    
    return 0;
}

ssize_t dgetline(char **lineptr, size_t *n, int fd) {
    if (*lineptr == NULL && *n == 0) {
        *n = 100;
        *lineptr = malloc(*n);
    }
    char *cur = *lineptr;
    char c;
    while (read(fd, &c, 1) == 1) {
        if (c == 0 || c == EOF) break;
        if (cur - *lineptr >= *n - 1) {
            *n *= 2;
            int l = cur - *lineptr;
            *lineptr = realloc(*lineptr, *n);
            cur = l + *lineptr;
        }
        *(cur++) = c;
        if (c == '\n') break;
    }
    *cur = 0;
    return cur - *lineptr;
}

// WARNING: scans *exactly* one line from `fd` at a time, no more, no less
int dscanf(int fd, const char *restrict format, ...) {
    va_list varargs;
    va_start(varargs, format);
    
    char *line = NULL;
    size_t size = 0;
    size_t actual = dgetline(&line, &size, fd);
    fprintf(stderr, "read line len %ld\n", actual);
    
    int ret = vsscanf(line, format, varargs);
    va_end(varargs);
    return ret;
}

// typedef FILE *ftype;
// #define RD_OPEN(filename) fopen(filename, "r")
// #define WR_OPEN(filename) fopen(filename, "w")
// #define WRITE_LIT(file, x) fputs(x, file)
typedef int ftype;
#define RD_OPEN(filename) ({            \
    printf("opening %s...\n", filename);\
    open(filename, O_RDONLY);           \
})
#define WR_OPEN(filename) ({            \
    printf("opening %s...\n", filename);\
    open(filename, O_WRONLY);           \
})
#define WRITE_LIT(fd, x) write(fd, x, sizeof(x)-1)

void Xwins() {
    printf(
        "end\n"
        "1\n"
        "-1\n"
    );
    exit(0);
}

void Owins() {
    printf(
        "end\n"
        "-1\n"
        "1\n"
    );
    exit(0);
}

void draw() {
    printf(
        "end\n"
        "0\n"
        "0\n"
    );
    exit(0);
}

void game_loop(ftype to1, ftype from1, ftype to2, ftype from2) {
    printf("%d, %d, %d, %d\n", to1, from1, to2, from2);
    // tell each player whether they're X or O
    WRITE_LIT(to1, "X\n");
    WRITE_LIT(to2, "O\n");
    
    debug_printboard();
    
    int r = -1, c = -1;
    int turns = 0;
    while (1) {
        // player 0's turn
        printf("turn 0\n");             // tells which player(s) are expected to respond
        if (dscanf(from1, "%d,%d\n", &r, &c) != 2) Owins();
        printf("frame\n");              // tells when the expected player(s) have responded
        fprintf(stderr, "got coords %d,%d\n", r, c);
        if (r < 0 || r > 2 || c < 0 || c > 2) Owins();
        if (board[r][c] != '.') Owins();
        board[r][c] = 'X';
        debug_printboard();
        if (winner() == 'X') Xwins();
        dprintf(to2, "%d,%d\n", r, c);
        
        turns++;
        if (turns == 9) draw();
        
        printf("turn 1\n");
        if (dscanf(from2, "%d,%d\n", &r, &c) != 2) Xwins();
        printf("frame\n");
        fprintf(stderr, "got coords %d,%d\n", r, c);
        if (r < 0 || r > 2 || c < 0 || c > 2) Xwins();
        if (board[r][c] != '.') Xwins();
        board[r][c] = 'O';
        debug_printboard();
        if (winner() == 'O') Owins();
        dprintf(to1, "%d,%d\n", r, c);
        
        turns++;
    }
}

int main() {
    u_int32_t num_players;
    if (scanf("%ud", &num_players) == EOF) FATAL("scanf");
    if (num_players != 2) exit(-1);
    
    SCAN_PATH(p1stdin);
    SCAN_PATH(p1stdout);
    SCAN_PATH(p2stdin);
    SCAN_PATH(p2stdout);
    // printf("received:\n%s\n%s\n%s\n%s\n",
    //     p1stdin, p1stdout, p2stdin, p2stdout);
    int in1 = WR_OPEN(p1stdin);
    int out1 = RD_OPEN(p1stdout);
    int in2 = WR_OPEN(p2stdin);
    int out2 = RD_OPEN(p2stdout);
    fprintf(stderr, "starting!\n");
    game_loop(in1, out1, in2, out2);
}