#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/syscall.h>

int main(int argc, char** argv) {
    pid_t pid = fork();
    switch (pid) {
        case -1: /* error */
            printf("%s", strerror(errno));
        case 0:  /* child */
            ptrace(PTRACE_TRACEME, 0, 0, 0);
            printf("before\n");
            execvp(argv[1], argv + 1);
            printf("after\n");
            printf("%s\n", strerror(errno));
            exit(-1);
    }
    waitpid(pid, 0, 0);
    ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL);
    
    while (1) {
        ptrace(PTRACE_SYSCALL, pid, 0, 0);
        waitpid(pid, 0, 0);
        
        struct user_regs_struct regs;
        ptrace(PTRACE_GETREGS, pid, 0, &regs);
        long syscall = regs.orig_rax;
        fprintf(stderr, "%ld(%ld, %ld, %ld, %ld, %ld, %ld)\n",
                syscall,
                (long)regs.rdi, (long)regs.rsi, (long)regs.rdx,
                (long)regs.r10, (long)regs.r8,  (long)regs.r9);
        if (syscall == 231) break;
    }
}