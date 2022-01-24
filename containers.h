#include <sys/types.h>

void setup_perms();

pid_t clone_fork(u_int64_t flags);

pid_t create_container(char *containerPath, char *homeMount);

void bind_to_parent(int ppid);

void limit_resources();

void discard_container(char *containerPath);