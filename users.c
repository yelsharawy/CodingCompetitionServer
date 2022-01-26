#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include "common.h"
#include "users.h"

#define randomUserID() (((u_int64_t)rand() << 32) | rand())

// returns newly allocated string, must be freed by caller
char *idtoa(UserID id) {
    char *result;
    CHECKNEG1MSG(asprintf(&result, USERID_SPEC, id), "asprintf failed");
    return result;
}

// char *userID_path(UserID id) {
//     char *ret = malloc(sizeof(USERS_DIR) + USERID_LEN);
//     strcpy(ret, USERS_DIR);
//     sprintf(ret+sizeof(USERS_DIR)-1, USERID_SPEC, id);
//     return ret;
// }

// char *userDataPath(UserID id) {
//     char *ret = malloc(sizeof(USERS_DIR) + USERID_LEN + sizeof("data"));
//     strcpy(ret, USERS_DIR);
//     sprintf(ret+sizeof(USERS_DIR)-1, USERID_SPEC, id);
//     strcpy(ret+sizeof(USERS_DIR)-1, USERID_SPEC, id);
//     strcpy(ret+sizeof(USERS_DIR)+USERID_LEN, "/data");
//     return ret;
// }

UserID atoid(char *str) {
    UserID id;
    CHECKTRUEMSG(sscanf(str, "%016llx", &id) == 1, "sscanf failed");
    return id;
}

// creates user folder for a new user and returns its id
UserID new_user() {
    UserID id;
    do {
        id = randomUserID();
        char *path;
        CHECKNEG1(asprintf(&path, USERS_DIR USERID_SPEC, id));
        errno = 0;
        mkdir(path, 0755);
        free(path);
    } while (errno == EEXIST);
    CHECKTRUEMSG(errno == 0, "mkdir failed: %m");
    return id;
}

int user_exists(UserID id) {
    char *path;
    asprintf(&path, USERS_DIR USERID_SPEC, id);
    struct stat st;
    int result = (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
    free(path);
    return result;
}

// returns 0 on failure, 1 on success
int find_username(char *username, UserID *out) {
    DIR *users;
    CHECKTRUE(users = opendir(USERS_DIR));
    UserID found;
    UserData data;
    struct dirent *entry;
    errno = 0;
    while (entry = readdir(users)) {
        if (entry->d_name[0] == '.') continue;
        // printf("entry->d_name =\"%s\"\n", entry->d_name);
        sscanf(entry->d_name, USERID_SPEC, &found);
        // printf(USERID_SPEC"\n", found);
        if (get_userdata(found, &data) == 0 && !strcmp(data.username, username)) {
            CHECKNEG1(closedir(users));
            *out = found;
            return 1;
        }
        errno = 0;
    }
    CHECKZEROMSG(errno, "readdir failed");
    CHECKNEG1(closedir(users));
    return 0;
}

// returns non-0 on success (id stored in *out)
// otherwise, returns 0, `*out` is undefined
int find_id_or_name(char *id_or_name, UserID *out) {
    int r;
    return ((r = sscanf(id_or_name, USERID_SPEC, out)) == 1
            && user_exists(*out))
            || (r != 1 && find_username(id_or_name, out));
}

int get_userdata(UserID id, UserData *data) {
    char *path;
    CHECKNEG1(asprintf(&path, USERS_DIR USERID_SPEC "/data", id));
    int fd = open(path, O_RDONLY | O_CREAT, 0744);
    free(path);
    if (fd == -1) return -1;
    int ret;
    CHECKNEG1(ret = read(fd, data, sizeof(UserData)));
    if (ret < sizeof(UserData)) ((char*)data)[ret] = 0;
    // ^ just in case if there is no terminating 0 in the file
    CHECKNEG1(close(fd));
    return 0;
}

int set_userdata(UserID id, UserData *data) {
    char *path;
    CHECKNEG1(asprintf(&path, USERS_DIR USERID_SPEC "/data", id));
    int fd = open(path, O_WRONLY | O_CREAT, 0744);
    if (fd == -1) return -1;
    CHECKTRUE(write(fd, data, sizeof(UserData)) == sizeof(UserData));
    free(path);
    return 0;
}

// int main() {
//     srand(time(NULL));
//     printf(USERID_SPEC"\n", new_user());
// }