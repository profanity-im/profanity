/*
 * commands.c
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
 *
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <glib.h>

#include "chat_session.h"
#include "command/commands.h"
#include "command/command.h"
#include "common.h"
#include "config/accounts.h"
#include "config/account.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "contact.h"
#include "roster_list.h"
#include "jid.h"
#include "log.h"
#include "muc.h"
#include "otr/otr.h"
#include "profanity.h"
#include "tools/autocomplete.h"
#include "tools/parser.h"
#include "tools/tinyurl.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "ui/windows.h"
#include "xmpp/xmpp.h"
#include "xmpp/bookmark.h"

static void _update_presence(const resource_presence_t presence,
    const char * const show, gchar **args);
static gboolean _cmd_set_boolean_preference(gchar *arg, struct cmd_help_t help,
    const char * const display, preference_t pref);
static int _strtoi(char *str, int *saveptr, int min, int max);
static void _cmd_show_filtered_help(char *heading, gchar *cmd_filter[], int filter_size);
static gint _compare_commands(Command *a, Command *b);

extern GHashTable *commands;

gboolean
cmd_connect(gchar **args, struct cmd_help_t help)
{
    gboolean result = FALSE;

    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if ((conn_status != JABBER_DISCONNECTED) && (conn_status != JABBER_STARTED)) {
        cons_show("You are either connected already, or a login is in process.");
        result = TRUE;
    } else {
        char *user = args[0];
        char *opt1 = args[1];
        char *opt1val = args[2];
        char *opt2 = args[3];
        char *opt2val = args[4];
        char *lower = g_utf8_strdown(user, -1);
        char *jid;

        // parse options
        char *altdomain = NULL;
        int port = 0;
        gboolean server_set = FALSE;
        gboolean port_set = FALSE;
        if (opt1 != NULL) {
            if (opt1val == NULL) {
                cons_show("Usage: %s", help.usage);
                cons_show("");
                return TRUE;
            }
            if (strcmp(opt1, "server") == 0) {
                altdomain = opt1val;
                server_set = TRUE;
            } else if (strcmp(opt1, "port") == 0) {
                if (_strtoi(opt1val, &port, 1, 65535) != 0) {
                    port = 0;
                    cons_show("");
                    return TRUE;
                } else {
                    port_set = TRUE;
                }
            } else {
                cons_show("Usage: %s", help.usage);
                cons_show("");
                return TRUE;
            }

            if (opt2 != NULL) {
                if (server_set && strcmp("server", opt2) == 0) {
                    cons_show("Usage: %s", help.usage);
                    cons_show("");
                    return TRUE;
                }
                if (port_set && strcmp("port", opt2) == 0) {
                    cons_show("Usage: %s", help.usage);
                    cons_show("");
                    return TRUE;
                }
                if (opt2val == NULL) {
                    cons_show("Usage: %s", help.usage);
                    cons_show("");
                    return TRUE;
                }
                if (strcmp(opt2, "server") == 0) {
                    if (server_set) {
                        cons_show("Usage: %s", help.usage);
                        return TRUE;
                    }
                    altdomain = opt2val;
                    server_set = TRUE;
                } else if (strcmp(opt2, "port") == 0) {
                    if (port_set) {
                        cons_show("Usage: %s", help.usage);
                        return TRUE;
                    }
                    if (_strtoi(opt2val, &port, 1, 65535) != 0) {
                        port = 0;
                        cons_show("");
                        return TRUE;
                    } else {
                        port_set = TRUE;
                    }
                } else {
                    cons_show("Usage: %s", help.usage);
                    cons_show("");
                    return TRUE;
                }
            }
        }

        ProfAccount *account = accounts_get_account(lower);
        if (account != NULL) {
            jid = account_create_full_jid(account);
            if (account->password == NULL) {
                account->password = ui_ask_password();
            }
            cons_show("Connecting with account %s as %s", account->name, jid);
            conn_status = jabber_connect_with_account(account);
            account_free(account);
        } else {
            char *passwd = ui_ask_password();
            jid = strdup(lower);
            cons_show("Connecting as %s", jid);
            conn_status = jabber_connect_with_details(jid, passwd, altdomain, port);
            free(passwd);
        }
        g_free(lower);

        if (conn_status == JABBER_DISCONNECTED) {
            cons_show_error("Connection attempt for %s failed.", jid);
            log_info("Connection attempt for %s failed", jid);
        }

        free(jid);

        result = TRUE;
    }

    return result;
}

gboolean
cmd_account(gchar **args, struct cmd_help_t help)
{
    char *command = args[0];

    if (command == NULL) {
        if (jabber_get_connection_status() != JABBER_CONNECTED) {
            cons_show("Usage: %s", help.usage);
        } else {
            ProfAccount *account = accounts_get_account(jabber_get_account_name());
            cons_show_account(account);
            account_free(account);
        }
    } else if (strcmp(command, "list") == 0) {
        gchar **accounts = accounts_get_list();
        cons_show_account_list(accounts);
        g_strfreev(accounts);
    } else if (strcmp(command, "show") == 0) {
        char *account_name = args[1];
        if (account_name == NULL) {
            cons_show("Usage: %s", help.usage);
        } else {
            ProfAccount *account = accounts_get_account(account_name);
            if (account == NULL) {
                cons_show("No such account.");
                cons_show("");
            } else {
                cons_show_account(account);
                account_free(account);
            }
        }
    } else if (strcmp(command, "add") == 0) {
        char *account_name = args[1];
        if (account_name == NULL) {
            cons_show("Usage: %s", help.usage);
        } else {
            accounts_add(account_name, NULL, 0);
            cons_show("Account created.");
            cons_show("");
        }
    } else if (strcmp(command, "enable") == 0) {
        char *account_name = args[1];
        if (account_name == NULL) {
            cons_show("Usage: %s", help.usage);
        } else {
            if (accounts_enable(account_name)) {
                cons_show("Account enabled.");
                cons_show("");
            } else {
                cons_show("No such account: %s", account_name);
                cons_show("");
            }
        }
    } else if (strcmp(command, "disable") == 0) {
        char *account_name = args[1];
        if (account_name == NULL) {
            cons_show("Usage: %s", help.usage);
        } else {
            if (accounts_disable(account_name)) {
                cons_show("Account disabled.");
                cons_show("");
            } else {
                cons_show("No such account: %s", account_name);
                cons_show("");
            }
        }
    } else if (strcmp(command, "rename") == 0) {
        if (g_strv_length(args) != 3) {
            cons_show("Usage: %s", help.usage);
        } else {
            char *account_name = args[1];
            char *new_name = args[2];

            if (accounts_rename(account_name, new_name)) {
                cons_show("Account renamed.");
                cons_show("");
            } else {
                cons_show("Either account %s doesn't exist, or account %s already exists.", account_name, new_name);
                cons_show("");
            }
        }
    } else if (strcmp(command, "set") == 0) {
        if (g_strv_length(args) != 4) {
            cons_show("Usage: %s", help.usage);
        } else {
            char *account_name = args[1];
            char *property = args[2];
            char *value = args[3];

            if (!accounts_account_exists(account_name)) {
                cons_show("Account %s doesn't exist", account_name);
                cons_show("");
            } else {
                if (strcmp(property, "jid") == 0) {
                    Jid *jid = jid_create(args[3]);
                    if (jid == NULL) {
                        cons_show("Malformed jid: %s", value);
                    } else {
                        accounts_set_jid(account_name, jid->barejid);
                        cons_show("Updated jid for account %s: %s", account_name, jid->barejid);
                        if (jid->resourcepart != NULL) {
                            accounts_set_resource(account_name, jid->resourcepart);
                            cons_show("Updated resource for account %s: %s", account_name, jid->resourcepart);
                        }
                        cons_show("");
                    }
                    jid_destroy(jid);
                } else if (strcmp(property, "server") == 0) {
                    accounts_set_server(account_name, value);
                    cons_show("Updated server for account %s: %s", account_name, value);
                    cons_show("");
                } else if (strcmp(property, "port") == 0) {
                    int port;
                    if (_strtoi(value, &port, 1, 65535) != 0) {
                        cons_show("");
                        return TRUE;
                    } else {
                        accounts_set_port(account_name, port);
                        cons_show("Updated port for account %s: %s", account_name, value);
                        cons_show("");
                    }
                } else if (strcmp(property, "resource") == 0) {
                    accounts_set_resource(account_name, value);
                    cons_show("Updated resource for account %s: %s", account_name, value);
                    cons_show("");
                } else if (strcmp(property, "password") == 0) {
                    accounts_set_password(account_name, value);
                    cons_show("Updated password for account %s", account_name);
                    cons_show("");
                } else if (strcmp(property, "muc") == 0) {
                    accounts_set_muc_service(account_name, value);
                    cons_show("Updated muc service for account %s: %s", account_name, value);
                    cons_show("");
                } else if (strcmp(property, "nick") == 0) {
                    accounts_set_muc_nick(account_name, value);
                    cons_show("Updated muc nick for account %s: %s", account_name, value);
                    cons_show("");
                } else if (strcmp(property, "status") == 0) {
                    if (!valid_resource_presence_string(value) && (strcmp(value, "last") != 0)) {
                        cons_show("Invalid status: %s", value);
                    } else {
                        accounts_set_login_presence(account_name, value);
                        cons_show("Updated login status for account %s: %s", account_name, value);
                    }
                    cons_show("");
                } else if (valid_resource_presence_string(property)) {
                        int intval;

                        if (_strtoi(value, &intval, -128, 127) == 0) {
                            resource_presence_t presence_type = resource_presence_from_string(property);
                            switch (presence_type)
                            {
                                case (RESOURCE_ONLINE):
                                    accounts_set_priority_online(account_name, intval);
                                    break;
                                case (RESOURCE_CHAT):
                                    accounts_set_priority_chat(account_name, intval);
                                    break;
                                case (RESOURCE_AWAY):
                                    accounts_set_priority_away(account_name, intval);
                                    break;
                                case (RESOURCE_XA):
                                    accounts_set_priority_xa(account_name, intval);
                                    break;
                                case (RESOURCE_DND):
                                    accounts_set_priority_dnd(account_name, intval);
                                    break;
                            }

                            jabber_conn_status_t conn_status = jabber_get_connection_status();
                            if (conn_status == JABBER_CONNECTED) {
                                char *connected_account = jabber_get_account_name();
                                resource_presence_t last_presence = accounts_get_last_presence(connected_account);

                                if (presence_type == last_presence) {
                                    char *message = jabber_get_presence_message();
                                    presence_update(last_presence, message, 0);
                                }
                            }
                            cons_show("Updated %s priority for account %s: %s", property, account_name, value);
                            cons_show("");
                        }
                } else {
                    cons_show("Invalid property: %s", property);
                    cons_show("");
                }
            }
        }
    } else if (strcmp(command, "clear") == 0) {
        if (g_strv_length(args) != 3) {
            cons_show("Usage: %s", help.usage);
        } else {
            char *account_name = args[1];
            char *property = args[2];

            if (!accounts_account_exists(account_name)) {
                cons_show("Account %s doesn't exist", account_name);
                cons_show("");
            } else {
                if (strcmp(property, "password") == 0) {
                    accounts_clear_password(account_name);
                    cons_show("Removed password for account %s", account_name);
                    cons_show("");
                } else {
                    cons_show("Invalid property: %s", property);
                    cons_show("");
                }
            }
        }
    } else {
        cons_show("");
    }

    return TRUE;
}

gboolean
cmd_sub(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are currently not connected.");
        return TRUE;
    }

    char *subcmd, *jid;
    subcmd = args[0];
    jid = args[1];

    if (subcmd == NULL) {
        cons_show("Usage: %s", help.usage);
        return TRUE;
    }

    if (strcmp(subcmd, "sent") == 0) {
        cons_show_sent_subs();
        return TRUE;
    }

    if (strcmp(subcmd, "received") == 0) {
        cons_show_received_subs();
        return TRUE;
    }

    win_type_t win_type = ui_current_win_type();
    if ((win_type != WIN_CHAT) && (jid == NULL)) {
        cons_show("You must specify a contact.");
        return TRUE;
    }

    if (jid == NULL) {
        jid = ui_current_recipient();
    }

    Jid *jidp = jid_create(jid);

    if (strcmp(subcmd, "allow") == 0) {
        presence_subscription(jidp->barejid, PRESENCE_SUBSCRIBED);
        cons_show("Accepted subscription for %s", jidp->barejid);
        log_info("Accepted subscription for %s", jidp->barejid);
    } else if (strcmp(subcmd, "deny") == 0) {
        presence_subscription(jidp->barejid, PRESENCE_UNSUBSCRIBED);
        cons_show("Deleted/denied subscription for %s", jidp->barejid);
        log_info("Deleted/denied subscription for %s", jidp->barejid);
    } else if (strcmp(subcmd, "request") == 0) {
        presence_subscription(jidp->barejid, PRESENCE_SUBSCRIBE);
        cons_show("Sent subscription request to %s.", jidp->barejid);
        log_info("Sent subscription request to %s.", jidp->barejid);
    } else if (strcmp(subcmd, "show") == 0) {
        PContact contact = roster_get_contact(jidp->barejid);
        if ((contact == NULL) || (p_contact_subscription(contact) == NULL)) {
            if (win_type == WIN_CHAT) {
                ui_current_print_line("No subscription information for %s.", jidp->barejid);
            } else {
                cons_show("No subscription information for %s.", jidp->barejid);
            }
        } else {
            if (win_type == WIN_CHAT) {
                if (p_contact_pending_out(contact)) {
                    ui_current_print_line("%s subscription status: %s, request pending.",
                        jidp->barejid, p_contact_subscription(contact));
                } else {
                    ui_current_print_line("%s subscription status: %s.", jidp->barejid,
                        p_contact_subscription(contact));
                }
            } else {
                if (p_contact_pending_out(contact)) {
                    cons_show("%s subscription status: %s, request pending.",
                        jidp->barejid, p_contact_subscription(contact));
                } else {
                    cons_show("%s subscription status: %s.", jidp->barejid,
                        p_contact_subscription(contact));
                }
            }
        }
    } else {
        cons_show("Usage: %s", help.usage);
    }

    jid_destroy(jidp);

    return TRUE;
}

gboolean
cmd_disconnect(gchar **args, struct cmd_help_t help)
{
    if (jabber_get_connection_status() == JABBER_CONNECTED) {
        char *jid = strdup(jabber_get_fulljid());
        cons_show("%s logged out successfully.", jid);
        jabber_disconnect();
        roster_clear();
        muc_clear_invites();
        chat_sessions_clear();
        ui_disconnected();
        ui_current_page_off();
        free(jid);
    } else {
        cons_show("You are not currently connected.");
    }

    return TRUE;
}

gboolean
cmd_quit(gchar **args, struct cmd_help_t help)
{
    log_info("Profanity is shutting down...");
    exit(0);
    return FALSE;
}

gboolean
cmd_wins(gchar **args, struct cmd_help_t help)
{
    if (args[0] == NULL) {
        cons_show_wins();
    } else if (strcmp(args[0], "tidy") == 0) {
        ui_tidy_wins();
    } else if (strcmp(args[0], "prune") == 0) {
        ui_prune_wins();
    }
    return TRUE;
}

gboolean
cmd_win(gchar **args, struct cmd_help_t help)
{
    int num = atoi(args[0]);
    gboolean switched = ui_switch_win(num);
    if (switched == FALSE) {
        cons_show("Window %d does not exist.", num);
    }
    return TRUE;
}

gboolean
cmd_help(gchar **args, struct cmd_help_t help)
{
    int num_args = g_strv_length(args);
    if (num_args == 0) {
        cons_help();
    } else if (strcmp(args[0], "commands") == 0) {
        cons_show("");
        cons_show("All commands");
        cons_show("");

        GList *ordered_commands = NULL;
        GHashTableIter iter;
        gpointer key;
        gpointer value;

        g_hash_table_iter_init(&iter, commands);
        while (g_hash_table_iter_next(&iter, &key, &value)) {
            ordered_commands = g_list_insert_sorted(ordered_commands, value, (GCompareFunc)_compare_commands);
        }

        GList *curr = ordered_commands;
        while (curr != NULL) {
            Command *cmd = curr->data;
            cons_show("%-12s: %s", cmd->cmd, cmd->help.short_help);
            curr = g_list_next(curr);
        }
        g_list_free(ordered_commands);
        g_list_free(curr);

        cons_show("");
        cons_show("Use /help [command] without the leading slash, for help on a specific command");
        cons_show("");

    } else if (strcmp(args[0], "basic") == 0) {
        gchar *filter[] = { "/about", "/clear", "/close", "/connect",
            "/disconnect", "/help", "/msg", "/join", "/quit", "/vercheck",
            "/wins" };
        _cmd_show_filtered_help("Basic commands", filter, ARRAY_SIZE(filter));

    } else if (strcmp(args[0], "chatting") == 0) {
        gchar *filter[] = { "/chlog", "/otr", "/duck", "/gone", "/history",
            "/info", "/intype", "/msg", "/notify", "/outtype", "/status",
            "/close", "/clear", "/tiny" };
        _cmd_show_filtered_help("Chat commands", filter, ARRAY_SIZE(filter));

    } else if (strcmp(args[0], "groupchat") == 0) {
        gchar *filter[] = { "/close", "/clear", "/decline", "/grlog",
            "/invite", "/invites", "/join", "/leave", "/notify", "/msg",
            "/rooms", "/tiny", "/who", "/nick" };
        _cmd_show_filtered_help("Groupchat commands", filter, ARRAY_SIZE(filter));

    } else if (strcmp(args[0], "presence") == 0) {
        gchar *filter[] = { "/autoaway", "/away", "/chat", "/dnd",
            "/online", "/priority", "/account", "/status", "/statuses", "/who",
            "/xa" };
        _cmd_show_filtered_help("Presence commands", filter, ARRAY_SIZE(filter));

    } else if (strcmp(args[0], "contacts") == 0) {
        gchar *filter[] = { "/group", "/roster", "/sub", "/who" };
        _cmd_show_filtered_help("Roster commands", filter, ARRAY_SIZE(filter));

    } else if (strcmp(args[0], "service") == 0) {
        gchar *filter[] = { "/caps", "/disco", "/info", "/software", "/rooms" };
        _cmd_show_filtered_help("Service discovery commands", filter, ARRAY_SIZE(filter));

    } else if (strcmp(args[0], "settings") == 0) {
        gchar *filter[] = { "/account", "/autoaway", "/autoping", "/autoconnect", "/beep",
            "/chlog", "/flash", "/gone", "/grlog", "/history", "/intype",
            "/log", "/mouse", "/notify", "/outtype", "/prefs", "/priority",
            "/reconnect", "/roster", "/splash", "/states", "/statuses", "/theme",
            "/titlebar", "/vercheck" };
        _cmd_show_filtered_help("Settings commands", filter, ARRAY_SIZE(filter));

    } else if (strcmp(args[0], "other") == 0) {
        gchar *filter[] = { "/duck", "/vercheck" };
        _cmd_show_filtered_help("Other commands", filter, ARRAY_SIZE(filter));

    } else if (strcmp(args[0], "navigation") == 0) {
        cons_navigation_help();
    } else {
        char *cmd = args[0];
        char cmd_with_slash[1 + strlen(cmd) + 1];
        sprintf(cmd_with_slash, "/%s", cmd);

        const gchar **help_text = NULL;
        Command *command = g_hash_table_lookup(commands, cmd_with_slash);

        if (command != NULL) {
            help_text = command->help.long_help;
        }

        cons_show("");

        if (help_text != NULL) {
            int i;
            for (i = 0; help_text[i] != NULL; i++) {
                cons_show(help_text[i]);
            }
        } else {
            cons_show("No such command.");
        }

        cons_show("");
    }

    return TRUE;
}

gboolean
cmd_about(gchar **args, struct cmd_help_t help)
{
    cons_show("");
    cons_about();
    if (ui_current_win_type() != WIN_CONSOLE) {
        status_bar_new(1);
    }
    return TRUE;
}

gboolean
cmd_prefs(gchar **args, struct cmd_help_t help)
{
    if (args[0] == NULL) {
        cons_prefs();
        cons_show("Use the /account command for preferences for individual accounts.");
    } else if (strcmp(args[0], "ui") == 0) {
        cons_show("");
        cons_show_ui_prefs();
        cons_show("");
    } else if (strcmp(args[0], "desktop") == 0) {
        cons_show("");
        cons_show_desktop_prefs();
        cons_show("");
    } else if (strcmp(args[0], "chat") == 0) {
        cons_show("");
        cons_show_chat_prefs();
        cons_show("");
    } else if (strcmp(args[0], "log") == 0) {
        cons_show("");
        cons_show_log_prefs();
        cons_show("");
    } else if (strcmp(args[0], "conn") == 0) {
        cons_show("");
        cons_show_connection_prefs();
        cons_show("");
    } else if (strcmp(args[0], "presence") == 0) {
        cons_show("");
        cons_show_presence_prefs();
        cons_show("");
    } else {
        cons_show("Usage: %s", help.usage);
    }

    return TRUE;
}

gboolean
cmd_theme(gchar **args, struct cmd_help_t help)
{
    // list themes
    if (strcmp(args[0], "list") == 0) {
        GSList *themes = theme_list();
        cons_show_themes(themes);
        g_slist_free_full(themes, g_free);

    // load a theme
    } else if (strcmp(args[0], "set") == 0) {
        if (args[1] == NULL) {
            cons_show("Usage: %s", help.usage);
        } else if (theme_load(args[1])) {
            ui_load_colours();
            prefs_set_string(PREF_THEME, args[1]);
            cons_show("Loaded theme: %s", args[1]);
        } else {
            cons_show("Couldn't find theme: %s", args[1]);
        }
    } else {
        cons_show("Usage: %s", help.usage);
    }

    return TRUE;
}

gboolean
cmd_who(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();
    win_type_t win_type = ui_current_win_type();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
    } else {
        char *presence = args[0];
        char *group = NULL;
        if ((g_strv_length(args) == 2) && (args[1] != NULL)) {
            group = args[1];
        }

        // bad arg
        if ((presence != NULL)
                && (strcmp(presence, "online") != 0)
                && (strcmp(presence, "available") != 0)
                && (strcmp(presence, "unavailable") != 0)
                && (strcmp(presence, "offline") != 0)
                && (strcmp(presence, "away") != 0)
                && (strcmp(presence, "chat") != 0)
                && (strcmp(presence, "xa") != 0)
                && (strcmp(presence, "dnd") != 0)
                && (strcmp(presence, "any") != 0)) {
            cons_show("Usage: %s", help.usage);

        // valid arg
        } else {
            if (win_type == WIN_MUC) {
                if (group != NULL) {
                    cons_show("The group argument is not valid when in a chat room.");
                    return TRUE;
                }

                char *room = ui_current_recipient();
                GList *list = muc_get_roster(room);

                // no arg, show all contacts
                if ((presence == NULL) || (g_strcmp0(presence, "any") == 0)) {
                    ui_room_roster(room, list, NULL);

                // available
                } else if (strcmp("available", presence) == 0) {
                    GList *filtered = NULL;

                    while (list != NULL) {
                        PContact contact = list->data;
                        if (p_contact_is_available(contact)) {
                            filtered = g_list_append(filtered, contact);
                        }
                        list = g_list_next(list);
                    }

                    ui_room_roster(room, filtered, "available");

                // unavailable
                } else if (strcmp("unavailable", presence) == 0) {
                    GList *filtered = NULL;

                    while (list != NULL) {
                        PContact contact = list->data;
                        if (!p_contact_is_available(contact)) {
                            filtered = g_list_append(filtered, contact);
                        }
                        list = g_list_next(list);
                    }

                    ui_room_roster(room, filtered, "unavailable");

                // online, available resources
                } else if (strcmp("online", presence) == 0) {
                    GList *filtered = NULL;

                    while (list != NULL) {
                        PContact contact = list->data;
                        if (p_contact_has_available_resource(contact)) {
                            filtered = g_list_append(filtered, contact);
                        }
                        list = g_list_next(list);
                    }

                    ui_room_roster(room, filtered, "online");

                // offline, no available resources
                } else if (strcmp("offline", presence) == 0) {
                    GList *filtered = NULL;

                    while (list != NULL) {
                        PContact contact = list->data;
                        if (!p_contact_has_available_resource(contact)) {
                            filtered = g_list_append(filtered, contact);
                        }
                        list = g_list_next(list);
                    }

                    ui_room_roster(room, filtered, "offline");

                // show specific status
                } else {
                    GList *filtered = NULL;

                    while (list != NULL) {
                        PContact contact = list->data;
                        if (strcmp(p_contact_presence(contact), presence) == 0) {
                            filtered = g_list_append(filtered, contact);
                        }
                        list = g_list_next(list);
                    }

                    ui_room_roster(room, filtered, presence);
                }

            // not in groupchat window
            } else {
                cons_show("");
                GSList *list = NULL;
                if (group != NULL) {
                    list = roster_get_group(group);
                } else {
                    list = roster_get_contacts();
                }

                // no arg, show all contacts
                if ((presence == NULL) || (g_strcmp0(presence, "any") == 0)) {
                    if (group != NULL) {
                        cons_show("%s:", group);
                    } else {
                        cons_show("All contacts:");
                    }
                    cons_show_contacts(list);

                // available
                } else if (strcmp("available", presence) == 0) {
                    if (group != NULL) {
                        cons_show("%s (%s):", group, presence);
                    } else {
                        cons_show("Contacts (%s):", presence);
                    }
                    GSList *filtered = NULL;

                    while (list != NULL) {
                        PContact contact = list->data;
                        if (p_contact_is_available(contact)) {
                            filtered = g_slist_append(filtered, contact);
                        }
                        list = g_slist_next(list);
                    }

                    cons_show_contacts(filtered);

                // unavailable
                } else if (strcmp("unavailable", presence) == 0) {
                    if (group != NULL) {
                        cons_show("%s (%s):", group, presence);
                    } else {
                        cons_show("Contacts (%s):", presence);
                    }
                    GSList *filtered = NULL;

                    while (list != NULL) {
                        PContact contact = list->data;
                        if (!p_contact_is_available(contact)) {
                            filtered = g_slist_append(filtered, contact);
                        }
                        list = g_slist_next(list);
                    }

                    cons_show_contacts(filtered);

                // online, available resources
                } else if (strcmp("online", presence) == 0) {
                    if (group != NULL) {
                        cons_show("%s (%s):", group, presence);
                    } else {
                        cons_show("Contacts (%s):", presence);
                    }
                    GSList *filtered = NULL;

                    while (list != NULL) {
                        PContact contact = list->data;
                        if (p_contact_has_available_resource(contact)) {
                            filtered = g_slist_append(filtered, contact);
                        }
                        list = g_slist_next(list);
                    }

                    cons_show_contacts(filtered);

                // offline, no available resources
                } else if (strcmp("offline", presence) == 0) {
                    if (group != NULL) {
                        cons_show("%s (%s):", group, presence);
                    } else {
                        cons_show("Contacts (%s):", presence);
                    }
                    GSList *filtered = NULL;

                    while (list != NULL) {
                        PContact contact = list->data;
                        if (!p_contact_has_available_resource(contact)) {
                            filtered = g_slist_append(filtered, contact);
                        }
                        list = g_slist_next(list);
                    }

                    cons_show_contacts(filtered);

                // show specific status
                } else {
                    if (group != NULL) {
                        cons_show("%s (%s):", group, presence);
                    } else {
                        cons_show("Contacts (%s):", presence);
                    }
                    GSList *filtered = NULL;

                    while (list != NULL) {
                        PContact contact = list->data;
                        if (strcmp(p_contact_presence(contact), presence) == 0) {
                            filtered = g_slist_append(filtered, contact);
                        }
                        list = g_slist_next(list);
                    }

                    cons_show_contacts(filtered);
                }
            }
        }
    }

    if (win_type != WIN_CONSOLE && win_type != WIN_MUC) {
        status_bar_new(1);
    }

    return TRUE;
}

gboolean
cmd_msg(gchar **args, struct cmd_help_t help)
{
    char *usr = args[0];
    char *msg = args[1];

    jabber_conn_status_t conn_status = jabber_get_connection_status();
    win_type_t win_type = ui_current_win_type();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (win_type == WIN_MUC) {
        char *room_name = ui_current_recipient();
        if (muc_nick_in_roster(room_name, usr)) {
            GString *full_jid = g_string_new(room_name);
            g_string_append(full_jid, "/");
            g_string_append(full_jid, usr);

            if (msg != NULL) {
                message_send(msg, full_jid->str);
                ui_outgoing_msg("me", full_jid->str, msg);
            } else {
                ui_new_chat_win(full_jid->str);
            }

            g_string_free(full_jid, TRUE);

        } else {
            ui_current_print_line("No such participant \"%s\" in room.", usr);
        }

        return TRUE;

    } else {
        char *usr_jid = roster_barejid_from_name(usr);
        if (usr_jid == NULL) {
            usr_jid = usr;
        }
        if (msg != NULL) {
#ifdef HAVE_LIBOTR
            if (otr_is_secure(usr_jid)) {
                char *encrypted = otr_encrypt_message(usr_jid, msg);
                if (encrypted != NULL) {
                    message_send(encrypted, usr_jid);
                    otr_free_message(encrypted);
                    ui_outgoing_msg("me", usr_jid, msg);

                    if (((win_type == WIN_CHAT) || (win_type == WIN_CONSOLE)) && prefs_get_boolean(PREF_CHLOG)) {
                        const char *jid = jabber_get_fulljid();
                        Jid *jidp = jid_create(jid);
                        if (strcmp(prefs_get_string(PREF_OTR_LOG), "on") == 0) {
                            chat_log_chat(jidp->barejid, usr_jid, msg, PROF_OUT_LOG, NULL);
                        } else if (strcmp(prefs_get_string(PREF_OTR_LOG), "redact") == 0) {
                            chat_log_chat(jidp->barejid, usr_jid, "[redacted]", PROF_OUT_LOG, NULL);
                        }
                        jid_destroy(jidp);
                    }
                } else {
                    cons_show_error("Failed to encrypt and send message,");
                }
            } else {
                message_send(msg, usr_jid);
                ui_outgoing_msg("me", usr_jid, msg);

                if (((win_type == WIN_CHAT) || (win_type == WIN_CONSOLE)) && prefs_get_boolean(PREF_CHLOG)) {
                    const char *jid = jabber_get_fulljid();
                    Jid *jidp = jid_create(jid);
                    chat_log_chat(jidp->barejid, usr_jid, msg, PROF_OUT_LOG, NULL);
                    jid_destroy(jidp);
                }
            }
            return TRUE;
#else
            message_send(msg, usr_jid);
            ui_outgoing_msg("me", usr_jid, msg);

            if (((win_type == WIN_CHAT) || (win_type == WIN_CONSOLE)) && prefs_get_boolean(PREF_CHLOG)) {
                const char *jid = jabber_get_fulljid();
                Jid *jidp = jid_create(jid);
                chat_log_chat(jidp->barejid, usr_jid, msg, PROF_OUT_LOG, NULL);
                jid_destroy(jidp);
            }
            return TRUE;
#endif

        } else {
            const char * jid = NULL;

            if (roster_barejid_from_name(usr_jid) != NULL) {
                jid = roster_barejid_from_name(usr_jid);
            } else {
                jid = usr_jid;
            }

            if (prefs_get_boolean(PREF_STATES)) {
                if (!chat_session_exists(jid)) {
                    chat_session_start(jid, TRUE);
                }
            }

            ui_new_chat_win(usr_jid);
#ifdef HAVE_LIBOTR
            if (otr_is_secure(jid)) {
                ui_gone_secure(jid, otr_is_trusted(jid));
            }
#endif
            return TRUE;
        }
    }
}

gboolean
cmd_group(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    // list all groups
    if (args[0] == NULL) {
        GSList *groups = roster_get_groups();
        GSList *curr = groups;
        if (curr != NULL) {
            cons_show("Groups:");
            while (curr != NULL) {
                cons_show("  %s", curr->data);
                curr = g_slist_next(curr);
            }

            g_slist_free_full(groups, g_free);
        } else {
            cons_show("No groups.");
        }
        return TRUE;
    }

    // show contacts in group
    if (strcmp(args[0], "show") == 0) {
        char *group = args[1];
        if (group == NULL) {
            cons_show("Usage: %s", help.usage);
            return TRUE;
        }

        GSList *list = roster_get_group(group);
        cons_show_roster_group(group, list);
        return TRUE;
    }

    // add contact to group
    if (strcmp(args[0], "add") == 0) {
        char *group = args[1];
        char *contact = args[2];

        if ((group == NULL) || (contact == NULL)) {
            cons_show("Usage: %s", help.usage);
            return TRUE;
        }

        char *barejid = roster_barejid_from_name(contact);
        if (barejid == NULL) {
            barejid = contact;
        }

        PContact pcontact = roster_get_contact(barejid);
        if (pcontact == NULL) {
            cons_show("Contact not found in roster: %s", barejid);
            return TRUE;
        }

        if (p_contact_in_group(pcontact, group)) {
            const char *display_name = p_contact_name_or_jid(pcontact);
            ui_contact_already_in_group(display_name, group);
            ui_current_page_off();
        } else {
            roster_send_add_to_group(group, pcontact);
        }

        return TRUE;
    }

    // remove contact from group
    if (strcmp(args[0], "remove") == 0) {
        char *group = args[1];
        char *contact = args[2];

        if ((group == NULL) || (contact == NULL)) {
            cons_show("Usage: %s", help.usage);
            return TRUE;
        }

        char *barejid = roster_barejid_from_name(contact);
        if (barejid == NULL) {
            barejid = contact;
        }

        PContact pcontact = roster_get_contact(barejid);
        if (pcontact == NULL) {
            cons_show("Contact not found in roster: %s", barejid);
            return TRUE;
        }

        if (!p_contact_in_group(pcontact, group)) {
            const char *display_name = p_contact_name_or_jid(pcontact);
            ui_contact_not_in_group(display_name, group);
            ui_current_page_off();
        } else {
            roster_send_remove_from_group(group, pcontact);
        }

        return TRUE;
    }

    cons_show("Usage: %s", help.usage);
    return TRUE;
}

gboolean
cmd_roster(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    // show roster
    if (args[0] == NULL) {
        GSList *list = roster_get_contacts();
        cons_show_roster(list);
        return TRUE;

    // add contact
    } else if (strcmp(args[0], "add") == 0) {
        char *jid = args[1];
        if (jid == NULL) {
            cons_show("Usage: %s", help.usage);
        } else {
            char *name = args[2];
            roster_send_add_new(jid, name);
        }
        return TRUE;

    // remove contact
    } else if (strcmp(args[0], "remove") == 0) {
        char *jid = args[1];
        if (jid == NULL) {
            cons_show("Usage: %s", help.usage);
        } else {
            roster_send_remove(jid);
        }
        return TRUE;

    // change nickname
    } else if (strcmp(args[0], "nick") == 0) {
        char *jid = args[1];
        if (jid == NULL) {
            cons_show("Usage: %s", help.usage);
            return TRUE;
        }

        char *name = args[2];
        if (name == NULL) {
            cons_show("Usage: %s", help.usage);
            return TRUE;
        }

        // contact does not exist
        PContact contact = roster_get_contact(jid);
        if (contact == NULL) {
            cons_show("Contact not found in roster: %s", jid);
            return TRUE;
        }

        const char *barejid = p_contact_barejid(contact);
        roster_change_name(contact, name);
        GSList *groups = p_contact_groups(contact);
        roster_send_name_change(barejid, name, groups);

        cons_show("Nickname for %s set to: %s.", jid, name);

        return TRUE;

    // remove nickname
    } else if (strcmp(args[0], "clearnick") == 0) {
        char *jid = args[1];
        if (jid == NULL) {
            cons_show("Usage: %s", help.usage);
            return TRUE;
        }

        // contact does not exist
        PContact contact = roster_get_contact(jid);
        if (contact == NULL) {
            cons_show("Contact not found in roster: %s", jid);
            return TRUE;
        }

        const char *barejid = p_contact_barejid(contact);
        roster_change_name(contact, NULL);
        GSList *groups = p_contact_groups(contact);
        roster_send_name_change(barejid, NULL, groups);

        cons_show("Nickname for %s removed.", jid);

        return TRUE;
    } else {
        cons_show("Usage: %s", help.usage);
        return TRUE;
    }
}

gboolean
cmd_duck(gchar **args, struct cmd_help_t help)
{
    char *query = args[0];

    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    // if no duck win open, create it and send a help command
    if (!ui_duck_exists()) {
        ui_create_duck_win();

        if (query != NULL) {
            message_send_duck(query);
            ui_duck(query);
        }

    // window exists, send query
    } else {
        ui_open_duck_win();

        if (query != NULL) {
            message_send_duck(query);
            ui_duck(query);
        }
    }

    return TRUE;
}

gboolean
cmd_status(gchar **args, struct cmd_help_t help)
{
    char *usr = args[0];
    char *usr_jid = NULL;

    jabber_conn_status_t conn_status = jabber_get_connection_status();
    win_type_t win_type = ui_current_win_type();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    switch (win_type)
    {
        case WIN_MUC:
            if (usr != NULL) {
                ui_status_room(usr);
            } else {
                ui_current_print_line("You must specify a nickname.");
            }
            break;
        case WIN_CHAT:
            if (usr != NULL) {
                ui_current_print_line("No parameter required when in chat.");
            } else {
                ui_status();
            }
            break;
        case WIN_PRIVATE:
            if (usr != NULL) {
                ui_current_print_line("No parameter required when in chat.");
            } else {
                ui_status_private();
            }
            break;
        case WIN_CONSOLE:
            if (usr != NULL) {
                usr_jid = roster_barejid_from_name(usr);
                if (usr_jid == NULL) {
                    usr_jid = usr;
                }
                cons_show_status(usr_jid);
            } else {
                cons_show("Usage: %s", help.usage);
            }
            break;
        default:
            break;
    }

    return TRUE;
}

gboolean
cmd_info(gchar **args, struct cmd_help_t help)
{
    char *usr = args[0];
    char *usr_jid = NULL;

    jabber_conn_status_t conn_status = jabber_get_connection_status();
    win_type_t win_type = ui_current_win_type();
    PContact pcontact = NULL;
    char *recipient;

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    recipient = ui_current_recipient();

    switch (win_type)
    {
        case WIN_MUC:
            if (usr != NULL) {
                pcontact = muc_get_participant(recipient, usr);
                if (pcontact != NULL) {
                    cons_show_info(pcontact);
                } else {
                    cons_show("No such participant \"%s\" in room.", usr);
                }
            } else {
                cons_show("No nickname supplied to /info in chat room.");
            }
            break;
        case WIN_CHAT:
            if (usr != NULL) {
                cons_show("No parameter required for /info in chat.");
            } else {
                pcontact = roster_get_contact(recipient);
                if (pcontact != NULL) {
                    cons_show_info(pcontact);
                } else {
                    cons_show("No such contact \"%s\" in roster.", recipient);
                }
            }
            break;
        case WIN_PRIVATE:
            if (usr != NULL) {
                ui_current_print_line("No parameter required when in chat.");
            } else {
                Jid *jid = jid_create(recipient);
                pcontact = muc_get_participant(jid->barejid, jid->resourcepart);
                if (pcontact != NULL) {
                    cons_show_info(pcontact);
                } else {
                    cons_show("No such participant \"%s\" in room.", jid->resourcepart);
                }
                jid_destroy(jid);
            }
            break;
        case WIN_CONSOLE:
            if (usr != NULL) {
                usr_jid = roster_barejid_from_name(usr);
                if (usr_jid == NULL) {
                    usr_jid = usr;
                }
                pcontact = roster_get_contact(usr_jid);
                if (pcontact != NULL) {
                    cons_show_info(pcontact);
                } else {
                    cons_show("No such contact \"%s\" in roster.", usr);
                }
            } else {
                cons_show("Usage: %s", help.usage);
            }
            break;
        default:
            break;
    }

    return TRUE;
}

gboolean
cmd_caps(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();
    win_type_t win_type = ui_current_win_type();
    PContact pcontact = NULL;
    char *recipient;

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    switch (win_type)
    {
        case WIN_MUC:
            if (args[0] != NULL) {
                recipient = ui_current_recipient();
                pcontact = muc_get_participant(recipient, args[0]);
                if (pcontact != NULL) {
                    Resource *resource = p_contact_get_resource(pcontact, args[0]);
                    cons_show_caps(args[0], resource);
                } else {
                    cons_show("No such participant \"%s\" in room.", args[0]);
                }
            } else {
                cons_show("No nickname supplied to /caps in chat room.");
            }
            break;
        case WIN_CHAT:
        case WIN_CONSOLE:
            if (args[0] != NULL) {
                Jid *jid = jid_create(args[0]);

                if (jid->fulljid == NULL) {
                    cons_show("You must provide a full jid to the /caps command.");
                } else {
                    pcontact = roster_get_contact(jid->barejid);
                    if (pcontact == NULL) {
                        cons_show("Contact not found in roster: %s", jid->barejid);
                    } else {
                        Resource *resource = p_contact_get_resource(pcontact, jid->resourcepart);
                        if (resource == NULL) {
                            cons_show("Could not find resource %s, for contact %s", jid->barejid, jid->resourcepart);
                        } else {
                            cons_show_caps(jid->fulljid, resource);
                        }
                    }
                }
            } else {
                cons_show("You must provide a jid to the /caps command.");
            }
            break;
        case WIN_PRIVATE:
            if (args[0] != NULL) {
                cons_show("No parameter needed to /caps when in private chat.");
            } else {
                recipient = ui_current_recipient();
                Jid *jid = jid_create(recipient);
                if (jid) {
                    pcontact = muc_get_participant(jid->barejid, jid->resourcepart);
                    Resource *resource = p_contact_get_resource(pcontact, jid->resourcepart);
                    cons_show_caps(jid->resourcepart, resource);
                    jid_destroy(jid);
                }
            }
            break;
        default:
            break;
    }

    return TRUE;
}


gboolean
cmd_software(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();
    win_type_t win_type = ui_current_win_type();
    PContact pcontact = NULL;
    char *recipient;

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    switch (win_type)
    {
        case WIN_MUC:
            if (args[0] != NULL) {
                recipient = ui_current_recipient();
                pcontact = muc_get_participant(recipient, args[0]);
                if (pcontact != NULL) {
                    Jid *jid = jid_create_from_bare_and_resource(recipient, args[0]);
                    iq_send_software_version(jid->fulljid);
                    jid_destroy(jid);
                } else {
                    cons_show("No such participant \"%s\" in room.", args[0]);
                }
            } else {
                cons_show("No nickname supplied to /software in chat room.");
            }
            break;
        case WIN_CHAT:
        case WIN_CONSOLE:
            if (args[0] != NULL) {
                Jid *jid = jid_create(args[0]);

                if (jid == NULL || jid->fulljid == NULL) {
                    cons_show("You must provide a full jid to the /software command.");
                } else {
                    iq_send_software_version(jid->fulljid);
                }
                jid_destroy(jid);
            } else {
                cons_show("You must provide a jid to the /software command.");
            }
            break;
        case WIN_PRIVATE:
            if (args[0] != NULL) {
                cons_show("No parameter needed to /software when in private chat.");
            } else {
                recipient = ui_current_recipient();
                iq_send_software_version(recipient);
            }
            break;
        default:
            break;
    }

    return TRUE;
}

gboolean
cmd_join(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();
    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (args[0] == NULL) {
        cons_show("Usage: %s", help.usage);
        cons_show("");
        return TRUE;
    }

    Jid *room_arg = jid_create(args[0]);
    if (room_arg == NULL) {
        cons_show_error("Specified room has incorrect format.");
        cons_show("");
        return TRUE;
    }

    int num_args = g_strv_length(args);
    char *room = NULL;
    char *nick = NULL;
    char *passwd = NULL;
    GString *room_str = g_string_new("");
    char *account_name = jabber_get_account_name();
    ProfAccount *account = accounts_get_account(account_name);

    // full room jid supplied (room@server)
    if (room_arg->localpart != NULL) {
        room = args[0];

    // server not supplied (room), use account preference
    } else {
        g_string_append(room_str, args[0]);
        g_string_append(room_str, "@");
        g_string_append(room_str, account->muc_service);
        room = room_str->str;
    }

    // Additional args supplied
    if (num_args > 1) {
        char *opt1 = args[1];
        char *opt1val = args[2];
        char *opt2 = args[3];
        char *opt2val = args[4];
        if (opt1 != NULL) {
            if (opt1val == NULL) {
                cons_show("Usage: %s", help.usage);
                cons_show("");
                return TRUE;
            }
            if (strcmp(opt1, "nick") == 0) {
                nick = opt1val;
            } else if (strcmp(opt1, "password") == 0) {
                passwd = opt1val;
            } else {
                cons_show("Usage: %s", help.usage);
                cons_show("");
                return TRUE;
            }
            if (opt2 != NULL) {
                if (strcmp(opt2, "nick") == 0) {
                    nick = opt2val;
                } else if (strcmp(opt2, "password") == 0) {
                    passwd = opt2val;
                } else {
                    cons_show("Usage: %s", help.usage);
                    cons_show("");
                    return TRUE;
                }
            }
        }
    }

    // In the case that a nick wasn't provided by the optional args...
    if (nick == NULL) {
        nick = account->muc_nick;
    }

    if (!muc_room_is_active(room)) {
        presence_join_room(room, nick, passwd);
    }
    ui_room_join(room);
    muc_remove_invite(room);

    jid_destroy(room_arg);
    g_string_free(room_str, TRUE);
    account_free(account);

    return TRUE;
}

gboolean
cmd_invite(gchar **args, struct cmd_help_t help)
{
    char *contact = args[0];
    char *reason = args[1];
    char *room = NULL;
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (ui_current_win_type() != WIN_MUC) {
        cons_show("You must be in a chat room to send an invite.");
        return TRUE;
    }

    char *usr_jid = roster_barejid_from_name(contact);
    if (usr_jid == NULL) {
        usr_jid = contact;
    }
    room = ui_current_recipient();
    message_send_invite(room, usr_jid, reason);
    if (reason != NULL) {
        cons_show("Room invite sent, contact: %s, room: %s, reason: \"%s\".",
            contact, room, reason);
    } else {
        cons_show("Room invite sent, contact: %s, room: %s.",
            contact, room);
    }

    return TRUE;
}

gboolean
cmd_invites(gchar **args, struct cmd_help_t help)
{
    GSList *invites = muc_get_invites();
    cons_show_room_invites(invites);
    g_slist_free_full(invites, g_free);
    return TRUE;
}

gboolean
cmd_decline(gchar **args, struct cmd_help_t help)
{
    if (!muc_invites_include(args[0])) {
        cons_show("No such invite exists.");
    } else {
        muc_remove_invite(args[0]);
        cons_show("Declined invite to %s.", args[0]);
    }

    return TRUE;
}

gboolean
cmd_rooms(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (args[0] == NULL) {
        ProfAccount *account = accounts_get_account(jabber_get_account_name());
        iq_room_list_request(account->muc_service);
    } else {
        iq_room_list_request(args[0]);
    }

    return TRUE;
}

gboolean
cmd_bookmark(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    gchar *cmd = args[0];
    if (cmd == NULL) {
        cons_show("Usage: %s", help.usage);
        return TRUE;
    }

    /* TODO: /bookmark list room@server */

    if (strcmp(cmd, "list") == 0) {
        const GList *bookmarks = bookmark_get_list();
        cons_show_bookmarks(bookmarks);
    } else {
        gboolean autojoin = FALSE;
        gchar *jid = NULL;
        gchar *nick = NULL;
        int idx = 1;

        while (args[idx] != NULL) {
            gchar *opt = args[idx];

            if (strcmp(opt, "autojoin") == 0) {
                autojoin = TRUE;
            } else if (jid == NULL) {
                jid = opt;
            } else if (nick == NULL) {
                nick = opt;
            } else {
                cons_show("Usage: %s", help.usage);
            }

            ++idx;
        }

        if (jid == NULL) {
            win_type_t win_type = ui_current_win_type();

            if (win_type == WIN_MUC) {
                jid = ui_current_recipient();
                nick = muc_get_room_nick(jid);
            } else {
                cons_show("Usage: %s", help.usage);
                return TRUE;
            }
        }

        if (strcmp(cmd, "add") == 0) {
            gboolean added = bookmark_add(jid, nick, autojoin);
            if (added) {
                GString *msg = g_string_new("Bookmark added for ");
                g_string_append(msg, jid);
                if (nick != NULL) {
                    g_string_append(msg, ", nickname: ");
                    g_string_append(msg, nick);
                }
                if (autojoin) {
                    g_string_append(msg, ", autojoin enabled");
                }
                g_string_append(msg, ".");
                cons_show(msg->str);
                g_string_free(msg, TRUE);
            } else {
                cons_show("Bookmark updated for %s.", jid);
            }
        } else if (strcmp(cmd, "remove") == 0) {
            gboolean removed = bookmark_remove(jid, autojoin);
            if (removed) {
                if (autojoin) {
                    cons_show("Autojoin disabled for %s.", jid);
                } else {
                    cons_show("Bookmark removed for %s.", jid);
                }
            } else {
                cons_show("No bookmark exists for %s.", jid);
            }
        } else {
            cons_show("Usage: %s", help.usage);
        }
    }

    return TRUE;
}

