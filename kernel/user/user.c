#include "user.h"
#include "uart.h"
#include "kernel.h"

static user_t users[MAX_USERS];
static int user_count = 0;
static int current_user_idx = 1; /* Varsayilan Administrator */

void user_init(void)
{
    user_count = 0;

    /* SYSTEM */
    users[0].uid = 0;
    users[0].role = USER_ROLE_SYSTEM;
    strncpy(users[0].username, "SYSTEM", 23);
    strncpy(users[0].password, "system", 23);
    strncpy(users[0].sid, "S-1-5-18", 35);
    user_count++;

    /* Administrator */
    users[1].uid = 500;
    users[1].role = USER_ROLE_ADMIN;
    strncpy(users[1].username, "Administrator", 23);
    strncpy(users[1].password, "admin123", 23);
    strncpy(users[1].sid, "S-1-5-21-362381-500", 35);
    user_count++;

    /* QuantroUser */
    users[2].uid = 1000;
    users[2].role = USER_ROLE_USER;
    strncpy(users[2].username, "QuantroUser", 23);
    strncpy(users[2].password, "quantro", 23);
    strncpy(users[2].sid, "S-1-5-21-362381-1000", 35);
    user_count++;

    uart_puts("[user] Windows 11 SAM / Security Accounts Manager yuklendi (NT SID & ACL ready)\r\n");
}

const char *user_current_name(void)
{
    return users[current_user_idx].username;
}

int user_current_role(void)
{
    return users[current_user_idx].role;
}

int user_login(const char *name, const char *pass)
{
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, name) == 0) {
            if (strcmp(users[i].password, pass) == 0) {
                current_user_idx = i;
                uart_printf("[user] '%s' basariyla oturum acti.\r\n", name);
                return 0;
            } else {
                uart_puts("[user] Hatali parola!\r\n");
                return -2;
            }
        }
    }
    uart_puts("[user] Kullanici bulunamadi.\r\n");
    return -1;
}

int user_add(const char *name, const char *pass, int role)
{
    if (user_count >= MAX_USERS)
        return -1;

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, name) == 0)
            return -2;
    }

    int idx = user_count++;
    users[idx].uid = 1000 + idx;
    users[idx].role = role;
    strncpy(users[idx].username, name, 23);
    users[idx].username[23] = '\0';
    strncpy(users[idx].password, pass, 23);
    users[idx].password[23] = '\0';
    ksnprintf(users[idx].sid, 35, "S-1-5-21-362381-%d", users[idx].uid);

    uart_printf("[user] Yeni kullanici olusturuldu: %s (UID: %d)\r\n", name, users[idx].uid);
    return 0;
}

void user_whoami(void)
{
    const char *role_str[] = { "SYSTEM", "Administrators", "Users", "Guests" };
    uart_printf("QUANTRO-PC\\%s (SID: %s, Grup: %s)\r\n",
                users[current_user_idx].username,
                users[current_user_idx].sid,
                role_str[users[current_user_idx].role]);
}

void user_list(void)
{
    uart_puts("\r\n UID   Kullanici Adi       Rol              Guvenlik Kimligi (SID)\r\n");
    uart_puts(" -------------------------------------------------------------------\r\n");
    const char *role_str[] = { "SYSTEM", "Administrators", "Users", "Guests" };
    for (int i = 0; i < user_count; i++) {
        uart_printf(" %-5d %-18s %-16s %s\r\n",
                    users[i].uid,
                    users[i].username,
                    role_str[users[i].role],
                    users[i].sid);
    }
}
