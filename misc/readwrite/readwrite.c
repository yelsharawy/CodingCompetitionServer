#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("requires exactly 2 arguments\n");
        return -1;
    }
    if (fork()) { // parent (writing)
        int writeto = open(argv[2], O_WRONLY);
        while (1) {
            char *line = NULL;
            size_t len = 0;
            size_t readcount = getline(&line, &len, stdin);
            
            write(writeto, line, readcount);
            
            free(line);
        }
    } else { // child (reading)
        int readfrom = open(argv[1], O_RDONLY);
        char c;
        while (read(readfrom, &c, 1) == 1) {
            putchar(c);
        }
    }
}