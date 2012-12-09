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
#include <limits.h>
#include <errno.h>

#include <glib.h>

#include "accounts.h"
#include "chat_session.h"
#include "command.h"
#include "common.h"
#include "contact.h"
#include "contact_list.h"
#include "chat_log.h"
#include "history.h"
#include "jabber.h"
#include "log.h"
#include "parser.h"
#include "preferences.h"
#include "prof_autocomplete.h"
#include "profanity.h"
#include "theme.h"
#include "tinyurl.h"
#include "ui.h"

typedef char*(*autocomplete_func)(char *);

/*
 * Command structure
 *
 * cmd - The command string including leading '/'
 * func - The function to execute for the command
 * parser - The function used to parse arguments
 * min_args - Minimum number of arguments
 * max_args - Maximum number of arguments
 * help - A help struct containing usage info etc
 */
struct cmd_t {
    const gchar *cmd;
    gboolean (*func)(gchar **args, struct cmd_help_t help);
    gchar** (*parser)(const char * const inp, int min, int max);
    int min_args;
    int max_args;
    struct cmd_help_t help;
};

static struct cmd_t * _cmd_get_command(const char * const command);
static void _update_presence(const jabber_presence_t presence,
    const char * const show, gchar **args);
static gboolean _cmd_set_boolean_preference(gchar *arg, struct cmd_help_t help,
    const char * const display,
    void (*set_func)(gboolean));

static void _cmd_complete_parameters(char *input, int *size);
static void _notify_autocomplete(char *input, int *size);
static void _titlebar_autocomplete(char *input, int *size);
static void _autoaway_autocomplete(char *input, int *size);
static void _parameter_autocomplete(char *input, int *size, char *command,
    autocomplete_func func);
static void _parameter_autocomplete_with_ac(char *input, int *size, char *command,
    PAutocomplete ac);

static int _strtoi(char *str, int *saveptr, int min, int max);
gchar** _cmd_parse_args(const char * const inp, int min, int max, int *num);

