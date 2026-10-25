#ifndef QUANTRO_USER_H
#define QUANTRO_USER_H

#include "types.h"

#define MAX_USERS 8

#define USER_ROLE_SYSTEM 0
#define USER_ROLE_ADMIN  1
#define USER_ROLE_USER   2
#define USER_ROLE_GUEST  3

typedef struct {
    int  uid;
    int  role;
    char username[24];
    char password[24];
    char sid[36];
} user_t;

void user_init(void);
const char *user_current_name(void);
int  user_current_role(void);
int  user_login(const char *name, const char *pass);
int  user_add(const char *name, const char *pass, int role);
void user_list(void);
void user_whoami(void);

#endif /* QUANTRO_USER_H */