gboolean
cmd_disco(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currenlty connected.");
        return TRUE;
    }

    GString *jid = g_string_new("");
    if (args[1] != NULL) {
        jid = g_string_append(jid, args[1]);
    } else {
        Jid *jidp = jid_create(jabber_get_fulljid());
        jid = g_string_append(jid, jidp->domainpart);
        jid_destroy(jidp);
    }

    if (g_strcmp0(args[0], "info") == 0) {
        iq_disco_info_request(jid->str);
    } else {
        iq_disco_items_request(jid->str);
    }

    g_string_free(jid, TRUE);

    return TRUE;
}

gboolean
cmd_nick(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }
    if (ui_current_win_type() != WIN_MUC) {
        cons_show("You can only change your nickname in a chat room window.");
        return TRUE;
    }

    char *room = ui_current_recipient();
    char *nick = args[0];
    presence_change_room_nick(room, nick);

    return TRUE;
}

gboolean
cmd_alias(gchar **args, struct cmd_help_t help)
{
    char *subcmd = args[0];

    if (strcmp(subcmd, "add") == 0) {
        char *alias = args[1];
        if (alias == NULL) {
            cons_show("Usage: %s", help.usage);
            return TRUE;
        } else {
            GString *ac_value = g_string_new("/");
            g_string_append(ac_value, alias);

            char *value = args[2];
            if (value == NULL) {
                cons_show("Usage: %s", help.usage);
                g_string_free(ac_value, TRUE);
                return TRUE;
            } else if (cmd_exists(ac_value->str)) {
                cons_show("Command or alias '%s' already exists.", ac_value->str);
                g_string_free(ac_value, TRUE);
                return TRUE;
            } else {
                prefs_add_alias(alias, value);
                cmd_autocomplete_add(ac_value->str);
                cmd_alias_add(alias);
                cons_show("Command alias added /%s -> %s", alias, value);
                g_string_free(ac_value, TRUE);
                return TRUE;
            }
        }
    } else if (strcmp(subcmd, "remove") == 0) {
        char *alias = args[1];
        if (alias == NULL) {
            cons_show("Usage: %s", help.usage);
            return TRUE;
        } else {
            gboolean removed = prefs_remove_alias(alias);
            if (!removed) {
                cons_show("No such command alias /%s", alias);
            } else {
                GString *ac_value = g_string_new("/");
                g_string_append(ac_value, alias);
                cmd_autocomplete_remove(ac_value->str);
                cmd_alias_remove(alias);
                g_string_free(ac_value, TRUE);
                cons_show("Command alias removed -> /%s", alias);
            }
            return TRUE;
        }
    } else if (strcmp(subcmd, "list") == 0) {
        GList *aliases = prefs_get_aliases();
        cons_show_aliases(aliases);
        prefs_free_aliases(aliases);
        return TRUE;
    } else {
        cons_show("Usage: %s", help.usage);
        return TRUE;
    }
}

