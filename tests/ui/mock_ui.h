#ifndef MOCK_UI_H
#define MICK_UI_H

#include <glib.h>
#include <setjmp.h>
#include <cmocka.h>

#include "config/account.h"
#include "contact.h"
#include "ui/window.h"

void stub_cons_show(void);

void mock_cons_show(void);
void expect_cons_show(char *output);
void expect_cons_show_calls(int n);

void mock_cons_show_contact_online(void);
void expect_cons_show_contact_online(PContact contact, Resource *resource, GDateTime *last_activity);
void stub_ui_chat_win_contact_online(void);

void mock_cons_show_error(void);
void expect_cons_show_error(char *output);

void stub_ui_handle_recipient_not_found(void);
void stub_ui_handle_recipient_error(void);
void expect_ui_handle_error(char *err_msg);
void expect_ui_handle_recipient_error(char *recipient, char *err_msg);
void expect_ui_handle_recipient_not_found(char *recipient, char *err_msg);

void mock_cons_show_account(void);
void expect_cons_show_account(ProfAccount *account);

void mock_cons_show_bookmarks(void);
void expect_cons_show_bookmarks(GList *bookmarks);

void mock_cons_show_aliases(void);
void expect_cons_show_aliases(void);

void mock_cons_show_account_list(void);
void expect_cons_show_account_list(gchar **accounts);

void stub_ui_ask_password(void);
void mock_ui_ask_password(void);
void mock_ui_ask_password_returns(char *password);

void mock_current_win_type(win_type_t type);

void mock_ui_current_recipient(void);
void ui_current_recipient_returns(char *jid);

void stub_ui_current_update_virtual(void);

void mock_ui_current_print_formatted_line(void);
void ui_current_print_formatted_line_expect(char show_char, int attrs, char *message);

void mock_ui_current_print_line(void);
void ui_current_print_line_expect(char *message);

void ui_current_win_is_otr_returns(gboolean result);

void ui_room_join_expect(char *room);

void mock_cons_show_roster(void);
void cons_show_roster_expect(GSList *list);

void ui_switch_win_expect_and_return(int given_i, gboolean result);

#endif
