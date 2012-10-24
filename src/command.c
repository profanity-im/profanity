/*
 * command.c
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "command.h"
#include "common.h"
#include "contact.h"
#include "contact_list.h"
#include "chat_log.h"
#include "history.h"
#include "jabber.h"
#include "log.h"
#include "preferences.h"
#include "prof_autocomplete.h"
#include "tinyurl.h"
#include "ui.h"

/*
 * Command structure
 *
 * cmd - The command string including leading '/'
 * func - The function to execute for the command
 * help - A help struct containing usage info etc
 */
struct cmd_t {
    const gchar *cmd;
    gboolean (*func)(const char * const inp, struct cmd_help_t help);
    struct cmd_help_t help;
};

static struct cmd_t * _cmd_get_command(const char * const command);
static void _update_presence(const jabber_presence_t presence,
    const char * const show, const char * const inp);
static gboolean _cmd_set_boolean_preference(const char * const inp,
    struct cmd_help_t help, const char * const cmd_str, const char * const display,
    void (*set_func)(gboolean));

// command prototypes
static gboolean _cmd_quit(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_help(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_about(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_prefs(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_who(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_connect(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_msg(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_tiny(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_close(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_set_beep(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_set_notify(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_set_typing(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_set_flash(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_set_showsplash(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_set_chlog(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_set_history(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_set_remind(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_vercheck(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_away(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_online(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_dnd(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_chat(const char * const inp, struct cmd_help_t help);
static gboolean _cmd_xa(const char * const inp, struct cmd_help_t help);

/*
 * The commands are broken down into three groups:
 * Main commands
 * Commands to change preferences
 * Commands to change users status
 */
static struct cmd_t main_commands[] =
{
    { "/help",
        _cmd_help,
        { "/help [area|command]", "Show help summary, or help on a specific area or command",
        { "/help [area|command]",
          "--------------------",
          "Show help options.",
          "Specify an area (basic, status, settings, navigation) for more help on that area.",
          "Specify the command if you want more detailed help on a specific command.",
          "",
          "Example : /help connect",
          "Example : /help settings",
          NULL } } },

    { "/about",
        _cmd_about,
        { "/about", "About Profanity",
        { "/about",
          "------",
          "Show versioning and license information.",
          NULL  } } },

    { "/connect",
        _cmd_connect,
        { "/connect user@host", "Login to jabber.",
        { "/connect user@host",
          "------------------",
          "Connect to the jabber server at host using the username user.",
          "Profanity should work with any XMPP (Jabber) compliant chat host.",
          "You can use tab completion to autocomplete any logins you have used before.",
          "",
          "Example: /connect myuser@gmail.com",
          NULL  } } },

    { "/prefs",
        _cmd_prefs,
        { "/prefs", "Show current preferences.",
        { "/prefs",
          "------",
          "List all current user preference settings.",
          "User preferences are stored at:",
          "",
          "    ~/.profanity/config",
          "",
          "Preference changes made using the various commands take effect immediately,",
          "you will need to restart Profanity for config file edits to take effect.",
          NULL } } },

    { "/msg",
        _cmd_msg,
        { "/msg user@host mesg", "Send mesg to user.",
        { "/msg user@host mesg",
          "-------------------",
          "Send a message to the user specified.",
          "Use tab completion to autocomplete online contacts.",
          "If there is no current chat with the recipient, a new chat window",
          "will be opened, and highlighted in the status bar at the bottom.",
          "pressing the corresponding F key will take you to that window.",
          "This command can be called from any window, including chat with other users.",
          "",
          "Example : /msg boothj5@gmail.com Hey, here's a message!",
          NULL } } },

    { "/tiny",
        _cmd_tiny,
        { "/tiny url", "Send url as tinyurl in current chat.",
        { "/tiny url",
          "---------",
          "Send the url as a tiny url.",
          "This command can only be called when in a chat window,",
          "not from the console.",
          "",
          "Example : /tiny http://www.google.com",
          NULL } } },

    { "/who",
        _cmd_who,
        { "/who [status]", "Show contacts with chosen status.",
        { "/who [status]",
          "-------------",
          "Show contacts with the specified status, no status shows all contacts.",
          "Possible statuses are: online, offline, away, dnd, xa, chat.",
          "online includes: chat, dnd, away, xa.",
          NULL } } },

    { "/close",
        _cmd_close,
        { "/close", "Close current chat window.",
        { "/close",
          "------",
          "Close the current chat window, no message is sent to the recipient,",
          "The chat window will become available for new chats.",
          NULL } } },

    { "/quit",
        _cmd_quit,
        { "/quit", "Quit Profanity.",
        { "/quit",
          "-----",
          "Logout of any current sessions, and quit Profanity.",
          NULL } } }
};

static struct cmd_t setting_commands[] =
{
    { "/beep",
        _cmd_set_beep,
        { "/beep on|off", "Enable/disable sound notifications.",
        { "/beep on|off",
          "------------",
          "Switch the terminal bell on or off.",
          "The bell will sound when incoming messages are received.",
          "If the terminal does not support sounds, it may attempt to",
          "flash the screen instead.",
          "",
          "Config file section : [ui]",
          "Config file value :   beep=true|false",
          NULL } } },

    { "/notify",
        _cmd_set_notify,
        { "/notify on|off", "Enable/disable message notifications.",
        { "/notify on|off",
          "--------------",
          "Switch the message notifications on or off.",
          "The notification will appear for all incoming messages.",
          "The desktop environment must support desktop notifications.",
          "",
          "Config file section : [ui]",
          "Config file value :   notify=true|false",
          NULL } } },

    { "/typing",
        _cmd_set_typing,
        { "/typing on|off", "Enable/disable typing notifications.",
        { "/typing on|off",
          "--------------",
          "Switch typing notifications on or off for incoming messages",
          "If desktop notifications are also enabled you will receive them",
          "for when users are typing a message to you.",
          "",
          "Config file section : [ui]",
          "Config file value :   typing=true|false",
          NULL } } },

    { "/remind",
        _cmd_set_remind,
        { "/remind seconds", "Set message reminder period in seconds.",
        { "/remind seconds",
          "--------------",
          "Set the period for new message reminders as desktop notifications.",
          "The value is in seconds, a setting of 0 will disable the feature.",
          "The desktop environment must support desktop notifications.",
          "",
          "Config file section : [ui]",
          "Config file value :   remind=seconds",
          NULL } } },

    { "/flash",
        _cmd_set_flash,
        { "/flash on|off", "Enable/disable screen flash notifications.",
        { "/flash on|off",
          "-------------",
          "Make the terminal flash when incoming messages are recieved.",
          "The flash will only occur if you are not in the chat window associated",
          "with the user sending the message.",
          "The terminal must support flashing, if it doesn't it may attempt to beep.",
          "",
          "Config file section : [ui]",
          "Config file value :   flash=true|false",
          NULL } } },

    { "/showsplash",
        _cmd_set_showsplash,
        { "/showsplash on|off", "Enable/disable splash logo on startup.",
        { "/showsplash on|off",
          "------------------",
          "Switch on or off the ascii logo on start up.",
          "",
          "Config file section : [ui]",
          "Config file value :   showsplash=true|false",
          NULL } } },
    
    { "/vercheck",
        _cmd_vercheck,
        { "/vercheck [on|off]", "Check for a new release.",
        { "/vercheck [on|off]",
          "------------------",
          "Without a parameter will check for a new release.",
          "Switching on or off will enable/disable a version check when Profanity starts,",
          "and each time the /about command is run.",
          NULL  } } },

    { "/chlog",
        _cmd_set_chlog,
        { "/chlog on|off", "Enable/disable chat logging.",
        { "/chlog on|off",
          "-------------",
          "Switch chat logging on or off.",
          "Chat logs are stored in the ~/.profanoty/log directory.",
          "A folder is created for each login that you have used with Profanity.",
          "Within in those folders, a log file is created for each user you chat to.",
          "",
          "For example if you are logged in as someuser@chatserv.com, and you chat",
          "to myfriend@chatserv.com, the following chat log will be created:",
          "",
          "    ~/.profanity/log/someuser_at_chatserv.com/myfriend_at_chatserv.com",
          NULL } } },

    { "/history",
        _cmd_set_history,
        { "/history on|off", "Enable/disable chat history.",
        { "/history on|off",
          "-------------",
          "Switch chat history on or off, requires chlog to be enabled.",
          "When history is enabled, previous messages are shown in chat windows.",
          "The last day of messages are shown, or if you have had profanity open",
          "for more than a day, messages will be shown from the day which",
          "you started profanity.",
          NULL } } }
};

static struct cmd_t status_commands[] =
{
    { "/away",
        _cmd_away,
        { "/away [msg]", "Set status to away.",
        { "/away [msg]",
          "-----------",
          "Set your status to \"away\" with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /away Gone for lunch",
          NULL } } },

    { "/chat",
        _cmd_chat,
        { "/chat [msg]", "Set status to chat (available for chat).",
        { "/chat [msg]",
          "-----------",
          "Set your status to \"chat\", meaning \"available for chat\",",
          "with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /chat Please talk to me!",
          NULL } } },

    { "/dnd",
        _cmd_dnd,
        { "/dnd [msg]", "Set status to dnd (do not disturb.",
        { "/dnd [msg]",
          "----------",
          "Set your status to \"dnd\", meaning \"do not disturb\",",
          "with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /dnd I'm in the zone",
          NULL } } },

    { "/online",
        _cmd_online,
        { "/online [msg]", "Set status to online.",
        { "/online [msg]",
          "-------------",
          "Set your status to \"online\" with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /online Up the Irons!",
          NULL } } },

    { "/xa",
        _cmd_xa,
        { "/xa [msg]", "Set status to xa (extended away).",
        { "/xa [msg]",
          "---------",
          "Set your status to \"xa\", meaning \"extended away\",",
          "with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /xa This meeting is going to be a long one",
          NULL } } },
};

static PAutocomplete commands_ac;
static PAutocomplete help_ac;

/*
 * Initialise command autocompleter and history
 */
void
cmd_init(void)
{
    log_info("Initialising commands");
    commands_ac = p_autocomplete_new();
    help_ac = p_autocomplete_new();
    p_autocomplete_add(help_ac, strdup("basic"));
    p_autocomplete_add(help_ac, strdup("status"));
    p_autocomplete_add(help_ac, strdup("settings"));
    p_autocomplete_add(help_ac, strdup("navigation"));

    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(main_commands); i++) {
        struct cmd_t *pcmd = main_commands+i;
        p_autocomplete_add(commands_ac, (gchar *)strdup(pcmd->cmd));
        p_autocomplete_add(help_ac, (gchar *)strdup(pcmd->cmd+1));
    }

    for (i = 0; i < ARRAY_SIZE(setting_commands); i++) {
        struct cmd_t *pcmd = setting_commands+i;
        p_autocomplete_add(commands_ac, (gchar *)strdup(pcmd->cmd));
        p_autocomplete_add(help_ac, (gchar *)strdup(pcmd->cmd+1));
    }

    for (i = 0; i < ARRAY_SIZE(status_commands); i++) {
        struct cmd_t *pcmd = status_commands+i;
        p_autocomplete_add(commands_ac, (gchar *)strdup(pcmd->cmd));
        p_autocomplete_add(help_ac, (gchar *)strdup(pcmd->cmd+1));
    }

    history_init();
}

void
cmd_close(void)
{
    p_autocomplete_clear(commands_ac);
    p_autocomplete_clear(help_ac);
}

// Command autocompletion functions

char *
cmd_complete(char *inp)
{
    return p_autocomplete_complete(commands_ac, inp);
}

void
cmd_reset_completer(void)
{
    p_autocomplete_reset(commands_ac);
}

// Command help
char *
cmd_help_complete(char *inp)
{
    return p_autocomplete_complete(help_ac, inp);
}

void
cmd_help_reset_completer(void)
{
    p_autocomplete_reset(help_ac);
}

GSList *
cmd_get_basic_help(void)
{
    GSList *result = NULL;

    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(main_commands); i++) {
        result = g_slist_append(result, &((main_commands+i)->help));
    }

    return result;
}

GSList *
cmd_get_settings_help(void)
{
    GSList *result = NULL;

    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(setting_commands); i++) {
        result = g_slist_append(result, &((setting_commands+i)->help));
    }

    return result;
}

GSList *
cmd_get_status_help(void)
{
    GSList *result = NULL;

    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(status_commands); i++) {
        result = g_slist_append(result, &((status_commands+i)->help));
    }

    return result;
}

// Command execution

gboolean
cmd_execute(const char * const command, const char * const inp)
{
    struct cmd_t *cmd = _cmd_get_command(command);

    if (cmd != NULL) {
        return (cmd->func(inp, cmd->help));
    } else {
        return cmd_execute_default(inp);
    }
}

gboolean
cmd_execute_default(const char * const inp)
{
    if (win_in_chat()) {
        char *recipient = win_get_recipient();
        jabber_send(inp, recipient);

        if (prefs_get_chlog()) {
            const char *jid = jabber_get_jid();
            chat_log_chat(jid, recipient, inp, OUT);
        }

        win_show_outgoing_msg("me", recipient, inp);
        free(recipient);
    } else {
        cons_bad_command(inp);
    }

    return TRUE;
}

// The command functions

static gboolean
_cmd_connect(const char * const inp, struct cmd_help_t help)
{
    gboolean result = FALSE;
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if ((conn_status != JABBER_DISCONNECTED) && (conn_status != JABBER_STARTED)) {
        cons_show("You are either connected already, or a login is in process.");
        result = TRUE;
    } else if (strlen(inp) < 10) {
        cons_show("Usage: %s", help.usage);
        result = TRUE;
    } else {
        char *user, *lower;
        user = strndup(inp+9, strlen(inp)-9);
        lower = g_utf8_strdown(user, -1);

        status_bar_get_password();
        status_bar_refresh();
        char passwd[21];
        inp_block();
        inp_get_password(passwd);
        inp_non_block();

        log_debug("Connecting as %s", lower);

        conn_status = jabber_connect(lower, passwd);
        if (conn_status == JABBER_CONNECTING) {
            cons_show("Connecting...");
            log_debug("Connecting...");
        }
        if (conn_status == JABBER_DISCONNECTED) {
            cons_bad_show("Connection to server failed.");
            log_debug("Connection using %s failed", lower);
        }

        result = TRUE;
    }

    return result;
}

static gboolean
_cmd_quit(const char * const inp, struct cmd_help_t help)
{
    log_info("Profanity is shutting down...");
    exit(0);
    return FALSE;
}

static gboolean
_cmd_help(const char * const inp, struct cmd_help_t help)
{
    if (strcmp(inp, "/help") == 0) {
        cons_help();
    } else if (strcmp(inp, "/help basic") == 0) {
        cons_basic_help();
    } else if (strcmp(inp, "/help status") == 0) {
        cons_status_help();
    } else if (strcmp(inp, "/help settings") == 0) {
        cons_settings_help();
    } else if (strcmp(inp, "/help navigation") == 0) {
        cons_navigation_help();
    } else {
        char *cmd = strndup(inp+6, strlen(inp)-6);
        char cmd_with_slash[1 + strlen(cmd) + 1];
        sprintf(cmd_with_slash, "/%s", cmd);

        const gchar **help_text = NULL;
        struct cmd_t *command = _cmd_get_command(cmd_with_slash);

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

static gboolean
_cmd_about(const char * const inp, struct cmd_help_t help)
{
    cons_show("");
    cons_about();
    return TRUE;
}

static gboolean
_cmd_prefs(const char * const inp, struct cmd_help_t help)
{
    cons_prefs();

    return TRUE;
}

static gboolean
_cmd_who(const char * const inp, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
    } else {
        // copy input
        char inp_cpy[strlen(inp) + 1];
        strcpy(inp_cpy, inp);

        // get show
        strtok(inp_cpy, " ");
        char *show = strtok(NULL, " ");

        // bad arg
        if ((show != NULL)
                && (strcmp(show, "online") != 0)
                && (strcmp(show, "offline") != 0)
                && (strcmp(show, "away") != 0)
                && (strcmp(show, "chat") != 0)
                && (strcmp(show, "xa") != 0)
                && (strcmp(show, "dnd") != 0)) {
            cons_show("Usage: %s", help.usage);

        // valid arg
        } else {
            GSList *list = get_contact_list();

            // no arg, show all contacts
            if (show == NULL) {
                cons_show("All contacts:");
                cons_show_contacts(list);

            // online, show all status that indicate online
            } else if (strcmp("online", show) == 0) {
                cons_show("Contacts (%s):", show);
                GSList *filtered = NULL;

                while (list != NULL) {
                    PContact contact = list->data;
                    const char * const contact_show = (p_contact_show(contact));
                    if ((strcmp(contact_show, "online") == 0)
                            || (strcmp(contact_show, "away") == 0)
                            || (strcmp(contact_show, "dnd") == 0)
                            || (strcmp(contact_show, "xa") == 0)
                            || (strcmp(contact_show, "chat") == 0)) {
                        filtered = g_slist_append(filtered, contact);
                    }
                    list = g_slist_next(list);
                }

                cons_show_contacts(filtered);

            // show specific status
            } else {
                cons_show("Contacts (%s):", show);
                GSList *filtered = NULL;

                while (list != NULL) {
                    PContact contact = list->data;
                    if (strcmp(p_contact_show(contact), show) == 0) {
                        filtered = g_slist_append(filtered, contact);
                    }
                    list = g_slist_next(list);
                }

                cons_show_contacts(filtered);
            }
        }
    }

    return TRUE;
}

static gboolean
_cmd_msg(const char * const inp, struct cmd_help_t help)
{
    char *usr = NULL;
    char *msg = NULL;

    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
    } else {
        // copy input
        char inp_cpy[strlen(inp) + 1];
        strcpy(inp_cpy, inp);

        // get user
        strtok(inp_cpy, " ");
        usr = strtok(NULL, " ");
        if ((usr != NULL) && (strlen(inp) > (5 + strlen(usr) + 1))) {
            // get message
            msg = strndup(inp+5+strlen(usr)+1, strlen(inp)-(5+strlen(usr)+1));

            if (msg != NULL) {
                jabber_send(msg, usr);
                win_show_outgoing_msg("me", usr, msg);

                if (prefs_get_chlog()) {
                    const char *jid = jabber_get_jid();
                    chat_log_chat(jid, usr, msg, OUT);
                }

            } else {
                cons_show("Usage: %s", help.usage);
            }
        } else {
            cons_show("Usage: %s", help.usage);
        }
    }

    return TRUE;
}

static gboolean
_cmd_tiny(const char * const inp, struct cmd_help_t help)
{
    if (strlen(inp) > 6) {
        char *url = strndup(inp+6, strlen(inp)-6);
        if (url == NULL) {
            log_error("Not enough memory.");
            return FALSE;
        }

        if (!tinyurl_valid(url)) {
            GString *error = g_string_new("/tiny, badly formed URL: ");
            g_string_append(error, url);
            cons_bad_show(error->str);
            if (win_in_chat()) {
                win_bad_show(error->str);
            }
            g_string_free(error, TRUE);
        } else if (win_in_chat()) {
            char *tiny = tinyurl_get(url);

            if (tiny != NULL) {
                char *recipient = win_get_recipient();
                jabber_send(tiny, recipient);

                if (prefs_get_chlog()) {
                    const char *jid = jabber_get_jid();
                    chat_log_chat(jid, recipient, tiny, OUT);
                }

                win_show_outgoing_msg("me", recipient, tiny);
                free(recipient);
                free(tiny);
            } else {
                cons_bad_show("Couldn't get tinyurl.");
            }
        } else {
            cons_bad_command(inp);
        }
        free(url);
    } else {
        cons_show("Usage: %s", help.usage);

        if (win_in_chat()) {
            char usage[strlen(help.usage + 8)];
            sprintf(usage, "Usage: %s", help.usage);
            win_show(usage);
        }
    }

    return TRUE;
}

static gboolean
_cmd_close(const char * const inp, struct cmd_help_t help)
{
    if (!win_close_win())
        cons_bad_command(inp);

    return TRUE;
}

static gboolean
_cmd_set_beep(const char * const inp, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(inp, help, "/beep",
        "Sound", prefs_set_beep);
}

static gboolean
_cmd_set_notify(const char * const inp, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(inp, help, "/notify",
        "Desktop notifications", prefs_set_notify);
}

static gboolean
_cmd_set_typing(const char * const inp, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(inp, help, "/typing",
        "Incoming typing notifications", prefs_set_typing);
}

static gboolean
_cmd_vercheck(const char * const inp, struct cmd_help_t help)
{
    if (strcmp(inp, "/vercheck") == 0) {
        cons_check_version(TRUE);
        return TRUE;
    } else {
        return _cmd_set_boolean_preference(inp, help, "/vercheck",
            "Version checking", prefs_set_vercheck);
    }
}

static gboolean
_cmd_set_flash(const char * const inp, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(inp, help, "/flash",
        "Screen flash", prefs_set_flash);
}

static gboolean
_cmd_set_showsplash(const char * const inp, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(inp, help, "/showsplash",
        "Splash screen", prefs_set_showsplash);
}

static gboolean
_cmd_set_chlog(const char * const inp, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(inp, help, "/chlog",
        "Chat logging", prefs_set_chlog);
}

static gboolean
_cmd_set_history(const char * const inp, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(inp, help, "/history",
        "Chat history", prefs_set_history);
}

static gboolean
_cmd_set_remind(const char * const inp, struct cmd_help_t help)
{
    if ((strncmp(inp, "/remind ", 8) != 0) || (strlen(inp) < 9)) {
        cons_show("Usage: %s", help.usage);
    } else {
        // copy input
        char inp_cpy[strlen(inp) + 1];
        strcpy(inp_cpy, inp);

        // get period
        strtok(inp_cpy, " ");
        char *period_str = strtok(NULL, " ");
        gint period = atoi(period_str);

        prefs_set_remind(period);
        if (period == 0) {
            cons_show("Message reminders disabled.");
        } else if (period == 1) {
            cons_show("Message reminder period set to 1 second.");
        } else {
            cons_show("Message reminder period set to %d seconds.", period);
        }
    }

    return TRUE;
}

static gboolean
_cmd_away(const char * const inp, struct cmd_help_t help)
{
    _update_presence(PRESENCE_AWAY, "away", inp);
    return TRUE;
}

static gboolean
_cmd_online(const char * const inp, struct cmd_help_t help)
{
    _update_presence(PRESENCE_ONLINE, "online", inp);
    return TRUE;
}

static gboolean
_cmd_dnd(const char * const inp, struct cmd_help_t help)
{
    _update_presence(PRESENCE_DND, "dnd", inp);
    return TRUE;
}

static gboolean
_cmd_chat(const char * const inp, struct cmd_help_t help)
{
    _update_presence(PRESENCE_CHAT, "chat", inp);
    return TRUE;
}

static gboolean
_cmd_xa(const char * const inp, struct cmd_help_t help)
{
    _update_presence(PRESENCE_XA, "xa", inp);
    return TRUE;
}

// helper function for status change commands

static void
_update_presence(const jabber_presence_t presence,
    const char * const show, const char * const inp)
{
    char *msg;
    if (strlen(inp) > strlen(show) + 2) {
        msg = strndup(inp+(strlen(show) + 2), strlen(inp)-(strlen(show) + 2));
    } else {
        msg = NULL;
    }

    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
    } else {
        jabber_update_presence(presence, msg);
        title_bar_set_status(presence);
        if (msg != NULL) {
            cons_show("Status set to %s, \"%s\"", show, msg);
            free(msg);
        } else {
            cons_show("Status set to %s", show);
        }
    }

}

// helper function for boolean preference commands

static gboolean
_cmd_set_boolean_preference(const char * const inp, struct cmd_help_t help,
    const char * const cmd_str, const char * const display,
    void (*set_func)(gboolean))
{
    GString *on = g_string_new(cmd_str);
    g_string_append(on, " on");

    GString *off = g_string_new(cmd_str);
    g_string_append(off, " off");

    GString *enabled = g_string_new(display);
    g_string_append(enabled, " enabled.");

    GString *disabled = g_string_new(display);
    g_string_append(disabled, " disabled.");

    if (strcmp(inp, on->str) == 0) {
        cons_show(enabled->str);
        set_func(TRUE);
    } else if (strcmp(inp, off->str) == 0) {
        cons_show(disabled->str);
        set_func(FALSE);
    } else {
        char usage[strlen(help.usage + 8)];
        sprintf(usage, "Usage: %s", help.usage);
        cons_show(usage);
    }

    g_string_free(on, TRUE);
    g_string_free(off, TRUE);
    g_string_free(enabled, TRUE);
    g_string_free(disabled, TRUE);

    return TRUE;
}

// helper to get command by string

static struct cmd_t *
_cmd_get_command(const char * const command)
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(main_commands); i++) {
        struct cmd_t *pcmd = main_commands+i;
        if (strcmp(pcmd->cmd, command) == 0) {
            return pcmd;
        }
    }

    for (i = 0; i < ARRAY_SIZE(setting_commands); i++) {
        struct cmd_t *pcmd = setting_commands+i;
        if (strcmp(pcmd->cmd, command) == 0) {
            return pcmd;
        }
    }

    for (i = 0; i < ARRAY_SIZE(status_commands); i++) {
        struct cmd_t *pcmd = status_commands+i;
        if (strcmp(pcmd->cmd, command) == 0) {
            return pcmd;
        }
    }

    return NULL;
}