gboolean
cmd_tiny(gchar **args, struct cmd_help_t help)
{
    char *url = args[0];
    win_type_t win_type = ui_current_win_type();

    if (!tinyurl_valid(url)) {
        GString *error = g_string_new("/tiny, badly formed URL: ");
        g_string_append(error, url);
        cons_show_error(error->str);
        if (win_type != WIN_CONSOLE) {
            ui_current_error_line(error->str);
        }
        g_string_free(error, TRUE);
    } else if (win_type != WIN_CONSOLE) {
        char *tiny = tinyurl_get(url);

        if (tiny != NULL) {
            if (win_type == WIN_CHAT) {
                char *recipient = ui_current_recipient();
#ifdef HAVE_LIBOTR
                if (otr_is_secure(recipient)) {
                    char *encrypted = otr_encrypt_message(recipient, tiny);
                    if (encrypted != NULL) {
                        message_send(encrypted, recipient);
                        otr_free_message(encrypted);
                        if (prefs_get_boolean(PREF_CHLOG)) {
                            const char *jid = jabber_get_fulljid();
                            Jid *jidp = jid_create(jid);
                            if (strcmp(prefs_get_string(PREF_OTR_LOG), "on") == 0) {
                                chat_log_chat(jidp->barejid, recipient, tiny, PROF_OUT_LOG, NULL);
                            } else if (strcmp(prefs_get_string(PREF_OTR_LOG), "redact") == 0) {
                                chat_log_chat(jidp->barejid, recipient, "[redacted]", PROF_OUT_LOG, NULL);
                            }
                            jid_destroy(jidp);
                        }

                        ui_outgoing_msg("me", recipient, tiny);
                    } else {
                        cons_show_error("Failed to send message.");
                    }
                } else {
                    message_send(tiny, recipient);
                    if (prefs_get_boolean(PREF_CHLOG)) {
                        const char *jid = jabber_get_fulljid();
                        Jid *jidp = jid_create(jid);
                        chat_log_chat(jidp->barejid, recipient, tiny, PROF_OUT_LOG, NULL);
                        jid_destroy(jidp);
                    }

                    ui_outgoing_msg("me", recipient, tiny);
                }
#else
                message_send(tiny, recipient);
                if (prefs_get_boolean(PREF_CHLOG)) {
                    const char *jid = jabber_get_fulljid();
                    Jid *jidp = jid_create(jid);
                    chat_log_chat(jidp->barejid, recipient, tiny, PROF_OUT_LOG, NULL);
                    jid_destroy(jidp);
                }

                ui_outgoing_msg("me", recipient, tiny);
#endif
            } else if (win_type == WIN_PRIVATE) {
                char *recipient = ui_current_recipient();
                message_send(tiny, recipient);
                ui_outgoing_msg("me", recipient, tiny);
            } else { // groupchat
                char *recipient = ui_current_recipient();
                message_send_groupchat(tiny, recipient);
            }
            free(tiny);
        } else {
            cons_show_error("Couldn't get tinyurl.");
        }
    } else {
        cons_show("/tiny can only be used in chat windows");
    }

    return TRUE;
}

