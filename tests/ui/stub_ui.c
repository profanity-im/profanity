#include <glib.h>
#include <wchar.h>

#include <setjmp.h>
#include <cmocka.h>

#include "ui/window.h"
#include "ui/ui.h"

#include "tests/ui/stub_ui.h"

// mock state

static char output[256];

void
expect_cons_show(char *expected)
{
    expect_string(cons_show, output, expected);
}

void
expect_any_cons_show(void)
{
    expect_any(cons_show, output);
}

void
expect_cons_show_error(char *expected)
{
    expect_string(cons_show_error, output, expected);
}

void
expect_any_cons_show_error(void)
{
    expect_any(cons_show_error, output);
}

void
expect_ui_current_print_line(char *message)
{
    expect_string(ui_current_print_line, output, message);
}

void
expect_ui_current_print_formatted_line(char show_char, int attrs, char *message)
{
    expect_value(ui_current_print_formatted_line, show_char, show_char);
    expect_value(ui_current_print_formatted_line, attrs, attrs);
    expect_string(ui_current_print_formatted_line, output, message);
}

// stubs

void ui_init(void) {}
void ui_load_colours(void) {}
void ui_update(void) {}
void ui_close(void) {}
void ui_redraw(void) {}
void ui_resize(void) {}
GSList* ui_get_chat_recipients(void)
{
    return NULL;
}

void ui_handle_special_keys(const wint_t * const ch, const int result) {}

gboolean ui_switch_win(const int i)
{
    check_expected(i);
    return (gboolean)mock();
    return FALSE;
}

void ui_next_win(void) {}
void ui_previous_win(void) {}

void ui_gone_secure(const char * const barejid, gboolean trusted) {}
void ui_gone_insecure(const char * const barejid) {}
void ui_trust(const char * const barejid) {}
void ui_untrust(const char * const barejid) {}
void ui_smp_recipient_initiated(const char * const barejid) {}
void ui_smp_recipient_initiated_q(const char * const barejid, const char *question) {}

void ui_smp_successful(const char * const barejid) {}
void ui_smp_unsuccessful_sender(const char * const barejid) {}
void ui_smp_unsuccessful_receiver(const char * const barejid) {}
void ui_smp_aborted(const char * const barejid) {}

void ui_smp_answer_success(const char * const barejid) {}
void ui_smp_answer_failure(const char * const barejid) {}

void ui_otr_authenticating(const char * const barejid) {}
void ui_otr_authetication_waiting(const char * const recipient) {}

unsigned long ui_get_idle_time(void)
{
    return 0;
}

void ui_reset_idle_time(void) {}
void ui_new_chat_win(const char * const barejid) {}
void ui_new_private_win(const char * const fulljid) {}
void ui_print_system_msg_from_recipient(const char * const barejid, const char *message) {}
gint ui_unread(void)
{
    return 0;
}

void ui_close_connected_win(int index) {}
int ui_close_all_wins(void)
{
    return 0;
}

int ui_close_read_wins(void)
{
    return 0;
}

// current window actions
void ui_clear_current(void) {}

win_type_t ui_current_win_type(void)
{
    return (win_type_t)mock();
}

int ui_current_win_index(void)
{
    return 0;
}

gboolean ui_current_win_is_otr(void)
{
    return (gboolean)mock();
}

ProfChatWin *ui_get_current_chat(void)
{
    return (ProfChatWin*)mock();
}

void ui_current_print_line(const char * const msg, ...)
{
    va_list args;
    va_start(args, msg);
    vsnprintf(output, sizeof(output), msg, args);
    check_expected(output);
    va_end(args);
}

void ui_current_print_formatted_line(const char show_char, int attrs, const char * const msg, ...)
{
    check_expected(show_char);
    check_expected(attrs);
    va_list args;
    va_start(args, msg);
    vsnprintf(output, sizeof(output), msg, args);
    check_expected(output);
    va_end(args);
}

void ui_current_error_line(const char * const msg) {}

win_type_t ui_win_type(int index)
{
    return WIN_CONSOLE;
}

void ui_close_win(int index) {}
gboolean ui_win_exists(int index)
{
    return FALSE;
}

int ui_win_unread(int index)
{
    return 0;
}

