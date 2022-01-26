#pragma once
#include <limits.h>
#include "common.h"

#define USERS_DIR "./server_data/users/"

#define USERID_SPEC "%016llx"
#define USERID_LEN 16

typedef struct UserData {
    char username[NAME_MAX+1]; // using from limits.h
} UserData;

char *idtoa(UserID id);

UserID atoid(char *str);

// char *userID_path(UserID id);

// char *userDataPath(UserID id);

UserID new_user();

int user_exists(UserID id);

int get_userdata(UserID id, UserData *data);

int set_userdata(UserID id, UserData *data);

int find_username(char *username, UserID *out);

int find_id_or_name(char *id_or_name, UserID *out);
