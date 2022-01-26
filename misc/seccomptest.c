#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

#define X32_SYSCALL_BIT 0x40000000
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

void setup_seccomp(int syscall_nr, int t_arch, int f_errno) {
    // prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT);
    int ret = prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);
    printf("prctl: %d\t%m\n", ret);
    unsigned int upper_nr_limit = 0xffffffff;
    if (t_arch == AUDIT_ARCH_X86_64)
        upper_nr_limit = X32_SYSCALL_BIT - 1;
    struct sock_filter filter[] = {
        /* [0] Load architecture from 'seccomp_data' buffer into
                accumulator. */
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS,
                (offsetof(struct seccomp_data, arch))),

        /* [1] Jump forward 5 instructions if architecture does not
                match 't_arch'. */
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, t_arch, 0, 5),

        /* [2] Load system call number from 'seccomp_data' buffer into
                accumulator. */
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS,
                (offsetof(struct seccomp_data, nr))),

        /* [3] Check ABI - only needed for x86-64 in deny-list use
                cases.  Use BPF_JGT instead of checking against the bit
                mask to avoid having to reload the syscall number. */
        BPF_JUMP(BPF_JMP | BPF_JGT | BPF_K, upper_nr_limit, 3, 0),

        /* [4] Jump forward 1 instruction if system call number
                does not match 'syscall_nr'. */
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, syscall_nr, 0, 1),

        /* [5] Matching architecture and system call: don't execute
            the system call, and return 'f_errno' in 'errno'. */
        BPF_STMT(BPF_RET | BPF_K,
                SECCOMP_RET_ERRNO | (f_errno & SECCOMP_RET_DATA)),

        /* [6] Destination of system call number mismatch: allow other
                system calls. */
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),

        /* [7] Destination of architecture mismatch: kill process. */
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),
    };
    struct sock_fprog prog = {
        .len = ARRAY_SIZE(filter),
        .filter = filter,
    };
    ret = syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &prog);
    printf("seccomp ret: %d\t%m\n", ret);
    
    // syscall(SYS_seccomp, SECCOMP_SET_MODE_STRICT, 0, 0);
}

int main() {
    int a = open("a", O_RDWR | O_CREAT, 0666);
    printf("before! %d\n", a);
    // setup_seccomp();
    setup_seccomp(SYS_open, AUDIT_ARCH_X86_64, EAGAIN);
    printf("after 1\n");
    int e = execlp("java", "java", "Main", NULL);
    printf("execlp: %d\t%m\n", e);
    // execlp("ls", "ls", ".", NULL);
}