gboolean
cmd_clear(gchar **args, struct cmd_help_t help)
{
    ui_clear_current();
    return TRUE;
}

gboolean
cmd_close(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();
    int index = 0;
    int count = 0;

    if (args[0] == NULL) {
        index = ui_current_win_index();
    } else if (strcmp(args[0], "all") == 0) {
        count = ui_close_all_wins();
        if (count == 0) {
            cons_show("No windows to close.");
        } else if (count == 1) {
            cons_show("Closed 1 window.");
        } else {
            cons_show("Closed %d windows.", count);
        }
        return TRUE;
    } else if (strcmp(args[0], "read") == 0) {
        count = ui_close_read_wins();
        if (count == 0) {
            cons_show("No windows to close.");
        } else if (count == 1) {
            cons_show("Closed 1 window.");
        } else {
            cons_show("Closed %d windows.", count);
        }
        return TRUE;
    } else {
        index = atoi(args[0]);
    }

    if (index < 0 || index == 10) {
        cons_show("No such window exists.");
        return TRUE;
    }

    if (index == 1) {
        cons_show("Cannot close console window.");
        return TRUE;
    }

    if (index == 0) {
        index = 10;
    }

    if (!ui_win_exists(index)) {
        cons_show("Window is not open.");
        return TRUE;
    }

    // handle leaving rooms, or chat
    if (conn_status == JABBER_CONNECTED) {
        ui_close_connected_win(index);
    }

    // close the window
    ui_close_win(index);
    cons_show("Closed window %d", index);

    return TRUE;
}