char * ui_ask_password(void)
{
    return mock_ptr_type(char *);
}

void ui_handle_stanza(const char * const msg) {}

// ui events
void ui_contact_typing(const char * const barejid, const char * const resource) {}
void ui_incoming_msg(const char * const from, const char * const resource, const char * const message, GTimeVal *tv_stamp) {}
void ui_incoming_private_msg(const char * const fulljid, const char * const message, GTimeVal *tv_stamp) {}

void ui_disconnected(void) {}
void ui_recipient_gone(const char * const barejid, const char * const resource) {}

void ui_outgoing_chat_msg(const char * const from, const char * const barejid,
    const char * const message) {}
void ui_outgoing_private_msg(const char * const from, const char * const fulljid,
    const char * const message) {}

void ui_room_join(const char * const roomjid, gboolean focus) {}
void ui_switch_to_room(const char * const roomjid) {}

void ui_room_role_change(const char * const roomjid, const char * const role, const char * const actor,
    const char * const reason) {}
void ui_room_affiliation_change(const char * const roomjid, const char * const affiliation, const char * const actor,
    const char * const reason) {}
void ui_room_role_and_affiliation_change(const char * const roomjid, const char * const role,
    const char * const affiliation, const char * const actor, const char * const reason) {}
void ui_room_occupant_role_change(const char * const roomjid, const char * const nick, const char * const role,
    const char * const actor, const char * const reason) {}
void ui_room_occupant_affiliation_change(const char * const roomjid, const char * const nick, const char * const affiliation,
    const char * const actor, const char * const reason) {}
void ui_room_occupant_role_and_affiliation_change(const char * const roomjid, const char * const nick, const char * const role,
    const char * const affiliation, const char * const actor, const char * const reason) {}
void ui_room_roster(const char * const roomjid, GList *occupants, const char * const presence) {}
void ui_room_history(const char * const roomjid, const char * const nick,
    GTimeVal tv_stamp, const char * const message) {}
void ui_room_message(const char * const roomjid, const char * const nick,
    const char * const message) {}
void ui_room_subject(const char * const roomjid, const char * const nick, const char * const subject) {}
void ui_room_requires_config(const char * const roomjid) {}
void ui_room_destroy(const char * const roomjid) {}
void ui_show_room_info(ProfMucWin *mucwin) {}
void ui_show_room_role_list(ProfMucWin *mucwin, muc_role_t role) {}
void ui_show_room_affiliation_list(ProfMucWin *mucwin, muc_affiliation_t affiliation) {}
void ui_handle_room_info_error(const char * const roomjid, const char * const error) {}
void ui_show_room_disco_info(const char * const roomjid, GSList *identities, GSList *features) {}
void ui_room_destroyed(const char * const roomjid, const char * const reason, const char * const new_jid,
    const char * const password) {}
void ui_room_kicked(const char * const roomjid, const char * const actor, const char * const reason) {}
void ui_room_member_kicked(const char * const roomjid, const char * const nick, const char * const actor,
    const char * const reason) {}
void ui_room_banned(const char * const roomjid, const char * const actor, const char * const reason) {}
void ui_room_member_banned(const char * const roomjid, const char * const nick, const char * const actor,
    const char * const reason) {}
void ui_leave_room(const char * const roomjid) {}
void ui_room_broadcast(const char * const roomjid,
    const char * const message) {}
void ui_room_member_offline(const char * const roomjid, const char * const nick) {}
void ui_room_member_online(const char * const roomjid, const char * const nick, const char * const roles,
    const char * const affiliation, const char * const show, const char * const status) {}
void ui_room_member_nick_change(const char * const roomjid,
    const char * const old_nick, const char * const nick) {}
void ui_room_nick_change(const char * const roomjid, const char * const nick) {}
void ui_room_member_presence(const char * const roomjid,
    const char * const nick, const char * const show, const char * const status) {}
