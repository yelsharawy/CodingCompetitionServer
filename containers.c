#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <linux/sched.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
// #include <sys/capability.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/prctl.h>
#include "common.h"
#include "containers.h"

// CLONE_NEWPID - new PID namespace (*first child* is the "root process")

// works just like `fork`, but you can specify `CLONE_*` flags!
pid_t clone_fork(u_int64_t flags) {
    // pid_t pid;
    // CHECKNEG1(pid = clone(do_thing, STACK, CLONE_FILES | CLONE_FS | SIGCHLD, 0));
    
    struct clone_args args = { .flags = flags,
        .pidfd = 0, .child_tid = 0, .parent_tid = 0, .exit_signal = SIGCHLD,
        .stack = 0, .stack_size = 0, .tls = 0};
    // int ret;
    // CHECKNEG1(ret = syscall(SYS_clone3, &args, sizeof(args)));
    // printf("pid: %d, ret: %d\n", pid, ret);
    // if (ret == 0) exit(0);
    
    return syscall(SYS_clone3, &args, sizeof(args));
    // if (pid == 0) {
    //     printf("my pid: %d\n", getpid());
    //     exit(21);
    // }
    // printf("pid: %d\n", pid);
    // int wstatus;
    // CHECKNEG1(waitpid(pid, &wstatus, 0));
    // printf("clone exit status: %d\n", WEXITSTATUS(wstatus));
}

void setup_perms() {
    uid_t orig_uid = getuid();
    gid_t orig_gid = getgid();
    CHECKNEG1(unshare(CLONE_NEWNS | CLONE_NEWUSER | CLONE_NEWUTS |
                    CLONE_NEWIPC | CLONE_NEWNET |
                    CLONE_NEWCGROUP));
    // * explanation of flags!
    // * these are actually used by my program:
    // CLONE_NEWNS - new mount namespace
    // CLONE_NEWUSER - new user namespace
    // CLONE_NEWUTS - new "domainname & hostname" namespace
    // * these are done for security purposes:
    // CLONE_NEWIPCS - new IPC (shared memory, semaphores, etc.) namespace
    // CLONE_NEWNET - new network namespace, so no outside communication!
    // * the rest probably don't do much, but just to be safe!
    // CLONE_NEWCGROUP - new cgroup namespace (for limiting resource usage (?))
    
    // set up uid & gid maps
    filename_printf("/proc/self/uid_map", "0 %d 1", orig_uid);
    filename_printf("/proc/self/setgroups", "deny\n");
    filename_printf("/proc/self/gid_map", "0 %d 1", orig_gid);
    // become root of new userspace
    CHECKNEG1(setgid(0));
    CHECKNEG1(setuid(0));
    
    sethostname("container", 9);
    
}

void bind_to_parent(int ppid) {
    CHECKNEG1(prctl(PR_SET_PDEATHSIG, SIGKILL));
    int gppid = getppid();
    CHECKTRUE(gppid == ppid || gppid == 0); // in case the parent already exited
    // if ppid is 0, it will be killed anyway (i think?)
}

// https://linux.die.net/man/2/setrlimit
void limit_resources() {
    #define MB (1024*1024)
    
    CHECKNEG1(setrlimit(RLIMIT_AS, &(struct rlimit){ .rlim_cur = 250*MB, .rlim_max = 250*MB }));
    CHECKNEG1(setrlimit(RLIMIT_CORE, &(struct rlimit){ .rlim_cur = 0, .rlim_max = 0 }));
    CHECKNEG1(setrlimit(RLIMIT_DATA, &(struct rlimit){ .rlim_cur = 250*MB, .rlim_max = 250*MB }));
    CHECKNEG1(setrlimit(RLIMIT_FSIZE, &(struct rlimit){ .rlim_cur = 10*MB, .rlim_max = 10*MB }));
    CHECKNEG1(setrlimit(RLIMIT_STACK, &(struct rlimit){ .rlim_cur = 1*MB, .rlim_max = 1*MB }));
    // CHECKNEG1(setuid(65534));  // "nobody"
    filename_printf("/proc/self/setgroups", "deny\n");
    
    // printf("uid: %d\n", getuid());
    // CHECKNEG1(setgid(0));
    // CHECKNEG1(setuid(0));
    // printf("uid: %d\n", getuid());
}

static char *bind_mounts[] = {"/bin", "/usr", "/lib", "/lib32", "/libx32", "/lib64", "/etc", "/dev", NULL};

// another fork-like function
// the directory at `containerPath` should already exist at the time of calling
// caller is responsible for `flock`-ing the `homeMount` beforehand if desired
// caller is also responsible for calling `chroot` and `limit_resources` on the child
// ( ^ designed this way as to allow any intermediate steps)
// this works similarly to `fork`: both the child and parent will return
// the return value will be `0` for the child, and the pid of the child for the parent
// -1 is returned on failure to fork
pid_t create_container(char *containerPath, char *homeMount) {
    pid_t pid;
    if (!(pid = clone_fork(CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUSER))) {
        char buf[PATH_MAX];
        // printf("containerPath: \"%s\"\n", containerPath);
        strcpy(buf, containerPath);
        char *cur = strchr(buf, 0);
        
        for (char **mnt = bind_mounts; *mnt; mnt++) {
            strcpy(cur, *mnt);
            // printf("mounting: \"%s\"\n", buf);
            CHECKNEG1(mkdir(buf, 0700));
            CHECKNEG1(mount(*mnt, buf, 0, MS_BIND | MS_REC | MS_RDONLY, 0));
        }
        strcpy(cur, "/home");
        CHECKNEG1(mkdir(buf, 0700));
        if (homeMount) {
            CHECKNEG1(mount(homeMount, buf, 0, MS_BIND, 0));
        }
        strcpy(cur, "/proc");
        CHECKNEG1(mkdir(buf, 0700));
        CHECKNEG1(mount("/proc", buf, "proc", 0, 0));
        
        CHECKZERO(clearenv());
        CHECKNEG1(setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", 1));
        CHECKNEG1(setenv("HOME", "/home", 1));
    }
    return pid;
}

void discard_container(char *containerPath) {
    CHECKZERO(WEXITSTATUS(execlp_block("rm", "-rf", "--", containerPath)));
}

// int main() {
//     setup_perms();
    
//     // int pid;
//     // CHECKNEG1(pid = clone_fork(CLONE_NEWPID));
//     // if (!pid) { // CHILD
//     //     printf("shell exit status %d\n", WEXITSTATUS(execlp_block("bash")));
//     //     exit(0);
//     // }
//     // int wstatus;
//     // waitpid(pid, &wstatus, 0);
//     // printf("child exit status: %d\n", WEXITSTATUS(wstatus));
    
//     char dirname[] = "testcontainer_XXXXXX";
//     ;
//     // printf("test thing %\n", 5);
//     int pid;
//     if (!(pid = create_container(mkdtemp(dirname), "homefrom"))) {
//         // printf("im in here!\n");
//         CHECKNEG1(chdir(dirname));
//         CHECKNEG1(chroot("."));
//         limit_resources();
//         exit(execlp("bash", "bash", NULL));
//         // printf("shell exit status %d\n", WEXITSTATUS(execlp_block("bash")));
//     }
//     CHECKNEG1MSG(pid, "failed to create container");
//     // printf("pid: %d\n", pid);
//     // sleep(10);
//     waitpid(pid, 0, 0);
//     printf("discarding container\n");
//     discard_container(dirname);
// }