gboolean
cmd_leave(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();
    win_type_t win_type = ui_current_win_type();
    int index = ui_current_win_index();

    if (win_type != WIN_MUC) {
        cons_show("You can only use the /leave command in a chat room.");
        cons_alert();
        return TRUE;
    }

    // handle leaving rooms, or chat
    if (conn_status == JABBER_CONNECTED) {
        ui_close_connected_win(index);
    }

    // close the window
    ui_close_win(index);

    return TRUE;
}

gboolean
cmd_beep(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help, "Sound", PREF_BEEP);
}

gboolean
cmd_states(gchar **args, struct cmd_help_t help)
{
    gboolean result = _cmd_set_boolean_preference(args[0], help, "Sending chat states",
        PREF_STATES);

    // if disabled, disable outtype and gone
    if (result == TRUE && (strcmp(args[0], "off") == 0)) {
        prefs_set_boolean(PREF_OUTTYPE, FALSE);
        prefs_set_gone(0);
    }

    return result;
}

gboolean
cmd_titlebar(gchar **args, struct cmd_help_t help)
{
    if (g_strcmp0(args[0], "off") == 0) {
        ui_clear_win_title();
    }
    return _cmd_set_boolean_preference(args[0], help, "Titlebar", PREF_TITLEBAR);
}

