#ifndef MOCK_XMPP_H
#define MOCK_XMPP_H

#include "xmpp/xmpp.h"

void mock_connection_status(jabber_conn_status_t status);
void mock_connection_account_name(char *name);
void mock_connection_presence_message(char *message);
void expect_room_list_request(char *conf_server);

void mock_jabber_connect_with_details(void);
void jabber_connect_with_details_expect_and_return(char *jid,
    char *password, char *altdomain, int port, jabber_conn_status_t result);
void jabber_connect_with_details_return(jabber_conn_status_t result);

void mock_jabber_connect_with_account(void);
void jabber_connect_with_account_expect_and_return(ProfAccount *account,
    jabber_conn_status_t result);
void jabber_connect_with_account_return(jabber_conn_status_t result);

void mock_presence_update(void);
void presence_update_expect(resource_presence_t presence, char *msg, int idle);

void bookmark_get_list_returns(GList *bookmarks);

void mock_bookmark_add(void);
void expect_and_return_bookmark_add(char *expected_jid, char *expected_nick,
    gboolean expected_autojoin, gboolean added);

void mock_bookmark_remove(void);
void expect_and_return_bookmark_remove(char *expected_jid, gboolean expected_autojoin,
    gboolean removed);

void message_send_expect(char *message, char *recipient);

void mock_presence_join_room(void);
void presence_join_room_expect(char *room, char *nick, char *passwd);

void mock_roster_send_add_new(void);
void roster_send_add_new_expect(char *jid, char *nick);

void mock_roster_send_remove(void);
void roster_send_remove_expect(char *jid);

void mock_roster_send_name_change(void);
void roster_send_name_change_expect(char *jid, char *name, GSList *groups);

#endif
