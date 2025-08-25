#include "prof_cmocka.h"

#include "xmpp/xmpp.h"

// connection functions
void
session_init(void)
{
}
void
session_init_activity(void)
{
}
void
session_check_autoaway(void)
{
}

jabber_conn_status_t
session_connect_with_details(const char* const jid,
                             const char* const passwd, const char* const altdomain, const int port, const char* const tls_policy,
                             const char* const auth_policy)
{
    check_expected(jid);
    check_expected(passwd);
    check_expected(altdomain);
    check_expected(port);
    return mock_type(jabber_conn_status_t);
}

jabber_conn_status_t
session_connect_with_account(const ProfAccount* const account)
{
    check_expected(account);
    return mock_type(jabber_conn_status_t);
}

void
session_reconnect_now(void)
{
}
void
session_disconnect(void)
{
}
void
session_process_events(void)
{
}
void
connection_disconnect(void)
{
}
const Jid*
connection_get_jid(void)
{
    return mock_ptr_type(Jid*);
}
const char*
connection_get_fulljid(void)
{
    return mock_ptr_type(char*);
}

const char*
connection_get_barejid(void)
{
    return mock_ptr_type(const char*);
}

gboolean
equals_our_barejid(const char* cmp)
{
    return TRUE;
}

const char*
connection_get_user(void)
{
    return mock_ptr_type(const char*);
}

const char*
connection_get_domain(void)
{
    return NULL;
}

gboolean
connection_is_secured(void)
{
    return 1;
}

TLSCertificate*
connection_get_tls_peer_cert(void)
{
    return NULL;
}

char*
connection_create_uuid(void)
{
    return NULL;
}

void
connection_free_uuid(char* uuid)
{
}

jabber_conn_status_t
connection_get_status(void)
{
    return mock_type(jabber_conn_status_t);
}

const char*
connection_get_presence_msg(void)
{
    return mock_ptr_type(const char*);
}

const char*
session_get_account_name(void)
{
    return mock_ptr_type(char*);
}

GList*
session_get_available_resources(void)
{
    return NULL;
}

void
connection_set_presence_msg(const char* const message)
{
}

gboolean
connection_send_stanza(const char* const stanza)
{
    return TRUE;
}

gboolean
connection_supports(const char* const feature)
{
    return FALSE;
}

const char*
connection_get_profanity_identifier(void)
{
    return "profident";
}

jabber_conn_status_t
connection_register(const char* const altdomain, int port, const char* const tls_policy,
                    const char* const username, const char* const password)
{
    check_expected(altdomain);
    check_expected(port);
    check_expected(tls_policy);
    check_expected(username);
    check_expected(password);
    return mock_type(jabber_conn_status_t);
}

// message functions
char*
message_send_chat(const char* const barejid, const char* const msg, const char* const oob_url, gboolean request_receipt, const char* const replace_id)
{
    check_expected(barejid);
    check_expected(msg);
    return NULL;
}

char*
message_send_chat_otr(const char* const barejid, const char* const msg, gboolean request_receipt, const char* const replace_id)
{
    check_expected(barejid);
    check_expected(msg);
    return NULL;
}

char*
message_send_chat_pgp(const char* const barejid, const char* const msg, gboolean request_receipt, const char* const replace_id)
{
    return NULL;
}

char*
message_send_chat_ox(const char* const barejid, const char* const msg, gboolean request_receipt, const char* const replace_id)
{
    return NULL;
}

char*
message_send_private(const char* const fulljid, const char* const msg, const char* const oob_url)
{
    return NULL;
}

char*
message_send_groupchat(const char* const roomjid, const char* const msg, const char* const oob_url, const char* const replace_id)
{
    return NULL;
}

void
message_send_groupchat_subject(const char* const roomjid, const char* const subject)
{
}

void
message_send_inactive(const char* const barejid)
{
}
void
message_send_composing(const char* const barejid)
{
}
void
message_send_paused(const char* const barejid)
{
}
void
message_send_gone(const char* const barejid)
{
}

void
message_send_invite(const char* const room, const char* const contact,
                    const char* const reason)
{
}

void
message_request_voice(const char* const roomjid)
{
}

bool
message_is_sent_by_us(const ProfMessage* const message, bool checkOID)
{
    return TRUE;
}

// presence functions
void
presence_subscription(const char* const jid, const jabber_subscr_t action)
{
}

GList*
presence_get_subscription_requests(void)
{
    return NULL;
}

gint
presence_sub_request_count(void)
{
    return 0;
}

void
presence_reset_sub_request_search(void)
{
}

char*
presence_sub_request_find(const char* const search_str, gboolean previous, void* context)
{
    return NULL;
}

void
presence_join_room(const char* const room, const char* const nick, const char* const passwd)
{
    check_expected(room);
    check_expected(nick);
    check_expected(passwd);
}

void
presence_change_room_nick(const char* const room, const char* const nick)
{
}
void
presence_leave_chat_room(const char* const room_jid)
{
}

void
presence_send(resource_presence_t status, int idle, char* signed_status)
{
    check_expected(status);
    check_expected(idle);
    check_expected(signed_status);
}