gboolean
cmd_outtype(gchar **args, struct cmd_help_t help)
{
    gboolean result = _cmd_set_boolean_preference(args[0], help,
        "Sending typing notifications", PREF_OUTTYPE);

    // if enabled, enable states
    if (result == TRUE && (strcmp(args[0], "on") == 0)) {
        prefs_set_boolean(PREF_STATES, TRUE);
    }

    return result;
}

gboolean
cmd_gone(gchar **args, struct cmd_help_t help)
{
    char *value = args[0];

    gint period = atoi(value);
    prefs_set_gone(period);
    if (period == 0) {
        cons_show("Automatic leaving conversations after period disabled.");
    } else if (period == 1) {
        cons_show("Leaving conversations after 1 minute of inactivity.");
    } else {
        cons_show("Leaving conversations after %d minutes of inactivity.", period);
    }

    // if enabled, enable states
    if (period > 0) {
        prefs_set_boolean(PREF_STATES, TRUE);
    }

    return TRUE;
}


gboolean
cmd_notify(gchar **args, struct cmd_help_t help)
{
    char *kind = args[0];
    char *value = args[1];

    // bad kind
    if ((strcmp(kind, "message") != 0) && (strcmp(kind, "typing") != 0) &&
            (strcmp(kind, "remind") != 0) && (strcmp(kind, "invite") != 0) &&
            (strcmp(kind, "sub") != 0)) {
        cons_show("Usage: %s", help.usage);

    // set message setting
    } else if (strcmp(kind, "message") == 0) {
        if (strcmp(value, "on") == 0) {
            cons_show("Message notifications enabled.");
            prefs_set_boolean(PREF_NOTIFY_MESSAGE, TRUE);
        } else if (strcmp(value, "off") == 0) {
            cons_show("Message notifications disabled.");
            prefs_set_boolean(PREF_NOTIFY_MESSAGE, FALSE);
        } else {
            cons_show("Usage: /notify message on|off");
        }

    // set typing setting
    } else if (strcmp(kind, "typing") == 0) {
        if (strcmp(value, "on") == 0) {
            cons_show("Typing notifications enabled.");
            prefs_set_boolean(PREF_NOTIFY_TYPING, TRUE);
        } else if (strcmp(value, "off") == 0) {
            cons_show("Typing notifications disabled.");
            prefs_set_boolean(PREF_NOTIFY_TYPING, FALSE);
        } else {
            cons_show("Usage: /notify typing on|off");
        }

    // set invite setting
    } else if (strcmp(kind, "invite") == 0) {
        if (strcmp(value, "on") == 0) {
            cons_show("Chat room invite notifications enabled.");
            prefs_set_boolean(PREF_NOTIFY_INVITE, TRUE);
        } else if (strcmp(value, "off") == 0) {
            cons_show("Chat room invite notifications disabled.");
            prefs_set_boolean(PREF_NOTIFY_INVITE, FALSE);
        } else {
            cons_show("Usage: /notify invite on|off");
        }

    // set subscription setting
    } else if (strcmp(kind, "sub") == 0) {
        if (strcmp(value, "on") == 0) {
            cons_show("Subscription notifications enabled.");
            prefs_set_boolean(PREF_NOTIFY_SUB, TRUE);
        } else if (strcmp(value, "off") == 0) {
            cons_show("Subscription notifications disabled.");
            prefs_set_boolean(PREF_NOTIFY_SUB, FALSE);
        } else {
            cons_show("Usage: /notify sub on|off");
        }

    // set remind setting
    } else if (strcmp(kind, "remind") == 0) {
        gint period = atoi(value);
        prefs_set_notify_remind(period);
        if (period == 0) {
            cons_show("Message reminders disabled.");
        } else if (period == 1) {
            cons_show("Message reminder period set to 1 second.");
        } else {
            cons_show("Message reminder period set to %d seconds.", period);
        }

    } else {
        cons_show("Unknown command: %s.", kind);
    }

    return TRUE;
}

