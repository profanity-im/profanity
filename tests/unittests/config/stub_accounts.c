#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "common.h"
#include "config/account.h"

void accounts_load(void) {}
void accounts_close(void) {}

char * accounts_find_all(char *prefix)
{
    return NULL;
}

char * accounts_find_enabled(char *prefix)
{
    return NULL;
}

void accounts_reset_all_search(void) {}
void accounts_reset_enabled_search(void) {}

void accounts_add(const char *jid, const char *altdomain, const int port)
{
    check_expected(jid);
    check_expected(altdomain);
    check_expected(port);
}

int  accounts_remove(const char *jid)
{
    return 0;
}

gchar** accounts_get_list(void)
{
    return (gchar **)mock();
}

ProfAccount* accounts_get_account(const char * const name)
{
    check_expected(name);
    return (ProfAccount*)mock();
}

gboolean accounts_enable(const char * const name)
{
    check_expected(name);
    return (gboolean)mock();
}

gboolean accounts_disable(const char * const name)
{
    check_expected(name);
    return (gboolean)mock();
}

gboolean accounts_rename(const char * const account_name,
    const char * const new_name)
{
    check_expected(account_name);
    check_expected(new_name);
    return (gboolean)mock();
}

gboolean accounts_account_exists(const char * const account_name)
{
    check_expected(account_name);
    return (gboolean)mock();
}

void accounts_set_jid(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

void accounts_set_server(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

void accounts_set_port(const char * const account_name, const int value) {}

void accounts_set_resource(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

void accounts_set_password(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

void accounts_set_eval_password(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

void accounts_set_muc_service(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

void accounts_set_muc_nick(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

void accounts_set_otr_policy(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

void accounts_set_last_presence(const char * const account_name, const char * const value) {}
void accounts_set_last_status(const char * const account_name, const char * const value) {}
void accounts_set_last_activity(const char * const account_name) {}
void accounts_set_pgp_keyid(const char * const account_name, const char * const value) {}
void accounts_set_script_start(const char * const account_name, const char * const value) {}
void accounts_set_theme(const char * const account_name, const char * const value) {}
void accounts_set_tls_policy(const char * const account_name, const char * const value) {}

void accounts_set_login_presence(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

resource_presence_t accounts_get_login_presence(const char * const account_name)
{
    return RESOURCE_ONLINE;
}

char * accounts_get_last_status(const char * const account_name)
{
    return NULL;
}

resource_presence_t accounts_get_last_presence(const char * const account_name)
{
    check_expected(account_name);
    return (resource_presence_t)mock();
}

void accounts_set_priority_online(const char * const account_name, const gint value)
{
    check_expected(account_name);
    check_expected(value);
}

void accounts_set_priority_chat(const char * const account_name, const gint value)
{
    check_expected(account_name);
    check_expected(value);
}

void accounts_set_priority_away(const char * const account_name, const gint value)
{
    check_expected(account_name);
    check_expected(value);
}

void accounts_set_priority_xa(const char * const account_name, const gint value)
{
    check_expected(account_name);
    check_expected(value);
}

void accounts_set_priority_dnd(const char * const account_name, const gint value)
{
    check_expected(account_name);
    check_expected(value);
}

void accounts_set_priority_all(const char * const account_name, const gint value) {}
gint accounts_get_priority_for_presence_type(const char * const account_name,
    resource_presence_t presence_type)
{
    return 0;
}

void accounts_clear_password(const char * const account_name) {}
void accounts_clear_eval_password(const char * const account_name) {}
void accounts_clear_server(const char * const account_name) {}
void accounts_clear_port(const char * const account_name) {}
void accounts_clear_otr(const char * const account_name) {}
void accounts_clear_pgp_keyid(const char * const account_name) {}
void accounts_clear_script_start(const char * const account_name) {}
void accounts_clear_theme(const char * const account_name) {}
void accounts_clear_muc(const char * const account_name) {}
void accounts_clear_resource(const char * const account_name) {}
void accounts_add_otr_policy(const char * const account_name, const char * const contact_jid, const char * const policy) {}
char* accounts_get_last_activity(const char *const account_name)
{
    return NULL;
}