void ui_room_show_occupants(const char * const roomjid) {}
void ui_room_hide_occupants(const char * const roomjid) {}
void ui_show_roster(void) {}
void ui_hide_roster(void) {}
void ui_roster_add(const char * const barejid, const char * const name) {}
void ui_roster_remove(const char * const barejid) {}
void ui_contact_already_in_group(const char * const contact, const char * const group) {}
void ui_contact_not_in_group(const char * const contact, const char * const group) {}
void ui_group_added(const char * const contact, const char * const group) {}
void ui_group_removed(const char * const contact, const char * const group) {}
void ui_chat_win_contact_online(PContact contact, Resource *resource, GDateTime *last_activity) {}
void ui_chat_win_contact_offline(PContact contact, char *resource, char *status) {}
gboolean ui_chat_win_exists(const char * const barejid)
{
    return TRUE;
}

void ui_contact_offline(char *barejid, char *resource, char *status) {}

void ui_handle_recipient_not_found(const char * const recipient, const char * const err_msg)
{
    check_expected(recipient);
    check_expected(err_msg);
}

void ui_handle_recipient_error(const char * const recipient, const char * const err_msg)
{
    check_expected(recipient);
    check_expected(err_msg);
}

void ui_handle_error(const char * const err_msg)
{
    check_expected(err_msg);
}

void ui_clear_win_title(void) {}
void ui_goodbye_title(void) {}
void ui_handle_room_join_error(const char * const roomjid, const char * const err) {}
void ui_handle_room_configuration(const char * const roomjid, DataForm *form) {}
void ui_handle_room_configuration_form_error(const char * const roomjid, const char * const message) {}
void ui_handle_room_config_submit_result(const char * const roomjid) {}
void ui_handle_room_config_submit_result_error(const char * const roomjid, const char * const message) {}
void ui_handle_room_affiliation_list_error(const char * const roomjid, const char * const affiliation,
    const char * const error) {}
void ui_handle_room_affiliation_list(const char * const roomjid, const char * const affiliation, GSList *jids) {}
void ui_handle_room_affiliation_set_error(const char * const roomjid, const char * const jid,
    const char * const affiliation, const char * const error) {}
void ui_handle_room_role_set_error(const char * const roomjid, const char * const nick, const char * const role,
    const char * const error) {}
void ui_handle_room_role_list_error(const char * const roomjid, const char * const role, const char * const error) {}
void ui_handle_room_role_list(const char * const roomjid, const char * const role, GSList *nicks) {}
void ui_handle_room_kick_error(const char * const roomjid, const char * const nick, const char * const error) {}
void ui_show_form(ProfMucConfWin *confwin) {}
void ui_show_form_field(ProfWin *window, DataForm *form, char *tag) {}
void ui_show_form_help(ProfMucConfWin *confwin) {}
void ui_show_form_field_help(ProfMucConfWin *confwin, char *tag) {}
void ui_show_lines(ProfWin *window, const gchar** lines) {}
void ui_redraw_all_room_rosters(void) {}
void ui_show_all_room_rosters(void) {}
void ui_hide_all_room_rosters(void) {}

void ui_tidy_wins(void) {}
void ui_prune_wins(void) {}
gboolean ui_swap_wins(int source_win, int target_win)
{
    return FALSE;
}

void ui_auto_away(void) {}
void ui_end_auto_away(void) {}
void ui_titlebar_presence(contact_presence_t presence) {}
void ui_handle_login_account_success(ProfAccount *account) {}
void ui_update_presence(const resource_presence_t resource_presence,
    const char * const message, const char * const show) {}
void ui_about(void) {}
void ui_statusbar_new(const int win) {}

wint_t ui_get_char(char *input, int *size, int *result)
{
    return 0;
}

void ui_input_clear(void) {}
void ui_input_nonblocking(gboolean reset) {}
void ui_replace_input(char *input, const char * const new_input, int *size) {}

void ui_invalid_command_usage(const char * const usage, void (*setting_func)(void)) {}

void ui_create_xmlconsole_win(void) {}
gboolean ui_xmlconsole_exists(void)
{
    return FALSE;
}

void ui_open_xmlconsole_win(void) {}

gboolean ui_win_has_unsaved_form(int num)
{
    return FALSE;
}

// console window actions

void cons_show(const char * const msg, ...)
{
    va_list args;
    va_start(args, msg);
    vsnprintf(output, sizeof(output), msg, args);
    check_expected(output);
    va_end(args);
}