gboolean
cmd_log(gchar **args, struct cmd_help_t help)
{
    char *subcmd = args[0];
    char *value = args[1];
    int intval;

    if (strcmp(subcmd, "maxsize") == 0) {
        if (_strtoi(value, &intval, PREFS_MIN_LOG_SIZE, INT_MAX) == 0) {
            prefs_set_max_log_size(intval);
            cons_show("Log maxinum size set to %d bytes", intval);
        }
    } else {
        cons_show("Usage: %s", help.usage);
    }

    /* TODO: make 'level' subcommand for debug level */

    return TRUE;
}

gboolean
cmd_reconnect(gchar **args, struct cmd_help_t help)
{
    char *value = args[0];
    int intval;

    if (_strtoi(value, &intval, 0, INT_MAX) == 0) {
        prefs_set_reconnect(intval);
        if (intval == 0) {
            cons_show("Reconnect disabled.", intval);
        } else {
            cons_show("Reconnect interval set to %d seconds.", intval);
        }
    } else {
        cons_show("Usage: %s", help.usage);
    }

    return TRUE;
}

gboolean
cmd_autoping(gchar **args, struct cmd_help_t help)
{
    char *value = args[0];
    int intval;

    if (_strtoi(value, &intval, 0, INT_MAX) == 0) {
        prefs_set_autoping(intval);
        iq_set_autoping(intval);
        if (intval == 0) {
            cons_show("Autoping disabled.", intval);
        } else {
            cons_show("Autoping interval set to %d seconds.", intval);
        }
    } else {
        cons_show("Usage: %s", help.usage);
    }

    return TRUE;
}

gboolean
cmd_autoaway(gchar **args, struct cmd_help_t help)
{
    char *setting = args[0];
    char *value = args[1];
    int minutesval;

    if ((strcmp(setting, "mode") != 0) && (strcmp(setting, "time") != 0) &&
            (strcmp(setting, "message") != 0) && (strcmp(setting, "check") != 0)) {
        cons_show("Setting must be one of 'mode', 'time', 'message' or 'check'");
        return TRUE;
    }

    if (strcmp(setting, "mode") == 0) {
        if ((strcmp(value, "idle") != 0) && (strcmp(value, "away") != 0) &&
                (strcmp(value, "off") != 0)) {
            cons_show("Mode must be one of 'idle', 'away' or 'off'");
        } else {
            prefs_set_string(PREF_AUTOAWAY_MODE, value);
            cons_show("Auto away mode set to: %s.", value);
        }

        return TRUE;
    }

    if (strcmp(setting, "time") == 0) {
        if (_strtoi(value, &minutesval, 1, INT_MAX) == 0) {
            prefs_set_autoaway_time(minutesval);
            cons_show("Auto away time set to: %d minutes.", minutesval);
        }

        return TRUE;
    }

    if (strcmp(setting, "message") == 0) {
        if (strcmp(value, "off") == 0) {
            prefs_set_string(PREF_AUTOAWAY_MESSAGE, NULL);
            cons_show("Auto away message cleared.");
        } else {
            prefs_set_string(PREF_AUTOAWAY_MESSAGE, value);
            cons_show("Auto away message set to: \"%s\".", value);
        }

        return TRUE;
    }

    if (strcmp(setting, "check") == 0) {
        return _cmd_set_boolean_preference(value, help, "Online check",
            PREF_AUTOAWAY_CHECK);
    }

    return TRUE;
}

gboolean
cmd_priority(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    char *value = args[0];
    int intval;

    if (_strtoi(value, &intval, -128, 127) == 0) {
        accounts_set_priority_all(jabber_get_account_name(), intval);
        resource_presence_t last_presence = accounts_get_last_presence(jabber_get_account_name());
        presence_update(last_presence, jabber_get_presence_message(), 0);
        cons_show("Priority set to %d.", intval);
    }

    return TRUE;
}

gboolean
cmd_statuses(gchar **args, struct cmd_help_t help)
{
    if (strcmp(args[0], "console") != 0 &&
            strcmp(args[0], "chat") != 0 &&
            strcmp(args[0], "muc") != 0) {
        cons_show("Usage: %s", help.usage);
        return TRUE;
    }

    if (strcmp(args[1], "all") != 0 &&
            strcmp(args[1], "online") != 0 &&
            strcmp(args[1], "none") != 0) {
        cons_show("Usage: %s", help.usage);
        return TRUE;
    }

    if (strcmp(args[0], "console") == 0) {
        prefs_set_string(PREF_STATUSES_CONSOLE, args[1]);
        if (strcmp(args[1], "all") == 0) {
            cons_show("All presence updates will appear in the console.");
        } else if (strcmp(args[1], "online") == 0) {
            cons_show("Only online/offline presence updates will appear in the console.");
        } else {
            cons_show("Presence updates will not appear in the console.");
        }
    }

    if (strcmp(args[0], "chat") == 0) {
        prefs_set_string(PREF_STATUSES_CHAT, args[1]);
        if (strcmp(args[1], "all") == 0) {
            cons_show("All presence updates will appear in chat windows.");
        } else if (strcmp(args[1], "online") == 0) {
            cons_show("Only online/offline presence updates will appear in chat windows.");
        } else {
            cons_show("Presence updates will not appear in chat windows.");
        }
    }

    if (strcmp(args[0], "muc") == 0) {
        prefs_set_string(PREF_STATUSES_MUC, args[1]);
        if (strcmp(args[1], "all") == 0) {
            cons_show("All presence updates will appear in chat room windows.");
        } else if (strcmp(args[1], "online") == 0) {
            cons_show("Only join/leave presence updates will appear in chat room windows.");
        } else {
            cons_show("Presence updates will not appear in chat room windows.");
        }
    }

    return TRUE;
}

gboolean
cmd_vercheck(gchar **args, struct cmd_help_t help)
{
    int num_args = g_strv_length(args);

    if (num_args == 0) {
        cons_check_version(TRUE);
        return TRUE;
    } else {
        return _cmd_set_boolean_preference(args[0], help,
            "Version checking", PREF_VERCHECK);
    }
}

gboolean
cmd_flash(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Screen flash", PREF_FLASH);
}

gboolean
cmd_intype(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Show contact typing", PREF_INTYPE);
}

gboolean
cmd_splash(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Splash screen", PREF_SPLASH);
}

gboolean
cmd_autoconnect(gchar **args, struct cmd_help_t help)
{
    if (strcmp(args[0], "off") == 0) {
        prefs_set_string(PREF_CONNECT_ACCOUNT, NULL);
        cons_show("Autoconnect account disabled.");
    } else if (strcmp(args[0], "set") == 0) {
        prefs_set_string(PREF_CONNECT_ACCOUNT, args[1]);
        cons_show("Autoconnect account set to: %s.", args[1]);
    } else {
        cons_show("Usage: %s", help.usage);
    }
    return true;
}

gboolean
cmd_chlog(gchar **args, struct cmd_help_t help)
{
    gboolean result = _cmd_set_boolean_preference(args[0], help,
        "Chat logging", PREF_CHLOG);

    // if set to off, disable history
    if (result == TRUE && (strcmp(args[0], "off") == 0)) {
        prefs_set_boolean(PREF_HISTORY, FALSE);
    }

    return result;
}

gboolean
cmd_grlog(gchar **args, struct cmd_help_t help)
{
    gboolean result = _cmd_set_boolean_preference(args[0], help,
        "Groupchat logging", PREF_GRLOG);

    return result;
}

gboolean
cmd_mouse(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Mouse handling", PREF_MOUSE);
}

gboolean
cmd_history(gchar **args, struct cmd_help_t help)
{
    gboolean result = _cmd_set_boolean_preference(args[0], help,
        "Chat history", PREF_HISTORY);

    // if set to on, set chlog
    if (result == TRUE && (strcmp(args[0], "on") == 0)) {
        prefs_set_boolean(PREF_CHLOG, TRUE);
    }

    return result;
}

gboolean
cmd_away(gchar **args, struct cmd_help_t help)
{
    _update_presence(RESOURCE_AWAY, "away", args);
    return TRUE;
}

gboolean
cmd_online(gchar **args, struct cmd_help_t help)
{
    _update_presence(RESOURCE_ONLINE, "online", args);
    return TRUE;
}

gboolean
cmd_dnd(gchar **args, struct cmd_help_t help)
{
    _update_presence(RESOURCE_DND, "dnd", args);
    return TRUE;
}

