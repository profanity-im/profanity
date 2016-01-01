#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "xmpp/xmpp.h"

// connection functions
void jabber_init(void) {}

jabber_conn_status_t jabber_connect_with_details(const char * const jid,
    const char * const passwd, const char * const altdomain, const int port, const char *const tls_policy)
{
    check_expected(jid);
    check_expected(passwd);
    check_expected(altdomain);
    check_expected(port);
    return (jabber_conn_status_t)mock();
}

jabber_conn_status_t jabber_connect_with_account(const ProfAccount * const account)
{
    check_expected(account);
    return (jabber_conn_status_t)mock();
}

void jabber_disconnect(void) {}
void jabber_shutdown(void) {}
void jabber_process_events(int millis) {}
const char * jabber_get_fulljid(void)
{
    return (char *)mock();
}

const char * jabber_get_domain(void)
{
    return NULL;
}

char* jabber_create_uuid(void)
{
    return NULL;
}

void jabber_free_uuid(char * uuid) {}

jabber_conn_status_t jabber_get_connection_status(void)
{
    return (jabber_conn_status_t)mock();
}

char* jabber_get_presence_message(void)
{
    return (char*)mock();
}

char* jabber_get_account_name(void)
{
    return (char*)mock();
}

GList * jabber_get_available_resources(void)
{
    return NULL;
}

// message functions
char* message_send_chat(const char * const barejid, const char * const msg)
{
    check_expected(barejid);
    check_expected(msg);
    return NULL;
}

char* message_send_chat_otr(const char * const barejid, const char * const msg)
{
    check_expected(barejid);
    check_expected(msg);
    return NULL;
}

char* message_send_chat_pgp(const char * const barejid, const char * const msg)
{
    return NULL;
}

void message_send_private(const char * const fulljid, const char * const msg) {}
void message_send_groupchat(const char * const roomjid, const char * const msg) {}
void message_send_groupchat_subject(const char * const roomjid, const char * const subject) {}

void message_send_inactive(const char * const barejid) {}
void message_send_composing(const char * const barejid) {}
void message_send_paused(const char * const barejid) {}
void message_send_gone(const char * const barejid) {}

void message_send_invite(const char * const room, const char * const contact,
    const char * const reason) {}

// presence functions
void presence_subscription(const char * const jid, const jabber_subscr_t action) {}

GSList* presence_get_subscription_requests(void)
{
    return NULL;
}

gint presence_sub_request_count(void)
{
    return 0;
}

void presence_reset_sub_request_search(void) {}

char * presence_sub_request_find(const char * const search_str)
{
    return  NULL;
}

void presence_join_room(char *room, char *nick, char * passwd)
{
    check_expected(room);
    check_expected(nick);
    check_expected(passwd);
}

void presence_change_room_nick(const char * const room, const char * const nick) {}
void presence_leave_chat_room(const char * const room_jid) {}

void presence_send(resource_presence_t status, const char * const msg, int idle, char *signed_status)
{
    check_expected(status);
    check_expected(msg);
    check_expected(idle);
    check_expected(signed_status);
}

gboolean presence_sub_request_exists(const char * const bare_jid)
{
    return FALSE;
}

// iq functions
void iq_disable_carbons() {};
void iq_enable_carbons() {};
void iq_send_software_version(const char * const fulljid) {}

void iq_room_list_request(gchar *conferencejid)
{
    check_expected(conferencejid);
}

void iq_disco_info_request(gchar *jid) {}
void iq_disco_items_request(gchar *jid) {}
void iq_set_autoping(int seconds) {}
void iq_confirm_instant_room(const char * const room_jid) {}
void iq_destroy_room(const char * const room_jid) {}
void iq_request_room_config_form(const char * const room_jid) {}
void iq_submit_room_config(const char * const room, DataForm *form) {}
void iq_room_config_cancel(const char * const room_jid) {}
void iq_send_ping(const char * const target) {}
void iq_send_caps_request(const char * const to, const char * const id,
    const char * const node, const char * const ver) {}
void iq_send_caps_request_for_jid(const char * const to, const char * const id,
    const char * const node, const char * const ver) {}
void iq_send_caps_request_legacy(const char * const to, const char * const id,
    const char * const node, const char * const ver) {}
void iq_room_info_request(const char * const room, gboolean display) {}
void iq_room_affiliation_list(const char * const room, char *affiliation) {}
void iq_room_affiliation_set(const char * const room, const char * const jid, char *affiliation,
    const char * const reason) {}
void iq_room_kick_occupant(const char * const room, const char * const nick, const char * const reason) {}
void iq_room_role_set(const char * const room, const char * const nick, char *role,
    const char * const reason) {}
void iq_room_role_list(const char * const room, char *role) {}
void iq_last_activity_request(gchar *jid) {}
void iq_autoping_check(void) {}

// caps functions
Capabilities* caps_lookup(const char * const jid)
{
    return NULL;
}

void caps_close(void) {}
void caps_destroy(Capabilities *caps) {}

gboolean bookmark_add(const char *jid, const char *nick, const char *password, const char *autojoin_str)
{
    check_expected(jid);
    check_expected(nick);
    check_expected(password);
    check_expected(autojoin_str);
    return (gboolean)mock();
}

gboolean bookmark_update(const char *jid, const char *nick, const char *password, const char *autojoin_str)
{
    return FALSE;
}

gboolean bookmark_remove(const char *jid)
{
    check_expected(jid);
    return (gboolean)mock();
}

gboolean bookmark_join(const char *jid)
{
    return FALSE;
}

const GList * bookmark_get_list(void)
{
    return (GList *)mock();
}

char * bookmark_find(const char * const search_str)
{
    return NULL;
}

void bookmark_autocomplete_reset(void) {}

void roster_send_name_change(const char * const barejid, const char * const new_name, GSList *groups)
{
    check_expected(barejid);
    check_expected(new_name);
    check_expected(groups);
}

void roster_send_add_to_group(const char * const group, PContact contact) {}
void roster_send_remove_from_group(const char * const group, PContact contact) {}

void roster_send_add_new(const char * const barejid, const char * const name)
{
    check_expected(barejid);
    check_expected(name);
}

void roster_send_remove(const char * const barejid)
{
    check_expected(barejid);
}