void cons_about(void) {}
void cons_help(void) {}
void cons_navigation_help(void) {}
void cons_prefs(void) {}
void cons_show_ui_prefs(void) {}
void cons_show_desktop_prefs(void) {}
void cons_show_chat_prefs(void) {}
void cons_show_log_prefs(void) {}
void cons_show_presence_prefs(void) {}
void cons_show_connection_prefs(void) {}
void cons_show_otr_prefs(void) {}

void cons_show_account(ProfAccount *account)
{
    check_expected(account);
}

void cons_debug(const char * const msg, ...) {}
void cons_show_time(void) {}
void cons_show_word(const char * const word) {}

void cons_show_error(const char * const cmd, ...)
{
    va_list args;
    va_start(args, cmd);
    vsnprintf(output, sizeof(output), cmd, args);
    check_expected(output);
    va_end(args);
}

void cons_show_contacts(GSList * list) {}

void cons_show_roster(GSList * list)
{
    check_expected(list);
}

void cons_show_roster_group(const char * const group, GSList * list) {}
void cons_show_wins(void) {}
void cons_show_status(const char * const barejid) {}
void cons_show_info(PContact pcontact) {}
void cons_show_caps(const char * const fulljid, resource_presence_t presence) {}
void cons_show_themes(GSList *themes) {}

void cons_show_aliases(GList *aliases)
{
    check_expected(aliases);
}

void cons_show_login_success(ProfAccount *account) {}
void cons_show_software_version(const char * const jid,
    const char * const presence, const char * const name,
    const char * const version, const char * const os) {}

void cons_show_account_list(gchar **accounts)
{
    check_expected(accounts);
}

void cons_show_room_list(GSList *room, const char * const conference_node) {}

void cons_show_bookmarks(const GList *list)
{
    check_expected(list);
}

void cons_show_disco_items(GSList *items, const char * const jid) {}
void cons_show_disco_info(const char *from, GSList *identities, GSList *features) {}
void cons_show_room_invite(const char * const invitor, const char * const room,
    const char * const reason) {}
void cons_check_version(gboolean not_available_msg) {}
void cons_show_typing(const char * const barejid) {}
void cons_show_incoming_message(const char * const short_from, const int win_index) {}
void cons_show_room_invites(GSList *invites) {}
void cons_show_received_subs(void) {}
void cons_show_sent_subs(void) {}
void cons_alert(void) {}
void cons_theme_setting(void) {}
void cons_privileges_setting(void) {}
void cons_beep_setting(void) {}
void cons_flash_setting(void) {}
void cons_splash_setting(void) {}
void cons_vercheck_setting(void) {}
void cons_resource_setting(void) {}
void cons_occupants_setting(void) {}
void cons_roster_setting(void) {}
void cons_presence_setting(void) {}
void cons_wrap_setting(void) {}
void cons_time_setting(void) {}
void cons_mouse_setting(void) {}
void cons_statuses_setting(void) {}
void cons_titlebar_setting(void) {}
void cons_notify_setting(void) {}
void cons_states_setting(void) {}
void cons_outtype_setting(void) {}
void cons_intype_setting(void) {}
void cons_gone_setting(void) {}
void cons_history_setting(void) {}
void cons_log_setting(void) {}
void cons_chlog_setting(void) {}
void cons_grlog_setting(void) {}
void cons_autoaway_setting(void) {}
void cons_reconnect_setting(void) {}
void cons_autoping_setting(void) {}
void cons_priority_setting(void) {}
void cons_autoconnect_setting(void) {}
void cons_inpblock_setting(void) {}

void cons_show_contact_online(PContact contact, Resource *resource, GDateTime *last_activity)
{
    check_expected(contact);
    check_expected(resource);
    check_expected(last_activity);
}

void cons_show_contact_offline(PContact contact, char *resource, char *status) {}
void cons_theme_colours(void) {}

// roster window
void rosterwin_roster(void) {}

// occupants window
void occupantswin_occupants(const char * const room) {}

// desktop notifier actions
void notifier_uninit(void) {}

void notify_typing(const char * const handle) {}
void notify_message(const char * const handle, int win, const char * const text) {}
void notify_room_message(const char * const handle, const char * const room,
    int win, const char * const text) {}
void notify_remind(void) {}
void notify_invite(const char * const from, const char * const room,
    const char * const reason) {}
void notify_subscription(const char * const from) {}
