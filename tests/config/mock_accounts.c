#include <glib.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>

#include "config/accounts.h"

// mocks and stubs

static ProfAccount *
_mock_accounts_get_account(const char * const name)
{
    check_expected(name);
    return (ProfAccount *)mock();
}

gchar **
_mock_accounts_get_list(void)
{
    return (gchar **)mock();
}

void
_mock_accounts_add(const char *account_name, const char *altdomain, const int port)
{
    check_expected(account_name);
    check_expected(altdomain);
}

void
_stub_accounts_add(const char *account_name, const char *altdomain, const int port)
{
    // do nothing
}

static gboolean
_mock_accounts_enable(const char * const name)
{
    check_expected(name);
    return (gboolean)mock();
}

static gboolean
_mock_accounts_disable(const char * const name)
{
    check_expected(name);
    return (gboolean)mock();
}

static gboolean
_mock_accounts_rename(const char * const account_name, const char * const new_name)
{
    check_expected(account_name);
    check_expected(new_name);
    return (gboolean)mock();
}

static gboolean
_mock_accounts_account_exists(const char * const account_name)
{
    check_expected(account_name);
    return (gboolean)mock();
}

static void
_mock_accounts_set_jid(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

static void
_stub_accounts_set_jid(const char * const account_name, const char * const value)
{
    // do nothing
}

static void
_mock_accounts_set_resource(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

static void
_stub_accounts_set_resource(const char * const account_name, const char * const value)
{
    // do nothing
}

static void
_mock_accounts_set_server(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

static void
_stub_accounts_set_server(const char * const account_name, const char * const value)
{
    // do nothing
}

static void
_mock_accounts_set_password(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

static void
_stub_accounts_set_password(const char * const account_name, const char * const value)
{
    // do nothing
}

static void
_mock_accounts_set_muc_service(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

static void
_stub_accounts_set_muc_service(const char * const account_name, const char * const value)
{
    // do nothing
}

static void
_mock_accounts_set_muc_nick(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

static void
_stub_accounts_set_muc_nick(const char * const account_name, const char * const value)
{
    // do nothing
}

static void
_mock_accounts_set_priority_online(const char * const account_name, const gint value)
{
    check_expected(account_name);
    check_expected(value);
}

static void
_stub_accounts_set_priority_online(const char * const account_name, const gint value)
{
    // do nothing
}

static void
_mock_accounts_set_priority_chat(const char * const account_name, const gint value)
{
    check_expected(account_name);
    check_expected(value);
}

static void
_stub_accounts_set_priority_chat(const char * const account_name, const gint value)
{
    // do nothing
}

static void
_mock_accounts_set_priority_away(const char * const account_name, const gint value)
{
    check_expected(account_name);
    check_expected(value);
}

static void
_stub_accounts_set_priority_away(const char * const account_name, const gint value)
{
    // do nothing
}

static void
_mock_accounts_set_priority_xa(const char * const account_name, const gint value)
{
    check_expected(account_name);
    check_expected(value);
}

static void
_stub_accounts_set_priority_xa(const char * const account_name, const gint value)
{
    // do nothing
}

static void
_mock_accounts_set_priority_dnd(const char * const account_name, const gint value)
{
    check_expected(account_name);
    check_expected(value);
}

static void
_stub_accounts_set_priority_dnd(const char * const account_name, const gint value)
{
    // do nothing
}

static void
_mock_accounts_set_login_presence(const char * const account_name, const char * const value)
{
    check_expected(account_name);
    check_expected(value);
}

static void
_stub_accounts_set_login_presence(const char * const account_name, const char * const value)
{
    // do nothing
}

static resource_presence_t
_mock_accounts_get_last_presence(const char * const account_name)
{
    check_expected(account_name);
    return (resource_presence_t)mock();
}

// set up functions

void
mock_accounts_get_account(void)
{
   accounts_get_account = _mock_accounts_get_account;
}

void
mock_accounts_get_list(void)
{
    accounts_get_list = _mock_accounts_get_list;
}

void
mock_accounts_add(void)
{
    accounts_add = _mock_accounts_add;
}

void
stub_accounts_add(void)
{
    accounts_add = _stub_accounts_add;
}

void
mock_accounts_enable(void)
{
    accounts_enable = _mock_accounts_enable;
}

void
mock_accounts_disable(void)
{
    accounts_disable = _mock_accounts_disable;
}

void
mock_accounts_rename(void)
{
    accounts_rename = _mock_accounts_rename;
}

void
mock_accounts_account_exists(void)
{
    accounts_account_exists = _mock_accounts_account_exists;
}

void
mock_accounts_set_jid(void)
{
    accounts_set_jid = _mock_accounts_set_jid;
}

void
stub_accounts_set_jid(void)
{
    accounts_set_jid = _stub_accounts_set_jid;
}

void
mock_accounts_set_resource(void)
{
    accounts_set_resource = _mock_accounts_set_resource;
}

void
stub_accounts_set_resource(void)
{
    accounts_set_resource = _stub_accounts_set_resource;
}

void
mock_accounts_set_server(void)
{
    accounts_set_server = _mock_accounts_set_server;
}

void
stub_accounts_set_server(void)
{
    accounts_set_server = _stub_accounts_set_server;
}

void
mock_accounts_set_password(void)
{
    accounts_set_password = _mock_accounts_set_password;
}

void
stub_accounts_set_password(void)
{
    accounts_set_password  = _stub_accounts_set_password;
}

void
mock_accounts_set_muc_service(void)
{
    accounts_set_muc_service = _mock_accounts_set_muc_service;
}

void
stub_accounts_set_muc_service(void)
{
    accounts_set_muc_service  = _stub_accounts_set_muc_service;
}

void
mock_accounts_set_muc_nick(void)
{
    accounts_set_muc_nick = _mock_accounts_set_muc_nick;
}

void
stub_accounts_set_muc_nick(void)
{
    accounts_set_muc_nick  = _stub_accounts_set_muc_nick;
}

void
mock_accounts_set_priorities(void)
{
    accounts_set_priority_online = _mock_accounts_set_priority_online;
    accounts_set_priority_chat = _mock_accounts_set_priority_chat;
    accounts_set_priority_away = _mock_accounts_set_priority_away;
    accounts_set_priority_xa = _mock_accounts_set_priority_xa;
    accounts_set_priority_dnd = _mock_accounts_set_priority_dnd;
}

void
stub_accounts_set_priorities(void)
{
    accounts_set_priority_online = _stub_accounts_set_priority_online;
    accounts_set_priority_chat = _stub_accounts_set_priority_chat;
    accounts_set_priority_away = _stub_accounts_set_priority_away;
    accounts_set_priority_xa = _stub_accounts_set_priority_xa;
    accounts_set_priority_dnd = _stub_accounts_set_priority_dnd;
}

void
mock_accounts_set_login_presence(void)
{
    accounts_set_login_presence = _mock_accounts_set_login_presence;
}

void
stub_accounts_set_login_presence(void)
{
    accounts_set_login_presence = _stub_accounts_set_login_presence;
}

void
mock_accounts_get_last_presence(void)
{
    accounts_get_last_presence = _mock_accounts_get_last_presence;
}

// mock behaviours

void
accounts_get_account_expect_and_return(const char * const name, ProfAccount *account)
{
    expect_string(_mock_accounts_get_account, name, name);
    will_return(_mock_accounts_get_account, account);
}

void
accounts_get_account_return(ProfAccount *account)
{
    expect_any(_mock_accounts_get_account, name);
    will_return(_mock_accounts_get_account, account);
}

void
accounts_get_list_return(gchar **accounts)
{
    will_return(_mock_accounts_get_list, accounts);
}

void
accounts_add_expect_account_name(char *account_name)
{
    expect_any(_mock_accounts_add, altdomain);
    expect_string(_mock_accounts_add, account_name, account_name);
}

void
accounts_enable_expect(char *name)
{
    expect_string(_mock_accounts_enable, name, name);
    will_return(_mock_accounts_enable, TRUE);
}

void
accounts_enable_return(gboolean result)
{
    expect_any(_mock_accounts_enable, name);
    will_return(_mock_accounts_enable, result);
}

void
accounts_disable_expect(char *name)
{
    expect_string(_mock_accounts_disable, name, name);
    will_return(_mock_accounts_disable, TRUE);
}

void
accounts_disable_return(gboolean result)
{
    expect_any(_mock_accounts_disable, name);
    will_return(_mock_accounts_disable, result);
}

void
accounts_rename_expect(char *account_name, char *new_name)
{
    expect_string(_mock_accounts_rename, account_name, account_name);
    expect_string(_mock_accounts_rename, new_name, new_name);
    will_return(_mock_accounts_rename, TRUE);
}

void
accounts_rename_return(gboolean result)
{
    expect_any(_mock_accounts_rename, account_name);
    expect_any(_mock_accounts_rename, new_name);
    will_return(_mock_accounts_rename, result);
}

void
accounts_account_exists_expect(char *account_name)
{
    expect_string(_mock_accounts_account_exists, account_name, account_name);
    will_return(_mock_accounts_account_exists, TRUE);
}

void
accounts_account_exists_return(gboolean result)
{
    expect_any(_mock_accounts_account_exists, account_name);
    will_return(_mock_accounts_account_exists, result);
}

void
accounts_set_jid_expect(char *account_name, char *jid)
{
    expect_string(_mock_accounts_set_jid, account_name, account_name);
    expect_string(_mock_accounts_set_jid, value, jid);
}

void
accounts_set_resource_expect(char *account_name, char *resource)
{
    expect_string(_mock_accounts_set_resource, account_name, account_name);
    expect_string(_mock_accounts_set_resource, value, resource);
}

void
accounts_set_server_expect(char *account_name, char *server)
{
    expect_string(_mock_accounts_set_server, account_name, account_name);
    expect_string(_mock_accounts_set_server, value, server);
}

void
accounts_set_password_expect(char *account_name, char *password)
{
    expect_string(_mock_accounts_set_password, account_name, account_name);
    expect_string(_mock_accounts_set_password, value, password);
}

void
accounts_set_muc_service_expect(char *account_name, char *service)
{
    expect_string(_mock_accounts_set_muc_service, account_name, account_name);
    expect_string(_mock_accounts_set_muc_service, value, service);
}

void
accounts_set_muc_nick_expect(char *account_name, char *nick)
{
    expect_string(_mock_accounts_set_muc_nick, account_name, account_name);
    expect_string(_mock_accounts_set_muc_nick, value, nick);
}

void
accounts_set_priority_online_expect(char *account_name, gint priority)
{
    expect_string(_mock_accounts_set_priority_online, account_name, account_name);
    expect_value(_mock_accounts_set_priority_online, value, priority);
}

void
accounts_set_priority_chat_expect(char *account_name, gint priority)
{
    expect_string(_mock_accounts_set_priority_chat, account_name, account_name);
    expect_value(_mock_accounts_set_priority_chat, value, priority);
}

void
accounts_set_priority_away_expect(char *account_name, gint priority)
{
    expect_string(_mock_accounts_set_priority_away, account_name, account_name);
    expect_value(_mock_accounts_set_priority_away, value, priority);
}

void
accounts_set_priority_xa_expect(char *account_name, gint priority)
{
    expect_string(_mock_accounts_set_priority_xa, account_name, account_name);
    expect_value(_mock_accounts_set_priority_xa, value, priority);
}

void
accounts_set_priority_dnd_expect(char *account_name, gint priority)
{
    expect_string(_mock_accounts_set_priority_dnd, account_name, account_name);
    expect_value(_mock_accounts_set_priority_dnd, value, priority);
}

void
accounts_set_login_presence_expect(char *account_name, char *presence)
{
    expect_string(_mock_accounts_set_login_presence, account_name, account_name);
    expect_string(_mock_accounts_set_login_presence, value, presence);
}

void
accounts_get_last_presence_return(resource_presence_t presence)
{
    expect_any(_mock_accounts_get_last_presence, account_name);
    will_return(_mock_accounts_get_last_presence, presence);
}
