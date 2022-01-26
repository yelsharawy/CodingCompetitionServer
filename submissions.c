#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include "common.h"
#include "containers.h"
#include "submissions.h"

static int childpid = -1;

static void signal_handler(int s) {
    if (s == SIGTERM) {
        if (childpid != -1) kill(childpid, SIGKILL);
    }
}

// #define make() execlp_block("make", "--silent")
// #define make_run() execlp_block("make", "--silent", "run")
// #define make_clean() execlp_block("make", "--silent" "clean")

void install_tar(char *zipfile, char *submission_folder) {
    CHECKTRUEMSG(signal(SIGTERM, signal_handler) != SIG_ERR, "failed to set signal handler: %m");
    int ppid = getpid();
    char containerPath[] = "/tmp/build_XXXXXX";
    int fd;
    CHECKNEG1(fd = open(submission_folder, O_DIRECTORY | O_CLOEXEC));
    CHECKNEG1(flock(fd, LOCK_EX)); // lock submission folder
    // unzip
    CHECKZERO(execlp_block("tar", "-xf", zipfile, "--no-same-owner", "-C", submission_folder));
    // build in container
    int pid;
    if (!(pid = create_container(mkdtemp(containerPath), submission_folder))) {
        bind_to_parent(ppid);
        CHECKNEG1(chroot(containerPath));
        CHECKNEG1(chdir("/home"));
        limit_resources();
        
        // char dbg[PATH_MAX];
        // getcwd(dbg, PATH_MAX-1);
        // printf("%s\n", dbg);
        // exit(execlp("bash", "bash", NULL)); // for testing
        exit(execlp("make", "make", "--silent", NULL));
    }
    childpid = pid;
    CHECKNEG1MSG(pid, "failed to create container");
    CHECKNEG1(waitpid(pid, 0, 0));
    // fprintf(stderr, "discarding container!\n");
    discard_container(containerPath);
    
    CHECKNEG1(close(fd)); // releases lock
}

void run_submission(char *submission_folder) {
    CHECKTRUEMSG(signal(SIGTERM, signal_handler) != SIG_ERR, "failed to set signal handler: %m");
    int ppid = getpid();
    char containerPath[] = "/tmp/run_XXXXXX";
    int fd;
    // fprintf(stderr, "given submission folder: \"%s\"\n", submission_folder);
    CHECKNEG1(fd = open(submission_folder, O_DIRECTORY | O_CLOEXEC));
    CHECKNEG1(flock(fd, LOCK_EX)); // lock submission folder
    
    // run in container
    int pid;
    if (!(pid = create_container(mkdtemp(containerPath), submission_folder))) {
        bind_to_parent(ppid);
        CHECKNEG1(chroot(containerPath));
        CHECKNEG1(chdir("/home"));
        limit_resources();
        // exit(execlp("bash", "bash", NULL)); // for testing
        exit(execlp("make", "make", "--silent", "run", NULL));
    }
    childpid = pid;
    CHECKNEG1MSG(pid, "failed to create container");
    CHECKNEG1(waitpid(pid, 0, 0));
    // fprintf(stderr, "discarding container!\n");
    discard_container(containerPath);
    
    CHECKNEG1(close(fd)); // releases lock
}

// no `make clean` cuz im running out of time :(

// int main(int argc, char **argv) {
//     // printf("%d\n", argc);
//     CHECKTRUE(argc >= 3);
//     setup_perms();
//     install_tar(argv[1], argv[2]);
//     // make();
//     // make_run();
// }