gboolean
cmd_chat(gchar **args, struct cmd_help_t help)
{
    _update_presence(RESOURCE_CHAT, "chat", args);
    return TRUE;
}

gboolean
cmd_xa(gchar **args, struct cmd_help_t help)
{
    _update_presence(RESOURCE_XA, "xa", args);
    return TRUE;
}

gboolean
cmd_otr(gchar **args, struct cmd_help_t help)
{
#ifdef HAVE_LIBOTR
    if (args[0] == NULL) {
        cons_show("Usage: %s", help.usage);
        return TRUE;
    }

    if (strcmp(args[0], "log") == 0) {
        char *choice = args[1];
        if (g_strcmp0(choice, "on") == 0) {
            prefs_set_string(PREF_OTR_LOG, "on");
            cons_show("OTR messages will be logged as plaintext.");
            if (!prefs_get_boolean(PREF_CHLOG)) {
                cons_show("Chat logging is currently disabled, use '/chlog on' to enable.");
            }
        } else if (g_strcmp0(choice, "off") == 0) {
            prefs_set_string(PREF_OTR_LOG, "off");
            cons_show("OTR message logging disabled.");
        } else if (g_strcmp0(choice, "redact") == 0) {
            prefs_set_string(PREF_OTR_LOG, "redact");
            cons_show("OTR messages will be logged as '[redacted]'.");
            if (!prefs_get_boolean(PREF_CHLOG)) {
                cons_show("Chat logging is currently disabled, use '/chlog on' to enable.");
            }
        } else {
            cons_show("Usage: %s", help.usage);
        }
        return TRUE;
    } else if (strcmp(args[0], "warn") == 0) {
        gboolean result =  _cmd_set_boolean_preference(args[1], help,
            "OTR warning message", PREF_OTR_WARN);
        ui_current_update_virtual();
        return result;
    } else if (strcmp(args[0], "libver") == 0) {
        char *version = otr_libotr_version();
        cons_show("Using libotr version %s", version);
        return TRUE;
    }

    if (jabber_get_connection_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    if (strcmp(args[0], "gen") == 0) {
        ProfAccount *account = accounts_get_account(jabber_get_account_name());
        otr_keygen(account);
        return TRUE;
    } else if (strcmp(args[0], "myfp") == 0) {
        if (!otr_key_loaded()) {
            ui_current_print_formatted_line('!', 0, "You have not generated or loaded a private key, use '/otr gen'");
        } else {
            char *fingerprint = otr_get_my_fingerprint();
            ui_current_print_formatted_line('!', 0, "Your OTR fingerprint: %s", fingerprint);
            free(fingerprint);
        }
        return TRUE;
    } else if (strcmp(args[0], "theirfp") == 0) {
        win_type_t win_type = ui_current_win_type();

        if (win_type != WIN_CHAT) {
            ui_current_print_line("You must be in a regular chat window to view a recipient's fingerprint.");
        } else if (!ui_current_win_is_otr()) {
            ui_current_print_formatted_line('!', 0, "You are not currently in an OTR session.");
        } else {
            char *recipient = ui_current_recipient();
            char *fingerprint = otr_get_their_fingerprint(recipient);
            ui_current_print_formatted_line('!', 0, "%s's OTR fingerprint: %s", recipient, fingerprint);
            free(fingerprint);
        }
        return TRUE;
    } else if (strcmp(args[0], "start") == 0) {
        if (args[1] != NULL) {
            char *contact = args[1];
            char *barejid = roster_barejid_from_name(contact);
            if (barejid == NULL) {
                barejid = contact;
            }

            if (prefs_get_boolean(PREF_STATES)) {
                if (!chat_session_exists(barejid)) {
                    chat_session_start(barejid, TRUE);
                }
            }

            ui_new_chat_win(barejid);

            if (ui_current_win_is_otr()) {
                ui_current_print_formatted_line('!', 0, "You are already in an OTR session.");
            } else {
                if (!otr_key_loaded()) {
                    ui_current_print_formatted_line('!', 0, "You have not generated or loaded a private key, use '/otr gen'");
                } else if (!otr_is_secure(barejid)) {
                    char *otr_query_message = otr_start_query();
                    message_send(otr_query_message, barejid);
                } else {
                    ui_gone_secure(barejid, otr_is_trusted(barejid));
                }
            }
        } else {
            win_type_t win_type = ui_current_win_type();

            if (win_type != WIN_CHAT) {
                ui_current_print_line("You must be in a regular chat window to start an OTR session.");
            } else if (ui_current_win_is_otr()) {
                ui_current_print_formatted_line('!', 0, "You are already in an OTR session.");
            } else {
                if (!otr_key_loaded()) {
                    ui_current_print_formatted_line('!', 0, "You have not generated or loaded a private key, use '/otr gen'");
                } else {
                    char *recipient = ui_current_recipient();
                    char *otr_query_message = otr_start_query();
                    message_send(otr_query_message, recipient);
                }
            }
        }
        return TRUE;
    } else if (strcmp(args[0], "end") == 0) {
        win_type_t win_type = ui_current_win_type();

        if (win_type != WIN_CHAT) {
            ui_current_print_line("You must be in a regular chat window to use OTR.");
        } else if (!ui_current_win_is_otr()) {
            ui_current_print_formatted_line('!', 0, "You are not currently in an OTR session.");
        } else {
            char *recipient = ui_current_recipient();
            ui_gone_insecure(recipient);
            otr_end_session(recipient);
        }
        return TRUE;
    } else if (strcmp(args[0], "trust") == 0) {
        win_type_t win_type = ui_current_win_type();

        if (win_type != WIN_CHAT) {
            ui_current_print_line("You must be in an OTR session to trust a recipient.");
        } else if (!ui_current_win_is_otr()) {
            ui_current_print_formatted_line('!', 0, "You are not currently in an OTR session.");
        } else {
            char *recipient = ui_current_recipient();
            ui_trust(recipient);
            otr_trust(recipient);
        }
        return TRUE;
    } else if (strcmp(args[0], "untrust") == 0) {
        win_type_t win_type = ui_current_win_type();

        if (win_type != WIN_CHAT) {
            ui_current_print_line("You must be in an OTR session to untrust a recipient.");
        } else if (!ui_current_win_is_otr()) {
            ui_current_print_formatted_line('!', 0, "You are not currently in an OTR session.");
        } else {
            char *recipient = ui_current_recipient();
            ui_untrust(recipient);
            otr_untrust(recipient);
        }
        return TRUE;

    } else {
        cons_show("Usage: %s", help.usage);
        return TRUE;
    }
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

// helper function for status change commands
static void
_update_presence(const resource_presence_t resource_presence,
    const char * const show, gchar **args)
{
    char *msg = NULL;
    int num_args = g_strv_length(args);
    if (num_args == 1) {
        msg = args[0];
    }

    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
    } else {
        presence_update(resource_presence, msg, 0);

        contact_presence_t contact_presence = contact_presence_from_resource_presence(resource_presence);
        title_bar_set_presence(contact_presence);

        gint priority = accounts_get_priority_for_presence_type(jabber_get_account_name(), resource_presence);
        if (msg != NULL) {
            cons_show("Status set to %s (priority %d), \"%s\".", show, priority, msg);
        } else {
            cons_show("Status set to %s (priority %d).", show, priority);
        }
    }

}

// helper function for boolean preference commands
static gboolean
_cmd_set_boolean_preference(gchar *arg, struct cmd_help_t help,
    const char * const display, preference_t pref)
{
    GString *enabled = g_string_new(display);
    g_string_append(enabled, " enabled.");

    GString *disabled = g_string_new(display);
    g_string_append(disabled, " disabled.");

    if (arg == NULL) {
        char usage[strlen(help.usage) + 8];
        sprintf(usage, "Usage: %s", help.usage);
        cons_show(usage);
    } else if (strcmp(arg, "on") == 0) {
        cons_show(enabled->str);
        prefs_set_boolean(pref, TRUE);
    } else if (strcmp(arg, "off") == 0) {
        cons_show(disabled->str);
        prefs_set_boolean(pref, FALSE);
    } else {
        char usage[strlen(help.usage) + 8];
        sprintf(usage, "Usage: %s", help.usage);
        cons_show(usage);
    }

    g_string_free(enabled, TRUE);
    g_string_free(disabled, TRUE);

    return TRUE;
}

static int
_strtoi(char *str, int *saveptr, int min, int max)
{
    char *ptr;
    int val;

    errno = 0;
    val = (int)strtol(str, &ptr, 0);
    if (errno != 0 || *str == '\0' || *ptr != '\0') {
        cons_show("Could not convert \"%s\" to a number.", str);
        return -1;
    } else if (val < min || val > max) {
        cons_show("Value %s out of range. Must be in %d..%d.", str, min, max);
        return -1;
    }

    *saveptr = val;

    return 0;
}

static void
_cmd_show_filtered_help(char *heading, gchar *cmd_filter[], int filter_size)
{
    cons_show("");
    cons_show("%s", heading);
    cons_show("");

    GList *ordered_commands = NULL;
    int i;
    for (i = 0; i < filter_size; i++) {
        Command *cmd = g_hash_table_lookup(commands, cmd_filter[i]);
        ordered_commands = g_list_insert_sorted(ordered_commands, cmd, (GCompareFunc)_compare_commands);
    }

    GList *curr = ordered_commands;
    while (curr != NULL) {
        Command *cmd = curr->data;
        cons_show("%-12s: %s", cmd->cmd, cmd->help.short_help);
        curr = g_list_next(curr);
    }
    g_list_free(ordered_commands);
    g_list_free(curr);

    cons_show("");
    cons_show("Use /help [command] without the leading slash, for help on a specific command");
    cons_show("");
}

static
gint _compare_commands(Command *a, Command *b)
{
    const char * utf8_str_a = a->cmd;
    const char * utf8_str_b = b->cmd;

    gchar *key_a = g_utf8_collate_key(utf8_str_a, -1);
    gchar *key_b = g_utf8_collate_key(utf8_str_b, -1);

    gint result = g_strcmp0(key_a, key_b);

    g_free(key_a);
    g_free(key_b);

    return result;
}

