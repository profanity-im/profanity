#ifndef MOCK_ACCOUNTS_H
#define MOCK_ACCOUNTS_H

void mock_accounts_get_account(void);
void accounts_get_account_expect_and_return(const char * const name, ProfAccount *account);
void accounts_get_account_return(ProfAccount *account);

void mock_accounts_get_list(void);
void accounts_get_list_return(gchar **accounts);

void mock_accounts_add(void);
void stub_accounts_add(void);
void accounts_add_expect_account_name(char *account_name);

void mock_accounts_enable(void);
void accounts_enable_expect(char *name);
void accounts_enable_return(gboolean result);

void mock_accounts_disable(void);
void accounts_disable_expect(char *name);
void accounts_disable_return(gboolean result);

void mock_accounts_rename(void);
void accounts_rename_expect(char *account_name, char *new_name);
void accounts_rename_return(gboolean result);

void mock_accounts_account_exists(void);
void accounts_account_exists_expect(char *account_name);
void accounts_account_exists_return(gboolean result);

void mock_accounts_set_jid(void);
void stub_accounts_set_jid(void);
void accounts_set_jid_expect(char *account_name, char *jid);

void mock_accounts_set_resource(void);
void stub_accounts_set_resource(void);
void accounts_set_resource_expect(char *account_name, char *resource);

void mock_accounts_set_server(void);
void stub_accounts_set_server(void);
void accounts_set_server_expect(char *account_name, char *server);

void mock_accounts_set_password(void);
void stub_accounts_set_password(void);
void accounts_set_password_expect(char *account_name, char *password);

void mock_accounts_set_muc_service(void);
void stub_accounts_set_muc_service(void);
void accounts_set_muc_service_expect(char *account_name, char *service);

void mock_accounts_set_muc_nick(void);
void stub_accounts_set_muc_nick(void);
void accounts_set_muc_nick_expect(char *account_name, char *nick);

void mock_accounts_set_priorities(void);
void stub_accounts_set_priorities(void);
void accounts_set_priority_online_expect(char *account_name, gint priority);
void accounts_set_priority_chat_expect(char *account_name, gint priority);
void accounts_set_priority_away_expect(char *account_name, gint priority);
void accounts_set_priority_xa_expect(char *account_name, gint priority);
void accounts_set_priority_dnd_expect(char *account_name, gint priority);

void mock_accounts_set_login_presence(void);
void stub_accounts_set_login_presence(void);
void accounts_set_login_presence_expect(char *account_name, char *presence);

void mock_accounts_get_last_presence(void);
void accounts_get_last_presence_return(resource_presence_t presence);

#endif