// command prototypes
static gboolean _cmd_quit(gchar **args, struct cmd_help_t help);
static gboolean _cmd_help(gchar **args, struct cmd_help_t help);
static gboolean _cmd_about(gchar **args, struct cmd_help_t help);
static gboolean _cmd_prefs(gchar **args, struct cmd_help_t help);
static gboolean _cmd_who(gchar **args, struct cmd_help_t help);
static gboolean _cmd_connect(gchar **args, struct cmd_help_t help);
static gboolean _cmd_disconnect(gchar **args, struct cmd_help_t help);
static gboolean _cmd_sub(gchar **args, struct cmd_help_t help);
static gboolean _cmd_msg(gchar **args, struct cmd_help_t help);
static gboolean _cmd_tiny(gchar **args, struct cmd_help_t help);
static gboolean _cmd_close(gchar **args, struct cmd_help_t help);
static gboolean _cmd_join(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_beep(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_notify(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_log(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_priority(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_reconnect(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_intype(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_flash(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_splash(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_chlog(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_history(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_states(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_outtype(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_autoping(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_titlebar(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_autoaway(gchar **args, struct cmd_help_t help);
static gboolean _cmd_vercheck(gchar **args, struct cmd_help_t help);
static gboolean _cmd_away(gchar **args, struct cmd_help_t help);
static gboolean _cmd_online(gchar **args, struct cmd_help_t help);
static gboolean _cmd_dnd(gchar **args, struct cmd_help_t help);
static gboolean _cmd_chat(gchar **args, struct cmd_help_t help);
static gboolean _cmd_xa(gchar **args, struct cmd_help_t help);
static gboolean _cmd_info(gchar **args, struct cmd_help_t help);
static gboolean _cmd_wins(gchar **args, struct cmd_help_t help);
static gboolean _cmd_nick(gchar **args, struct cmd_help_t help);
static gboolean _cmd_theme(gchar **args, struct cmd_help_t help);

/*
 * The commands are broken down into three groups:
 * Main commands
 * Commands to change preferences
 * Commands to change users status
 */
static struct cmd_t main_commands[] =
{
    { "/help",
        _cmd_help, parse_args, 0, 1,
        { "/help [list|area|command]", "Show help summary, or help on a specific area or command",
        { "/help [list|area|command]",
          "-------------------------",
          "Show help options.",
          "Specify list if you want a list of all commands.",
          "Specify an area (basic, presence, settings, navigation) for more help on that area.",
          "Specify the command if you want more detailed help on a specific command.",
          "",
          "Example : /help list",
          "Example : /help connect",
          "Example : /help settings",
          NULL } } },

    { "/about",
        _cmd_about, parse_args, 0, 0,
        { "/about", "About Profanity",
        { "/about",
          "------",
          "Show versioning and license information.",
          NULL  } } },

    { "/connect",
        _cmd_connect, parse_args, 1, 2,
        { "/connect user@domain [server]", "Login to jabber.",
        { "/connect user@domain [server]",
          "-----------------------------",
          "Connect to the jabber server at domain using the username user.",
          "Profanity should work with any XMPP (Jabber) compliant chat service.",
          "You can use tab completion to autocomplete any logins you have used before.",
          "Use the server option if the chat service is hosted at a different domain to the 'domain' part.",
          "",
          "Example: /connect myuser@gmail.com",
          "Example: /connect myuser@mycompany.com talk.google.com",
          NULL  } } },

    { "/disconnect",
        _cmd_disconnect, parse_args, 0, 0,
        { "/disconnect", "Logout of current jabber session.",
        { "/disconnect",
          "------------------",
          "Disconnect from the current jabber session.",
          "See the /connect command for connecting again.",
          NULL  } } },

    { "/prefs",
        _cmd_prefs, parse_args, 0, 1,
        { "/prefs [ui|desktop|chat|log|conn|presence]", "Show current preferences.",
        { "/prefs [ui|desktop|chat|log|conn|presence]",
          "------------------------------------------",
          "Show current preferences.",
          "The argument narrows down the category of preferences, with no argument showing all.",
          "The preferences are stored in:",
          "",
          "    $XDG_CONFIG_HOME/profanity/profrc",
          "",
          "If the environment variable XDG_CONFIG_HOME is not set the following default if used:",
          "",
          "    $HOME/.config/profanity/profrc",
          "",
          "Preference changes made using the various commands take effect immediately,",
          "you will need to restart Profanity for config file edits to take effect.",
          NULL } } },

    { "/theme",
        _cmd_theme, parse_args, 1, 1,
        { "/theme [theme-name]", "Change colour theme.",
        { "/theme [theme-name]",
          "--------------",
          "Change the colour setting as defined in:",
          "",
          "    $XDG_CONFIG_HOME/profanity/themes/theme-name",
          "",
          "If the environment variable XDG_CONFIG_HOME is not set the following default if used:",
          "",
          "    $HOME/.config/profanity/themes/theme-name",
          "",
          "Using \"default\" as the theme name will reset to the default colours.",
          NULL } } },

    { "/msg",
        _cmd_msg, parse_args_with_freetext, 1, 2,
        { "/msg user@host [message]", "Start chat window with user.",
        { "/msg user@host [message]",
          "------------------------",
          "Open a chat window with user@host and send the message if one is supplied.",
          "Use tab completion to autocomplete conacts from the roster.",
          "",
          "Example : /msg myfriend@server.com Hey, here's a message!",
          "Example : /msg otherfriend@server.com",
          NULL } } },

    { "/info",
        _cmd_info, parse_args, 1, 1,
        { "/info user@host", "Find out a contacts presence information.",
        { "/info user@host",
          "---------------",
          "Find out someones presence information.",
          "Use tab completion to autocomplete the contact.",
          NULL } } },

    { "/join",
        _cmd_join, parse_args_with_freetext, 1, 2,
        { "/join room@server [nick]", "Join a chat room.",
        { "/join room@server [nick]",
          "------------------------",
          "Join a chat room at the conference server.",
          "If nick is specified you will join with this nickname,",
          "otherwise the first part of your JID (before the @) will be used.",
          "If the room doesn't exist, and the server allows it, a new one will be created."
          "",
          "Example : /join jdev@conference.jabber.org",
          "Example : /join jdev@conference.jabber.org mynick",
          NULL } } },

    { "/nick",
        _cmd_nick, parse_args_with_freetext, 1, 1,
        { "/nick [nickname]", "Change nickname in chat room.",
        { "/nick [nickname]",
          "------------------------",
          "Change the name by which other member of a chat room see you.",
          "This command is only valid when called within a chat room window.",
          "The new nickname may contain spaces.",
          "",
          "Example : /nick kai hansen",
          "Example : /nick bob",
          NULL } } },

    { "/wins",
        _cmd_wins, parse_args, 0, 0,
        { "/wins", "List active windows.",
        { "/wins",
          "-----",
          "List all currently active windows and information about them.",
          NULL } } },

    { "/sub",
        _cmd_sub, parse_args, 1, 2,
        { "/sub <request|allow|deny|show|sent|received> [jid]", "Manage subscriptions.",
        { "/sub <request|allow|deny|show|sent|received> [jid]",
          "--------------------------------------------------",
          "request  : Send a subscription request to the user to be informed of their",
          "         : presence.",
          "allow    : Approve a contact's subscription reqeust to see your presence.",
          "deny     : Remove subscription for a contact, or deny a request",
          "show     : Show subscriprion status for a contact.",
          "sent     : Show all sent subscription requests pending a response.",
          "received : Show all received subscription requests awaiting your response.",
          "",
          "The optional 'jid' parameter only applys to 'request', 'allow', 'deny' and 'show'",
          "If it is omitted the contact of the current window is used.",
          "",
          "Example: /sub request myfriend@jabber.org",
          "Example: /sub allow myfriend@jabber.org",
          "Example: /sub request (whilst in chat with contact)",
          "Example: /sub sent",
          NULL  } } },

    { "/tiny",
        _cmd_tiny, parse_args, 1, 1,
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
        _cmd_who, parse_args, 0, 1,
        { "/who [status]", "Show contacts with chosen status.",
        { "/who [status]",
          "-------------",
          "Show contacts with the specified status, no status shows all contacts.",
          "Possible statuses are: online, offline, away, dnd, xa, chat, available, unavailable.",
          "",
          "\"/who online\" will list contacts that are connected, i.e. online, chat, away, xa, dnd",
          "\"/who available\" will list contacts that are available for chat, i.e. online, chat.",
          "\"/who unavailable\" will list contacts that are not available for chat, i.e. offline, away, xa, dnd.",
          "",
          "If in a chat room, this command shows the room roster in the room.",
          NULL } } },

    { "/close",
        _cmd_close, parse_args, 0, 0,
        { "/close", "Close current chat window.",
        { "/close",
          "------",
          "Close the current chat window, no message is sent to the recipient,",
          "The chat window will become available for new chats.",
          "If in a chat room, you will leave the room.",
          NULL } } },

    { "/quit",
        _cmd_quit, parse_args, 0, 0,
        { "/quit", "Quit Profanity.",
        { "/quit",
          "-----",
          "Logout of any current sessions, and quit Profanity.",
          NULL } } }
};

static struct cmd_t setting_commands[] =
{
    { "/beep",
        _cmd_set_beep, parse_args, 1, 1,
        { "/beep on|off", "Terminal beep on new messages.",
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
        _cmd_set_notify, parse_args, 2, 2,
        { "/notify type value", "Control various desktop noficiations.",
        { "/notify type value",
          "------------------",
          "Settings for various desktop notifications where type is one of:",
          "message : Notificaitons for messages.",
          "        : on|off",
          "remind  : Notification reminders of unread messages.",
          "        : where value is the reminder period in seconds,",
          "        : use 0 to disable.",
          "typing  : Notifications when contacts are typing.",
          "        : on|off",
          "",
          "Example : /notify message on (enable message notifications)",
          "Example : /notify remind 10  (remind every 10 seconds)",
          "Example : /notify remind 0   (switch off reminders)",
          "Example : /notify typing on  (enable typing notifications)",
          "",
          "Config file section : [notifications]",
          "Config file value :   message=on|off",
          "Config file value :   typing=on|off",
          "Config file value :   remind=seconds",
          NULL } } },

    { "/flash",
        _cmd_set_flash, parse_args, 1, 1,
        { "/flash on|off", "Terminal flash on new messages.",
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

    { "/intype",
        _cmd_set_intype, parse_args, 1, 1,
        { "/intype on|off", "Show when contact is typing.",
        { "/intype on|off",
          "--------------",
          "Show when a contact is typing in the console, and in active message window.",
          "",
          "Config file section : [ui]",
          "Config file value :   intype=true|false",
          NULL } } },

    { "/splash",
        _cmd_set_splash, parse_args, 1, 1,
        { "/splash on|off", "Splash logo on startup.",
        { "/splash on|off",
          "--------------",
          "Switch on or off the ascii logo on start up.",
          "",
          "Config file section : [ui]",
          "Config file value :   splash=true|false",
          NULL } } },

    { "/vercheck",
        _cmd_vercheck, parse_args, 0, 1,
        { "/vercheck [on|off]", "Check for a new release.",
        { "/vercheck [on|off]",
          "------------------",
          "Without a parameter will check for a new release.",
          "Switching on or off will enable/disable a version check when Profanity starts,",
          "and each time the /about command is run.",
          NULL  } } },

    { "/titlebar",
        _cmd_set_titlebar, parse_args, 2, 2,
        { "/titlebar property on|off", "Show various properties in the window title bar.",
        { "/titlebar property on|off",
          "-------------------------",
          "Possible properties are 'version'.",
          "",
          "Config file section : [ui]",
          "Config file value :   titlebar.version=true|false",
          NULL  } } },

    { "/chlog",
        _cmd_set_chlog, parse_args, 1, 1,
        { "/chlog on|off", "Chat logging to file",
        { "/chlog on|off",
          "-------------",
          "Switch chat logging on or off.",
          "",
          "Config file section : [logging]",
          "Config file value :   chlog=true|false",
          NULL } } },

    { "/states",
        _cmd_set_states, parse_args, 1, 1,
        { "/states on|off", "Send chat states during a chat session.",
        { "/states on|off",
          "--------------",
          "Sending of chat state notifications during chat sessions.",
          "Enabling this will send information about your activity during a chat",
          "session with somebody, such as whether you have become inactive, or",
          "have close the chat window.",
          "",
          "Config file section : [chatstates]",
          "Config file value :   enabled=true|false",
          NULL } } },

    { "/outtype",
        _cmd_set_outtype, parse_args, 1, 1,
        { "/outtype on|off", "Send typing notification to recipient.",
        { "/outtype on|off",
          "--------------",
          "Send an indication that you are typing to the other person in chat.",
          "Chat states must be enabled for this to work, see the /states command.",
          "",
          "Config file section : [chatstates]",
          "Config file value :   outtype=true|false",
          NULL } } },

    { "/history",
        _cmd_set_history, parse_args, 1, 1,
        { "/history on|off", "Chat history in message windows.",
        { "/history on|off",
          "---------------",
          "Switch chat history on or off, requires chlog to be enabled.",
          "When history is enabled, previous messages are shown in chat windows.",
          "The last day of messages are shown, or if you have had profanity open",
          "for more than a day, messages will be shown from the day which",
          "you started profanity.",
          "",
          "Config file section : [ui]",
          "Config file value :   history=true|false",
          NULL } } },

    { "/log",
        _cmd_set_log, parse_args, 2, 2,
        { "/log maxsize <value>", "Manage system logging settings.",
        { "/log maxsize <value>",
          "--------------------",
          "maxsize : When log file size exceeds this value it will be automatically",
          "          rotated (file will be renamed). Default value is 1048580 (1MB)",
          "",
          "Config file section : [logging]",
          "Config file value :   maxsize=bytes",
          NULL } } },

    { "/reconnect",
        _cmd_set_reconnect, parse_args, 1, 1,
        { "/reconnect seconds", "Set reconnect interval.",
        { "/reconnect seconds",
          "--------------------",
          "Set the reconnect attempt interval in seconds for when the connection is lost.",
          "A value of 0 will switch of reconnect attempts.",
          "",
          "Config file section : [connection]",
          "Config file value :   reconnect=seconds",
          NULL } } },

    { "/autoping",
        _cmd_set_autoping, parse_args, 1, 1,
        { "/autoping seconds", "Server ping interval.",
        { "/autoping seconds",
          "--------------------",
          "Set the number of seconds between server pings, so ensure connection kept alive.",
          "A value of 0 will switch off autopinging the server.",
          "",
          "Config file section : [connection]",
          "Config file value :   autoping=seconds",
          NULL } } },

    { "/autoaway",
        _cmd_set_autoaway, parse_args_with_freetext, 2, 2,
        { "/autoaway setting value", "Set auto idle/away properties.",
        { "/autoaway setting value",
          "-----------------------",
          "'setting' may be one of 'mode', 'minutes', 'message' or 'check', with the following values:",
          "",
          "mode    : idle - Sends idle time, whilst your status remains online.",
          "          away - Sends an away presence.",
          "          off - Disabled (default).",
          "time    : Number of minutes before the presence change is sent, the default is 15.",
          "message : Optional message to send with the presence change.",
          "        : off - Disable message (default).",
          "check   : on|off, when enabled, checks for activity and sends online presence, default is 'on'.",
          "",
          "Example: /autoaway mode idle",
          "Example: /autoaway time 30",
          "Example: /autoaway message I'm not really doing much",
          "Example: /autoaway check false",
          "",
          "Config file section : [presence]",
          "Config file value :   autoaway.mode=idle|away|off",
          "Config file value :   autoaway.time=value",
          "Config file value :   autoaway.message=value",
          "Config file value :   autoaway.check=on|off",
          NULL } } },

    { "/priority",
        _cmd_set_priority, parse_args, 1, 1,
        { "/priority <value>", "Set priority for connection.",
        { "/priority <value>",
          "--------------------",
          "value : Number between -128 and 127. Default value is 0.",
          "",
          "Config file section : [presence]",
          "Config file value :   priority=value",
          NULL } } }
};

static struct cmd_t presence_commands[] =
{
    { "/away",
        _cmd_away, parse_args_with_freetext, 0, 1,
        { "/away [msg]", "Set status to away.",
        { "/away [msg]",
          "-----------",
          "Set your status to \"away\" with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /away Gone for lunch",
          NULL } } },

    { "/chat",
        _cmd_chat, parse_args_with_freetext, 0, 1,
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
        _cmd_dnd, parse_args_with_freetext, 0, 1,
        { "/dnd [msg]", "Set status to dnd (do not disturb).",
        { "/dnd [msg]",
          "----------",
          "Set your status to \"dnd\", meaning \"do not disturb\",",
          "with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /dnd I'm in the zone",
          NULL } } },

    { "/online",
        _cmd_online, parse_args_with_freetext, 0, 1,
        { "/online [msg]", "Set status to online.",
        { "/online [msg]",
          "-------------",
          "Set your status to \"online\" with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /online Up the Irons!",
          NULL } } },

    { "/xa",
        _cmd_xa, parse_args_with_freetext, 0, 1,
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
static PAutocomplete who_ac;
static PAutocomplete help_ac;
static PAutocomplete notify_ac;
static PAutocomplete prefs_ac;
static PAutocomplete sub_ac;
static PAutocomplete log_ac;
static PAutocomplete autoaway_ac;
static PAutocomplete autoaway_mode_ac;
static PAutocomplete titlebar_ac;

/*
 * Initialise command autocompleter and history
 */
void
cmd_init(void)
{
    log_info("Initialising commands");

    commands_ac = p_autocomplete_new();
    who_ac = p_autocomplete_new();

    prefs_ac = p_autocomplete_new();
    p_autocomplete_add(prefs_ac, strdup("ui"));
    p_autocomplete_add(prefs_ac, strdup("desktop"));
    p_autocomplete_add(prefs_ac, strdup("chat"));
    p_autocomplete_add(prefs_ac, strdup("log"));
    p_autocomplete_add(prefs_ac, strdup("conn"));
    p_autocomplete_add(prefs_ac, strdup("presence"));

    help_ac = p_autocomplete_new();
    p_autocomplete_add(help_ac, strdup("list"));
    p_autocomplete_add(help_ac, strdup("basic"));
    p_autocomplete_add(help_ac, strdup("presence"));
    p_autocomplete_add(help_ac, strdup("settings"));
    p_autocomplete_add(help_ac, strdup("navigation"));

    notify_ac = p_autocomplete_new();
    p_autocomplete_add(notify_ac, strdup("message"));
    p_autocomplete_add(notify_ac, strdup("typing"));
    p_autocomplete_add(notify_ac, strdup("remind"));

    sub_ac = p_autocomplete_new();
    p_autocomplete_add(sub_ac, strdup("request"));
    p_autocomplete_add(sub_ac, strdup("allow"));
    p_autocomplete_add(sub_ac, strdup("deny"));
    p_autocomplete_add(sub_ac, strdup("show"));
    p_autocomplete_add(sub_ac, strdup("sent"));
    p_autocomplete_add(sub_ac, strdup("received"));

    titlebar_ac = p_autocomplete_new();
    p_autocomplete_add(titlebar_ac, strdup("version"));

    log_ac = p_autocomplete_new();
    p_autocomplete_add(log_ac, strdup("maxsize"));

    autoaway_ac = p_autocomplete_new();
    p_autocomplete_add(autoaway_ac, strdup("mode"));
    p_autocomplete_add(autoaway_ac, strdup("time"));
    p_autocomplete_add(autoaway_ac, strdup("message"));
    p_autocomplete_add(autoaway_ac, strdup("check"));

    autoaway_mode_ac = p_autocomplete_new();
    p_autocomplete_add(autoaway_mode_ac, strdup("away"));
    p_autocomplete_add(autoaway_mode_ac, strdup("idle"));
    p_autocomplete_add(autoaway_mode_ac, strdup("off"));

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

    for (i = 0; i < ARRAY_SIZE(presence_commands); i++) {
        struct cmd_t *pcmd = presence_commands+i;
        p_autocomplete_add(commands_ac, (gchar *)strdup(pcmd->cmd));
        p_autocomplete_add(help_ac, (gchar *)strdup(pcmd->cmd+1));
        p_autocomplete_add(who_ac, (gchar *)strdup(pcmd->cmd+1));
    }

    p_autocomplete_add(who_ac, strdup("offline"));
    p_autocomplete_add(who_ac, strdup("available"));
    p_autocomplete_add(who_ac, strdup("unavailable"));

    history_init();
}

void
cmd_close(void)
{
    p_autocomplete_clear(commands_ac);
    p_autocomplete_clear(who_ac);
    p_autocomplete_clear(help_ac);
    p_autocomplete_clear(notify_ac);
    p_autocomplete_clear(sub_ac);
    p_autocomplete_clear(log_ac);
    p_autocomplete_clear(prefs_ac);
    p_autocomplete_clear(autoaway_ac);
    p_autocomplete_clear(autoaway_mode_ac);
}

// Command autocompletion functions
void
cmd_autocomplete(char *input, int *size)
{
    int i = 0;
    char *found = NULL;
    char *auto_msg = NULL;
    char inp_cpy[*size];

    // autocomplete command
    if ((strncmp(input, "/", 1) == 0) && (!str_contains(input, *size, ' '))) {
        for(i = 0; i < *size; i++) {
            inp_cpy[i] = input[i];
        }
        inp_cpy[i] = '\0';
        found = p_autocomplete_complete(commands_ac, inp_cpy);
        if (found != NULL) {
            auto_msg = (char *) malloc((strlen(found) + 1) * sizeof(char));
            strcpy(auto_msg, found);
            inp_replace_input(input, auto_msg, size);
            free(auto_msg);
            free(found);
        }

    // autocomplete parameters
    } else {
        _cmd_complete_parameters(input, size);
    }
}

void
cmd_reset_autocomplete()
{
    contact_list_reset_search_attempts();
    accounts_reset_login_search();
    prefs_reset_boolean_choice();
    p_autocomplete_reset(help_ac);
    p_autocomplete_reset(notify_ac);
    p_autocomplete_reset(sub_ac);
    p_autocomplete_reset(who_ac);
    p_autocomplete_reset(prefs_ac);
    p_autocomplete_reset(log_ac);
    p_autocomplete_reset(commands_ac);
    p_autocomplete_reset(autoaway_ac);
    p_autocomplete_reset(autoaway_mode_ac);
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
cmd_get_presence_help(void)
{
    GSList *result = NULL;

    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(presence_commands); i++) {
        result = g_slist_append(result, &((presence_commands+i)->help));
    }

    return result;
}

// Command execution

gboolean
cmd_execute(const char * const command, const char * const inp)
{
    struct cmd_t *cmd = _cmd_get_command(command);

    if (cmd != NULL) {
        gchar **args = cmd->parser(inp, cmd->min_args, cmd->max_args);
        if (args == NULL) {
            cons_show("Usage: %s", cmd->help.usage);
            if (win_current_is_chat()) {
                char usage[strlen(cmd->help.usage) + 8];
                sprintf(usage, "Usage: %s", cmd->help.usage);
                win_current_show(usage);
            }
            return TRUE;
        } else {
            gboolean result = cmd->func(args, cmd->help);
            g_strfreev(args);
            return result;
        }
    } else {
        return cmd_execute_default(inp);
    }
}

gboolean
cmd_execute_default(const char * const inp)
{
    if (win_current_is_groupchat()) {
        jabber_conn_status_t status = jabber_get_connection_status();
        if (status != JABBER_CONNECTED) {
            win_current_show("You are not currently connected.");
        } else {
            char *recipient = win_current_get_recipient();
            jabber_send_groupchat(inp, recipient);
            free(recipient);
        }
    } else if (win_current_is_chat() || win_current_is_private()) {
        jabber_conn_status_t status = jabber_get_connection_status();
        if (status != JABBER_CONNECTED) {
            win_current_show("You are not currently connected.");
        } else {
            char *recipient = win_current_get_recipient();
            jabber_send(inp, recipient);

            if (prefs_get_chlog()) {
                const char *jid = jabber_get_jid();
                chat_log_chat(jid, recipient, inp, OUT, NULL);
            }

            win_show_outgoing_msg("me", recipient, inp);
            free(recipient);
        }
    } else {
        cons_bad_command(inp);
    }

    return TRUE;
}

static void
_cmd_complete_parameters(char *input, int *size)
{
    _parameter_autocomplete(input, size, "/beep",
        prefs_autocomplete_boolean_choice);
    _parameter_autocomplete(input, size, "/intype",
        prefs_autocomplete_boolean_choice);
    _parameter_autocomplete(input, size, "/states",
        prefs_autocomplete_boolean_choice);
    _parameter_autocomplete(input, size, "/outtype",
        prefs_autocomplete_boolean_choice);
    _parameter_autocomplete(input, size, "/flash",
        prefs_autocomplete_boolean_choice);
    _parameter_autocomplete(input, size, "/splash",
        prefs_autocomplete_boolean_choice);
    _parameter_autocomplete(input, size, "/chlog",
        prefs_autocomplete_boolean_choice);
    _parameter_autocomplete(input, size, "/history",
        prefs_autocomplete_boolean_choice);
    _parameter_autocomplete(input, size, "/vercheck",
        prefs_autocomplete_boolean_choice);

    _parameter_autocomplete(input, size, "/msg",
        contact_list_find_contact);
    _parameter_autocomplete(input, size, "/info",
        contact_list_find_contact);
    _parameter_autocomplete(input, size, "/connect",
        accounts_find_login);
    _parameter_autocomplete_with_ac(input, size, "/sub", sub_ac);
    _parameter_autocomplete_with_ac(input, size, "/help", help_ac);
    _parameter_autocomplete_with_ac(input, size, "/who", who_ac);
    _parameter_autocomplete_with_ac(input, size, "/prefs", prefs_ac);
    _parameter_autocomplete_with_ac(input, size, "/log", log_ac);

    _notify_autocomplete(input, size);
    _autoaway_autocomplete(input, size);
    _titlebar_autocomplete(input, size);
}

// The command functions

static gboolean
_cmd_connect(gchar **args, struct cmd_help_t help)
{
    gboolean result = FALSE;

    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if ((conn_status != JABBER_DISCONNECTED) && (conn_status != JABBER_STARTED)) {
        cons_show("You are either connected already, or a login is in process.");
        result = TRUE;
    } else {
        char *user = args[0];
        char *altdomain = args[1];
        char *lower = g_utf8_strdown(user, -1);

        status_bar_get_password();
        status_bar_refresh();
        char passwd[21];
        inp_block();
        inp_get_password(passwd);
        inp_non_block();

        log_debug("Connecting as %s", lower);

        conn_status = jabber_connect(lower, passwd, altdomain);
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
_cmd_sub(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are currently not connected.");
        return TRUE;
    }

    char *subcmd, *jid, *bare_jid;
    subcmd = args[0];
    jid = args[1];

    if (subcmd == NULL) {
        cons_show("Usage: %s", help.usage);
        return TRUE;
    }

    if (strcmp(subcmd, "sent") == 0) {
        if (contact_list_has_pending_subscriptions()) {
            cons_show("Awaiting subscription responses from:");
            GSList *contacts = get_contact_list();
            while (contacts != NULL) {
                PContact contact = (PContact) contacts->data;
                if (p_contact_pending_out(contact)) {
                    cons_show(p_contact_jid(contact));
                }
                contacts = g_slist_next(contacts);
            }
        } else {
            cons_show("No pending requests sent.");
        }

        return TRUE;
    }

    if (strcmp(subcmd, "received") == 0) {
        GList *received = jabber_get_subscription_requests();

        if (received == NULL) {
            cons_show("No outstanding subscription requests.");
        } else {
            cons_show("Outstanding subscription requests from:",
                g_list_length(received));
            while (received != NULL) {
                cons_show(received->data);
                received = g_list_next(received);
            }
        }

        return TRUE;
    }

    if (!win_current_is_chat() && (jid == NULL)) {
        cons_show("You must specify a contact.");
        return TRUE;
    }

    if (jid != NULL) {
        jid = strdup(jid);
    } else {
        jid = win_current_get_recipient();
    }

    bare_jid = strtok(jid, "/");

    if (strcmp(subcmd, "allow") == 0) {
        jabber_subscription(bare_jid, PRESENCE_SUBSCRIBED);
        cons_show("Accepted subscription for %s", bare_jid);
        log_info("Accepted subscription for %s", bare_jid);
    } else if (strcmp(subcmd, "deny") == 0) {
        jabber_subscription(bare_jid, PRESENCE_UNSUBSCRIBED);
        cons_show("Deleted/denied subscription for %s", bare_jid);
        log_info("Deleted/denied subscription for %s", bare_jid);
    } else if (strcmp(subcmd, "request") == 0) {
        jabber_subscription(bare_jid, PRESENCE_SUBSCRIBE);
        cons_show("Sent subscription request to %s.", bare_jid);
        log_info("Sent subscription request to %s.", bare_jid);
    } else if (strcmp(subcmd, "show") == 0) {
        PContact contact = contact_list_get_contact(bare_jid);
        if ((contact == NULL) || (p_contact_subscription(contact) == NULL)) {
            if (win_current_is_chat()) {
                win_current_show("No subscription information for %s.", bare_jid);
            } else {
                cons_show("No subscription information for %s.", bare_jid);
            }
        } else {
            if (win_current_is_chat()) {
                if (p_contact_pending_out(contact)) {
                    win_current_show("%s subscription status: %s, request pending.",
                        bare_jid, p_contact_subscription(contact));
                } else {
                    win_current_show("%s subscription status: %s.", bare_jid,
                        p_contact_subscription(contact));
                }
            } else {
                if (p_contact_pending_out(contact)) {
                    cons_show("%s subscription status: %s, request pending.",
                        bare_jid, p_contact_subscription(contact));
                } else {
                    cons_show("%s subscription status: %s.", bare_jid,
                        p_contact_subscription(contact));
                }
            }
        }
    } else {
        cons_show("Usage: %s", help.usage);
    }

    free(jid);
    return TRUE;
}

static gboolean
_cmd_disconnect(gchar **args, struct cmd_help_t help)
{
    if (jabber_get_connection_status() == JABBER_CONNECTED) {
        char *jid = strdup(jabber_get_jid());
        prof_handle_disconnect(jid);
        free(jid);
    } else {
        cons_show("You are not currently connected.");
    }

    return TRUE;
}

static gboolean
_cmd_quit(gchar **args, struct cmd_help_t help)
{
    log_info("Profanity is shutting down...");
    exit(0);
    return FALSE;
}

static gboolean
_cmd_wins(gchar **args, struct cmd_help_t help)
{
    cons_show_wins();
    return TRUE;
}

static gboolean
_cmd_help(gchar **args, struct cmd_help_t help)
{
    int num_args = g_strv_length(args);
    if (num_args == 0) {
        cons_help();
    } else if (strcmp(args[0], "list") == 0) {
        cons_show("");
        cons_show("Basic commands:");
        cons_show_time();
        unsigned int i;
        for (i = 0; i < ARRAY_SIZE(main_commands); i++) {
            cons_show_word( (main_commands+i)->cmd );
            if (i < ARRAY_SIZE(main_commands) - 1) {
                cons_show_word(", ");
            }
        }
        cons_show_word("\n");
        cons_show("Settings commands:");
        cons_show_time();
        for (i = 0; i < ARRAY_SIZE(setting_commands); i++) {
            cons_show_word( (setting_commands+i)->cmd );
            if (i < ARRAY_SIZE(setting_commands) - 1) {
                cons_show_word(", ");
            }
        }
        cons_show_word("\n");
        cons_show("Presence commands:");
        cons_show_time();
        for (i = 0; i < ARRAY_SIZE(presence_commands); i++) {
            cons_show_word( (presence_commands+i)->cmd );
            if (i < ARRAY_SIZE(presence_commands) - 1) {
                cons_show_word(", ");
            }
        }
        cons_show_word("\n");
    } else if (strcmp(args[0], "basic") == 0) {
        cons_basic_help();
    } else if (strcmp(args[0], "presence") == 0) {
        cons_presence_help();
    } else if (strcmp(args[0], "settings") == 0) {
        cons_settings_help();
    } else if (strcmp(args[0], "navigation") == 0) {
        cons_navigation_help();
    } else {
        char *cmd = args[0];
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
_cmd_about(gchar **args, struct cmd_help_t help)
{
    cons_show("");
    cons_about();
    return TRUE;
}

static gboolean
_cmd_prefs(gchar **args, struct cmd_help_t help)
{
    if (args[0] == NULL) {
        cons_prefs();
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

static gboolean
_cmd_theme(gchar **args, struct cmd_help_t help)
{
    // list themes
    if (strcmp(args[0], "list") == 0) {
        GSList *themes = theme_list();
        cons_show_themes(themes);
        g_slist_free_full(themes, g_free);

    // load a theme
    } else if (theme_load(args[0])) {
        ui_load_colours();
        prefs_set_theme(args[0]);
        cons_show("Loaded theme: %s", args[0]);

    // theme not found
    } else {
        cons_show("Couldn't find theme: %s", args[0]);
    }

    return TRUE;
}

static gboolean
_cmd_who(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
    } else {
        char *presence = args[0];

        // bad arg
        if ((presence != NULL)
                && (strcmp(presence, "online") != 0)
                && (strcmp(presence, "available") != 0)
                && (strcmp(presence, "unavailable") != 0)
                && (strcmp(presence, "offline") != 0)
                && (strcmp(presence, "away") != 0)
                && (strcmp(presence, "chat") != 0)
                && (strcmp(presence, "xa") != 0)
                && (strcmp(presence, "dnd") != 0)) {
            cons_show("Usage: %s", help.usage);

        // valid arg
        } else {
            if (win_current_is_groupchat()) {
                char *room = win_current_get_recipient();
                win_show_room_roster(room);
            } else {
                GSList *list = get_contact_list();

                // no arg, show all contacts
                if (presence == NULL) {
                    cons_show("All contacts:");
                    cons_show_contacts(list);

                // available
                } else if (strcmp("available", presence) == 0) {
                    cons_show("Contacts (%s):", presence);
                    GSList *filtered = NULL;

                    while (list != NULL) {
                        PContact contact = list->data;
                        const char * const contact_presence = (p_contact_presence(contact));
                        if ((strcmp(contact_presence, "online") == 0)
                                || (strcmp(contact_presence, "chat") == 0)) {
                            filtered = g_slist_append(filtered, contact);
                        }
                        list = g_slist_next(list);
                    }

                    cons_show_contacts(filtered);

                // unavailable
                } else if (strcmp("unavailable", presence) == 0) {
                    cons_show("Contacts (%s):", presence);
                    GSList *filtered = NULL;

                    while (list != NULL) {
                        PContact contact = list->data;
                        const char * const contact_presence = (p_contact_presence(contact));
                        if ((strcmp(contact_presence, "offline") == 0)
                                || (strcmp(contact_presence, "away") == 0)
                                || (strcmp(contact_presence, "dnd") == 0)
                                || (strcmp(contact_presence, "xa") == 0)) {
                            filtered = g_slist_append(filtered, contact);
                        }
                        list = g_slist_next(list);
                    }

                    cons_show_contacts(filtered);

                // online, show all status that indicate online
                } else if (strcmp("online", presence) == 0) {
                    cons_show("Contacts (%s):", presence);
                    GSList *filtered = NULL;

                    while (list != NULL) {
                        PContact contact = list->data;
                        const char * const contact_presence = (p_contact_presence(contact));
                        if ((strcmp(contact_presence, "online") == 0)
                                || (strcmp(contact_presence, "away") == 0)
                                || (strcmp(contact_presence, "dnd") == 0)
                                || (strcmp(contact_presence, "xa") == 0)
                                || (strcmp(contact_presence, "chat") == 0)) {
                            filtered = g_slist_append(filtered, contact);
                        }
                        list = g_slist_next(list);
                    }

                    cons_show_contacts(filtered);

                // show specific status
                } else {
                    cons_show("Contacts (%s):", presence);
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

    return TRUE;
}

static gboolean
_cmd_msg(gchar **args, struct cmd_help_t help)
{
    char *usr = args[0];
    char *msg = args[1];

    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (ui_windows_full()) {
        cons_bad_show("Windows all used, close a window and try again.");
        return TRUE;
    }

    if (msg != NULL) {
        jabber_send(msg, usr);
        win_show_outgoing_msg("me", usr, msg);

        if (prefs_get_chlog()) {
            const char *jid = jabber_get_jid();
            chat_log_chat(jid, usr, msg, OUT, NULL);
        }

        return TRUE;
    } else {
        win_new_chat_win(usr);
        return TRUE;
    }
}

static gboolean
_cmd_info(gchar **args, struct cmd_help_t help)
{
    char *usr = args[0];

    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
    } else {
        cons_show_status(usr);
    }

    return TRUE;
}

static gboolean
_cmd_join(gchar **args, struct cmd_help_t help)
{
    char *room = args[0];
    char *nick = NULL;

    int num_args = g_strv_length(args);
    if (num_args == 2) {
        nick = args[1];
    }

    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
    } else if (ui_windows_full()) {
        cons_bad_show("Windows all used, close a window and try again.");
    } else {
        // if no nick, set to first part of jid
        if (nick == NULL) {
            const char *jid = jabber_get_jid();
            char jid_cpy[strlen(jid) + 1];
            strcpy(jid_cpy, jid);
            nick = strdup(strtok(jid_cpy, "@"));
        }
        jabber_join(room, nick);
        win_join_chat(room, nick);
    }

    return TRUE;
}

static gboolean
_cmd_nick(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }
    if (!win_current_is_groupchat()) {
        cons_show("You can only change your nickname in a chat room window.");
        return TRUE;
    }

    char *room = win_current_get_recipient();
    char *nick = args[0];
    jabber_change_room_nick(room, nick);

    return TRUE;
}

static gboolean
_cmd_tiny(gchar **args, struct cmd_help_t help)
{
    char *url = args[0];

    if (!tinyurl_valid(url)) {
        GString *error = g_string_new("/tiny, badly formed URL: ");
        g_string_append(error, url);
        cons_bad_show(error->str);
        if (win_current_is_chat()) {
            win_current_bad_show(error->str);
        }
        g_string_free(error, TRUE);
    } else if (win_current_is_chat()) {
        char *tiny = tinyurl_get(url);

        if (tiny != NULL) {
            char *recipient = win_current_get_recipient();
            jabber_send(tiny, recipient);

            if (prefs_get_chlog()) {
                const char *jid = jabber_get_jid();
                chat_log_chat(jid, recipient, tiny, OUT, NULL);
            }

            win_show_outgoing_msg("me", recipient, tiny);
            free(recipient);
            free(tiny);
        } else {
            cons_bad_show("Couldn't get tinyurl.");
        }
    } else {
        cons_show("/tiny can only be used in chat windows");
    }

    return TRUE;
}

static gboolean
_cmd_close(gchar **args, struct cmd_help_t help)
{
    if (win_current_is_groupchat()) {
        char *room_jid = win_current_get_recipient();
        jabber_leave_chat_room(room_jid);
        win_current_close();
    } else if (win_current_is_chat() || win_current_is_private()) {

        if (prefs_get_states()) {
            char *recipient = win_current_get_recipient();

            // send <gone/> chat state before closing
            if (chat_session_get_recipient_supports(recipient)) {
                chat_session_set_gone(recipient);
                jabber_send_gone(recipient);
                chat_session_end(recipient);
            }
        }

        win_current_close();

    } else {
        cons_show("Cannot close console window.");
    }

    return TRUE;
}

static gboolean
_cmd_set_beep(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help, "Sound", prefs_set_beep);
}

static gboolean
_cmd_set_states(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help, "Sending chat states",
        prefs_set_states);
}

static gboolean
_cmd_set_titlebar(gchar **args, struct cmd_help_t help)
{
    if (strcmp(args[0], "version") != 0) {
        cons_show("Usage: %s", help.usage);
        return TRUE;
    } else {
        return _cmd_set_boolean_preference(args[1], help,
        "Show version in window title", prefs_set_titlebarversion);
    }
}

static gboolean
_cmd_set_outtype(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Sending typing notifications", prefs_set_outtype);
}

static gboolean
_cmd_set_notify(gchar **args, struct cmd_help_t help)
{
    char *kind = args[0];
    char *value = args[1];

    // bad kind
    if ((strcmp(kind, "message") != 0) && (strcmp(kind, "typing") != 0) &&
            (strcmp(kind, "remind") != 0)) {
        cons_show("Usage: %s", help.usage);

    // set message setting
    } else if (strcmp(kind, "message") == 0) {
        if (strcmp(value, "on") == 0) {
            cons_show("Message notifications enabled.");
            prefs_set_notify_message(TRUE);
        } else if (strcmp(value, "off") == 0) {
            cons_show("Message notifications disabled.");
            prefs_set_notify_message(FALSE);
        } else {
            cons_show("Usage: /notify message on|off");
        }

    // set typing setting
    } else if (strcmp(kind, "typing") == 0) {
        if (strcmp(value, "on") == 0) {
            cons_show("Typing notifications enabled.");
            prefs_set_notify_typing(TRUE);
        } else if (strcmp(value, "off") == 0) {
            cons_show("Typing notifications disabled.");
            prefs_set_notify_typing(FALSE);
        } else {
            cons_show("Usage: /notify typing on|off");
        }

    } else { // remind
        gint period = atoi(value);
        prefs_set_notify_remind(period);
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
_cmd_set_log(gchar **args, struct cmd_help_t help)
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

static gboolean
_cmd_set_reconnect(gchar **args, struct cmd_help_t help)
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

static gboolean
_cmd_set_autoping(gchar **args, struct cmd_help_t help)
{
    char *value = args[0];
    int intval;

    if (_strtoi(value, &intval, 0, INT_MAX) == 0) {
        prefs_set_autoping(intval);
        jabber_set_autoping(intval);
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

static gboolean
_cmd_set_autoaway(gchar **args, struct cmd_help_t help)
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
            prefs_set_autoaway_mode(value);
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
            prefs_set_autoaway_message(NULL);
            cons_show("Auto away message cleared.");
        } else {
            prefs_set_autoaway_message(value);
            cons_show("Auto away message set to: \"%s\".", value);
        }

        return TRUE;
    }

    if (strcmp(setting, "check") == 0) {
        return _cmd_set_boolean_preference(value, help, "Online check",
            prefs_set_autoaway_check);
    }

    return TRUE;
}

static gboolean
_cmd_set_priority(gchar **args, struct cmd_help_t help)
{
    char *value = args[0];
    int intval;

    if (_strtoi(value, &intval, -128, 127) == 0) {
        char *status = jabber_get_status();
        prefs_set_priority((int)intval);
        // update presence with new priority
        jabber_update_presence(jabber_get_presence(), status, 0);
        if (status != NULL)
            free(status);
        cons_show("Priority set to %d.", intval);
    }

    return TRUE;
}

static gboolean
_cmd_vercheck(gchar **args, struct cmd_help_t help)
{
    int num_args = g_strv_length(args);

    if (num_args == 0) {
        cons_check_version(TRUE);
        return TRUE;
    } else {
        return _cmd_set_boolean_preference(args[0], help,
            "Version checking", prefs_set_vercheck);
    }
}

static gboolean
_cmd_set_flash(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Screen flash", prefs_set_flash);
}

static gboolean
_cmd_set_intype(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Show contact typing", prefs_set_intype);
}

static gboolean
_cmd_set_splash(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Splash screen", prefs_set_splash);
}

static gboolean
_cmd_set_chlog(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Chat logging", prefs_set_chlog);
}

static gboolean
_cmd_set_history(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Chat history", prefs_set_history);
}

static gboolean
_cmd_away(gchar **args, struct cmd_help_t help)
{
    _update_presence(PRESENCE_AWAY, "away", args);
    return TRUE;
}

static gboolean
_cmd_online(gchar **args, struct cmd_help_t help)
{
    _update_presence(PRESENCE_ONLINE, "online", args);
    return TRUE;
}

static gboolean
_cmd_dnd(gchar **args, struct cmd_help_t help)
{
    _update_presence(PRESENCE_DND, "dnd", args);
    return TRUE;
}

static gboolean
_cmd_chat(gchar **args, struct cmd_help_t help)
{
    _update_presence(PRESENCE_CHAT, "chat", args);
    return TRUE;
}

static gboolean
_cmd_xa(gchar **args, struct cmd_help_t help)
{
    _update_presence(PRESENCE_XA, "xa", args);
    return TRUE;
}

// helper function for status change commands

static void
_update_presence(const jabber_presence_t presence,
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
        jabber_update_presence(presence, msg, 0);
        title_bar_set_status(presence);
        if (msg != NULL) {
            cons_show("Status set to %s, \"%s\"", show, msg);
        } else {
            cons_show("Status set to %s", show);
        }
    }

}

// helper function for boolean preference commands

static gboolean
_cmd_set_boolean_preference(gchar *arg, struct cmd_help_t help,
    const char * const display,
    void (*set_func)(gboolean))
{
    GString *enabled = g_string_new(display);
    g_string_append(enabled, " enabled.");

    GString *disabled = g_string_new(display);
    g_string_append(disabled, " disabled.");

    if (strcmp(arg, "on") == 0) {
        cons_show(enabled->str);
        set_func(TRUE);
    } else if (strcmp(arg, "off") == 0) {
        cons_show(disabled->str);
        set_func(FALSE);
    } else {
        char usage[strlen(help.usage) + 8];
        sprintf(usage, "Usage: %s", help.usage);
        cons_show(usage);
    }

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

    for (i = 0; i < ARRAY_SIZE(presence_commands); i++) {
        struct cmd_t *pcmd = presence_commands+i;
        if (strcmp(pcmd->cmd, command) == 0) {
            return pcmd;
        }
    }

    return NULL;
}

static void
_parameter_autocomplete(char *input, int *size, char *command,
    autocomplete_func func)
{
    char *found = NULL;
    char *auto_msg = NULL;
    char inp_cpy[*size];
    int i;
    char *command_cpy = malloc(strlen(command) + 2);
    sprintf(command_cpy, "%s ", command);
    int len = strlen(command_cpy);
    if ((strncmp(input, command_cpy, len) == 0) && (*size > len)) {
        for(i = len; i < *size; i++) {
            inp_cpy[i-len] = input[i];
        }
        inp_cpy[(*size) - len] = '\0';
        found = func(inp_cpy);
        if (found != NULL) {
            auto_msg = (char *) malloc((len + (strlen(found) + 1)) * sizeof(char));
            strcpy(auto_msg, command_cpy);
            strcat(auto_msg, found);
            inp_replace_input(input, auto_msg, size);
            free(auto_msg);
            free(found);
        }
    }
    free(command_cpy);
}

static void
_parameter_autocomplete_with_ac(char *input, int *size, char *command,
    PAutocomplete ac)
{
    char *found = NULL;
    char *auto_msg = NULL;
    char inp_cpy[*size];
    int i;
    char *command_cpy = malloc(strlen(command) + 2);
    sprintf(command_cpy, "%s ", command);
    int len = strlen(command_cpy);
    if ((strncmp(input, command_cpy, len) == 0) && (*size > len)) {
        for(i = len; i < *size; i++) {
            inp_cpy[i-len] = input[i];
        }
        inp_cpy[(*size) - len] = '\0';
        found = p_autocomplete_complete(ac, inp_cpy);
        if (found != NULL) {
            auto_msg = (char *) malloc((len + (strlen(found) + 1)) * sizeof(char));
            strcpy(auto_msg, command_cpy);
            strcat(auto_msg, found);
            inp_replace_input(input, auto_msg, size);
            free(auto_msg);
            free(found);
        }
    }
    free(command_cpy);
}

static void
_notify_autocomplete(char *input, int *size)
{
    char *found = NULL;
    char *auto_msg = NULL;
    char inp_cpy[*size];
    int i;

    if ((strncmp(input, "/notify message ", 16) == 0) && (*size > 16)) {
        for(i = 16; i < *size; i++) {
            inp_cpy[i-16] = input[i];
        }
        inp_cpy[(*size) - 16] = '\0';
        found = prefs_autocomplete_boolean_choice(inp_cpy);
        if (found != NULL) {
            auto_msg = (char *) malloc((16 + (strlen(found) + 1)) * sizeof(char));
            strcpy(auto_msg, "/notify message ");
            strcat(auto_msg, found);
            inp_replace_input(input, auto_msg, size);
            free(auto_msg);
            free(found);
        }
    } else if ((strncmp(input, "/notify typing ", 15) == 0) && (*size > 15)) {
        for(i = 15; i < *size; i++) {
            inp_cpy[i-15] = input[i];
        }
        inp_cpy[(*size) - 15] = '\0';
        found = prefs_autocomplete_boolean_choice(inp_cpy);
        if (found != NULL) {
            auto_msg = (char *) malloc((15 + (strlen(found) + 1)) * sizeof(char));
            strcpy(auto_msg, "/notify typing ");
            strcat(auto_msg, found);
            inp_replace_input(input, auto_msg, size);
            free(auto_msg);
            free(found);
        }
    } else if ((strncmp(input, "/notify ", 8) == 0) && (*size > 8)) {
        _parameter_autocomplete_with_ac(input, size, "/notify", notify_ac);
    }
}

static void
_titlebar_autocomplete(char *input, int *size)
{
    char *found = NULL;
    char *auto_msg = NULL;
    char inp_cpy[*size];
    int i;

    if ((strncmp(input, "/titlebar version ", 18) == 0) && (*size > 18)) {
        for(i = 18; i < *size; i++) {
            inp_cpy[i-18] = input[i];
        }
        inp_cpy[(*size) - 18] = '\0';
        found = prefs_autocomplete_boolean_choice(inp_cpy);
        if (found != NULL) {
            auto_msg = (char *) malloc((18 + (strlen(found) + 1)) * sizeof(char));
            strcpy(auto_msg, "/titlebar version ");
            strcat(auto_msg, found);
            inp_replace_input(input, auto_msg, size);
            free(auto_msg);
            free(found);
        }
    } else if ((strncmp(input, "/titlebar ", 10) == 0) && (*size > 10)) {
        _parameter_autocomplete_with_ac(input, size, "/titlebar", titlebar_ac);
    }
}

static void
_autoaway_autocomplete(char *input, int *size)
{
    char *found = NULL;
    char *auto_msg = NULL;
    char inp_cpy[*size];
    int i;

    if ((strncmp(input, "/autoaway mode ", 15) == 0) && (*size > 15)) {
        _parameter_autocomplete_with_ac(input, size, "/autoaway mode", autoaway_mode_ac);
    } else if ((strncmp(input, "/autoaway check ", 16) == 0) && (*size > 16)) {
        for(i = 16; i < *size; i++) {
            inp_cpy[i-16] = input[i];
        }
        inp_cpy[(*size) - 16] = '\0';
        found = prefs_autocomplete_boolean_choice(inp_cpy);
        if (found != NULL) {
            auto_msg = (char *) malloc((16 + (strlen(found) + 1)) * sizeof(char));
            strcpy(auto_msg, "/autoaway check ");
            strcat(auto_msg, found);
            inp_replace_input(input, auto_msg, size);
            free(auto_msg);
            free(found);
        }
    } else if ((strncmp(input, "/autoaway ", 10) == 0) && (*size > 10)) {
        _parameter_autocomplete_with_ac(input, size, "/autoaway", autoaway_ac);
    }
}

static int
_strtoi(char *str, int *saveptr, int min, int max)
{
    char *ptr;
    int val;

    errno = 0;
    val = (int)strtol(str, &ptr, 0);
    if (*str == '\0' || *ptr != '\0') {
        cons_show("Illegal character. Must be a number.");
        return -1;
    } else if (errno == ERANGE || val < min || val > max) {
        cons_show("Value out of range. Must be in %d..%d.", min, max);
        return -1;
    }

    *saveptr = val;

    return 0;
}