gboolean
presence_sub_request_exists(const char* const bare_jid)
{
    return FALSE;
}

// iq functions
void iq_disable_carbons(){};
void iq_enable_carbons(){};
void
iq_send_software_version(const char* const fulljid)
{
}

void
iq_room_list_request(const char* conferencejid, char* filter)
{
    check_expected(conferencejid);
    check_expected(filter);
}

void
iq_disco_info_request(const char* jid)
{
}
void
iq_disco_items_request(const char* jid)
{
}
void
iq_set_autoping(int seconds)
{
}
void
iq_http_upload_request(HTTPUpload* upload)
{
}
void
iq_confirm_instant_room(const char* const room_jid)
{
}
void
iq_destroy_room(const char* const room_jid)
{
}
void
iq_request_room_config_form(const char* const room_jid)
{
}
void
iq_submit_room_config(ProfConfWin* confwin)
{
}
void
iq_room_config_cancel(ProfConfWin* confwin)
{
}
void
iq_send_ping(const char* const target)
{
}
void
iq_send_caps_request(const char* const to, const char* const id,
                     const char* const node, const char* const ver)
{
}
void
iq_send_caps_request_for_jid(const char* const to, const char* const id,
                             const char* const node, const char* const ver)
{
}
void
iq_send_caps_request_legacy(const char* const to, const char* const id,
                            const char* const node, const char* const ver)
{
}
void
iq_room_info_request(const char* const room, gboolean display)
{
}
void
iq_room_affiliation_list(const char* const room, char* affiliation, bool show)
{
}
void
iq_room_affiliation_set(const char* const room, const char* const jid, char* affiliation,
                        const char* const reason)
{
}
void
iq_room_kick_occupant(const char* const room, const char* const nick, const char* const reason)
{
}
void
iq_room_role_set(const char* const room, const char* const nick, char* role,
                 const char* const reason)
{
}
void
iq_room_role_list(const char* const room, char* role)
{
}
void
iq_last_activity_request(const char* jid)
{
}
void
iq_autoping_timer_cancel(void)
{
}
void
iq_autoping_check(void)
{
}
void
iq_rooms_cache_clear(void)
{
}
void
iq_command_list(const char* const target)
{
}
void
iq_command_exec(const char* const target, const char* const command)
{
}
void
iq_register_change_password(const char* const user, const char* const password)
{
}

void
iq_muc_register_nick(const char* const roomjid)
{
}

void
iq_mam_request(ProfChatWin* win, GDateTime* enddate)
{
}

void
iq_feature_retrieval_complete_handler(void)
{
}

void
publish_user_mood(const char* const mood, const char* const text)
{
}

// caps functions
void
caps_add_feature(char* feature)
{
}

void
caps_remove_feature(char* feature)
{
}

EntityCapabilities*
caps_lookup(const char* const jid)
{
    return NULL;
}

void
caps_destroy(EntityCapabilities* caps)
{
}

void
caps_reset_ver(void)
{
}

gboolean
caps_jid_has_feature(const char* const jid, const char* const feature)
{
    return FALSE;
}

gboolean
bookmark_add(const char* jid, const char* nick, const char* password, const char* autojoin_str, const char* name)
{
    check_expected(jid);
    check_expected(nick);
    check_expected(password);
    check_expected(autojoin_str);
    return mock_type(gboolean);
}

gboolean
bookmark_update(const char* jid, const char* nick, const char* password, const char* autojoin_str, const char* name)
{
    return FALSE;
}

gboolean
bookmark_remove(const char* jid)
{
    check_expected(jid);
    return mock_type(gboolean);
}

gboolean
bookmark_join(const char* jid)
{
    return FALSE;
}

GList*
bookmark_get_list(void)
{
    return mock_ptr_type(GList*);
}

Bookmark*
bookmark_get_by_jid(const char* jid)
{
    return NULL;
}

char*
bookmark_find(const char* const search_str, gboolean previous, void* context)
{
    return NULL;
}

void
bookmark_autocomplete_reset(void)
{
}

void
roster_send_name_change(const char* const barejid, const char* const new_name, GSList* groups)
{
    check_expected(barejid);
    check_expected(new_name);
    check_expected(groups);
}

gboolean
bookmark_exists(const char* const room)
{
    return FALSE;
}

void
roster_send_add_to_group(const char* const group, PContact contact)
{
}
void
roster_send_remove_from_group(const char* const group, PContact contact)
{
}

void
roster_send_add_new(const char* const barejid, const char* const name)
{
    check_expected(barejid);
    check_expected(name);
}

void
roster_send_remove(const char* const barejid)
{
    check_expected(barejid);
}

GList*
blocked_list(void)
{
    return NULL;
}

gboolean
blocked_add(char* jid, blocked_report reportkind, const char* const message)
{
    return TRUE;
}

gboolean
blocked_remove(char* jid)
{
    return TRUE;
}

char*
blocked_ac_find(const char* const search_str, gboolean previous, void* context)
{
    return NULL;
}

void
blocked_ac_reset(void)
{
}
