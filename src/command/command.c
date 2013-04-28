/*
 * command.c
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "chat_session.h"
#include "command/command.h"
#include "command/history.h"
#include "command/parser.h"
#include "common.h"
#include "config/accounts.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "contact.h"
#include "contact_list.h"
#include "jid.h"
#include "log.h"
#include "muc.h"
#include "profanity.h"
#include "tools/autocomplete.h"
#include "tools/tinyurl.h"
#include "ui/ui.h"
#include "xmpp/xmpp.h"

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
static void _update_presence(const resource_presence_t presence,
    const char * const show, gchar **args);
static gboolean _cmd_set_boolean_preference(gchar *arg, struct cmd_help_t help,
    const char * const display, preference_t pref);

static void _cmd_complete_parameters(char *input, int *size);
static void _notify_autocomplete(char *input, int *size);
static void _titlebar_autocomplete(char *input, int *size);
static void _theme_autocomplete(char *input, int *size);
static void _autoaway_autocomplete(char *input, int *size);
static void _account_autocomplete(char *input, int *size);
static void _parameter_autocomplete(char *input, int *size, char *command,
    autocomplete_func func);
static void _parameter_autocomplete_with_ac(char *input, int *size, char *command,
    Autocomplete ac);

static int _strtoi(char *str, int *saveptr, int min, int max);

// command prototypes
static gboolean _cmd_quit(gchar **args, struct cmd_help_t help);
static gboolean _cmd_help(gchar **args, struct cmd_help_t help);
static gboolean _cmd_about(gchar **args, struct cmd_help_t help);
static gboolean _cmd_prefs(gchar **args, struct cmd_help_t help);
static gboolean _cmd_who(gchar **args, struct cmd_help_t help);
static gboolean _cmd_connect(gchar **args, struct cmd_help_t help);
static gboolean _cmd_account(gchar **args, struct cmd_help_t help);
static gboolean _cmd_disconnect(gchar **args, struct cmd_help_t help);
static gboolean _cmd_sub(gchar **args, struct cmd_help_t help);
static gboolean _cmd_msg(gchar **args, struct cmd_help_t help);
static gboolean _cmd_tiny(gchar **args, struct cmd_help_t help);
static gboolean _cmd_close(gchar **args, struct cmd_help_t help);
static gboolean _cmd_clear(gchar **args, struct cmd_help_t help);
static gboolean _cmd_join(gchar **args, struct cmd_help_t help);
static gboolean _cmd_invite(gchar **args, struct cmd_help_t help);
static gboolean _cmd_invites(gchar **args, struct cmd_help_t help);
static gboolean _cmd_decline(gchar **args, struct cmd_help_t help);
static gboolean _cmd_rooms(gchar **args, struct cmd_help_t help);
static gboolean _cmd_disco(gchar **args, struct cmd_help_t help);
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
static gboolean _cmd_set_gone(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_autoping(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_titlebar(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_autoaway(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_mouse(gchar **args, struct cmd_help_t help);
static gboolean _cmd_set_statuses(gchar **args, struct cmd_help_t help);
static gboolean _cmd_vercheck(gchar **args, struct cmd_help_t help);
static gboolean _cmd_away(gchar **args, struct cmd_help_t help);
static gboolean _cmd_online(gchar **args, struct cmd_help_t help);
static gboolean _cmd_dnd(gchar **args, struct cmd_help_t help);
static gboolean _cmd_chat(gchar **args, struct cmd_help_t help);
static gboolean _cmd_xa(gchar **args, struct cmd_help_t help);
static gboolean _cmd_info(gchar **args, struct cmd_help_t help);
static gboolean _cmd_caps(gchar **args, struct cmd_help_t help);
static gboolean _cmd_software(gchar **args, struct cmd_help_t help);
static gboolean _cmd_wins(gchar **args, struct cmd_help_t help);
static gboolean _cmd_nick(gchar **args, struct cmd_help_t help);
static gboolean _cmd_theme(gchar **args, struct cmd_help_t help);
static gboolean _cmd_status(gchar **args, struct cmd_help_t help);

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
        { "/help [list|area|command]", "Get help on using Profanity",
        { "/help [list|area|command]",
          "-------------------------",
          "list    : List of all commands.",
          "area    : One of 'basic', 'presence', 'settings', 'navigation' for more summary help in that area.",
          "command : Detailed help on a specific command.",
          "",
          "Example : /help list",
          "Example : /help connect",
          "Example : /help settings",
          "",
          "For more details help, see the user guide at http://www.profanity.im/userguide.html.",
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
        { "/connect account [server]", "Login to a chat service.",
        { "/connect account [server]",
          "-------------------------",
          "Connect to an XMPP service using the specified account.",
          "Use the server argument for chat services hosted at a different domain to the 'domainpart' of the Jabber ID.",
          "An account is automatically created if one does not exist.  See the /account command for more details.",
          "",
          "Example: /connect myuser@gmail.com",
          "Example: /connect myuser@mycompany.com talk.google.com",
          NULL  } } },

    { "/disconnect",
        _cmd_disconnect, parse_args, 0, 0,
        { "/disconnect", "Logout of current session.",
        { "/disconnect",
          "-----------",
          "Disconnect from the current chat service.",
          NULL  } } },

    { "/account",
        _cmd_account, parse_args, 0, 4,
        { "/account [command] [account] [property] [value]", "Manage accounts.",
        { "/account [command] [account] [property] [value]",
          "-----------------------------------------------",
          "Commands for creating and managing accounts.",
          "list                       : List all accounts.",
          "show account               : Show information about an account.",
          "enable account             : Enable the account, it will be used for autocomplete.",
          "disable account            : Disable the account.",
          "add account                : Create a new account.",
          "rename account newname     : Rename account to newname.",
          "set account property value : Set 'property' of 'account' to 'value'.",
          "",
          "When connected, the /account command can be called with no arguments, to show current account settings.",
          "The 'property' may be one of.",
          "jid              : The Jabber ID of the account, the account name will be used if this property is not set.",
          "server           : The chat server, if different to the domainpart of the JID.",
          "status           : The presence status to use on login, use 'last' to use whatever your last status was.",
          "online|chat|away",
          "|xa|dnd          : Priority for the specified presence.",
          "resource         : The resource to be used.",
          "",
          "Example : /account add work",
          "        : /account set work jid myuser@mycompany.com",
          "        : /account set work server talk.google.com",
          "        : /account set work resource desktop",
          "        : /account set work status dnd",
          "        : /account set work dnd -1",
          "        : /account set work online 10",
          "        : /account rename work gtalk",
          NULL  } } },

    { "/prefs",
        _cmd_prefs, parse_args, 0, 1,
        { "/prefs [area]", "Show configuration.",
        { "/prefs [area]",
          "-------------",
          "Area is one of:",
          "ui       : User interface preferences.",
          "desktop  : Desktop notification preferences.",
          "chat     : Chat state preferences.",
          "log      : Logging preferences.",
          "conn     : Connection handling preferences.",
          "presence : Chat presence preferences.",
          "",
          "No argument shows all categories.",
          NULL } } },

    { "/theme",
        _cmd_theme, parse_args, 1, 2,
        { "/theme command [theme-name]", "Change colour theme.",
        { "/theme command [theme-name]",
          "---------------------------",
          "Change the colour settings used.",
          "",
          "command : One of the following,",
          "list             : List all available themes.",
          "set [theme-name] : Load the named theme.\"default\" will reset to the default colours.",
          "",
          "Example : /theme list",
          "Example : /theme set mycooltheme",
          NULL } } },

    { "/msg",
        _cmd_msg, parse_args_with_freetext, 1, 2,
        { "/msg jid|nick [message]", "Start chat with user.",
        { "/msg jid|nick [message]",
          "-----------------------",
          "Open a chat window with for the user JID (Jabber ID) and send the message if one is supplied.",
          "When in a chat room, supply the nickname to start private chat with the room member.",
          "Use quotes if the nickname includes spaces.",
          "",
          "Example : /msg myfriend@server.com Hey, here's a message!",
          "Example : /msg otherfriend@server.com",
          "Example : /msg Bob Here is a private message",
          "Example : /msg \"My Friend\" Hi, how are you?",
          NULL } } },

    { "/info",
        _cmd_info, parse_args, 0, 1,
        { "/info [jid|nick]", "Show basic information about a contact, or room member.",
        { "/info [jid|nick]",
          "----------------",
          "Show information including current subscription status and summary information for each connected resource.",
          "If in a chat window the parameter is not required, the current recipient will be used.",
          "",
          "Example : /info mybuddy@chat.server.org (contact)",
          "Example : /info kai (room member)",
          NULL } } },

    { "/caps",
        _cmd_caps, parse_args, 0, 1,
        { "/caps [fulljid|nick]", "Find out a contacts client software capabilities.",
        { "/caps [fulljid|nick]",
          "--------------------",
          "Find out a contact, or room members client software capabilities.",
          "If in the console window or a regular chat window, a full JID is required.",
          "If in a chat room, the nickname is required.",
          "If in private chat, no parameter is required.",
          "",
          "Example : /caps mybuddy@chat.server.org/laptop (contact's laptop resource)",
          "Example : /caps mybuddy@chat.server.org/phone (contact's phone resource)",
          "Example : /caps bruce (room member)",
          NULL } } },

    { "/software",
        _cmd_software, parse_args, 0, 1,
        { "/software [fulljid|nick]", "Find out software version information about a contacts resource.",
        { "/software [fulljid|nick]",
          "------------------------",
          "Find out a contact, or room members software version information, if such requests are supported.",
          "If in the console window or a regular chat window, a full JID is required.",
          "If in a chat room, the nickname is required.",
          "If in private chat, no parameter is required.",
          "If the contacts software does not support software version requests, nothing will be displayed.",
          "",
          "Example : /software mybuddy@chat.server.org/laptop (contact's laptop resource)",
          "Example : /software mybuddy@chat.server.org/phone (contact's phone resource)",
          "Example : /software bruce (room member)",
          NULL } } },

    { "/status",
        _cmd_status, parse_args, 0, 1,
        { "/status [jid|nick]", "Find out a contacts presence information.",
        { "/status [jid|nick]",
          "------------------",
          "Find out a contact, or room members presence information.",
          "If in a chat window the parameter is not required, the current recipient will be used.",
          "",
          "Example : /status buddy@server.com (contact)",
          "Example : /status jon (room member)",
          NULL } } },

    { "/join",
        _cmd_join, parse_args_with_freetext, 1, 2,
        { "/join room[@server] [nick]", "Join a chat room.",
        { "/join room[@server] [nick]",
          "--------------------------",
          "Join a chat room at the conference server.",
          "If nick is specified you will join with this nickname.",
          "Otherwise the 'localpart' of your JID (before the @) will be used.",
          "If no server is supplied, it will default to 'conference.<domain-part>'",
          "If the room doesn't exist, and the server allows it, a new one will be created.",
          "",
          "Example : /join jdev@conference.jabber.org",
          "Example : /join jdev@conference.jabber.org mynick",
          "Example : /join jdev (as user@jabber.org will join jdev@conference.jabber.org)",
          NULL } } },

    { "/invite",
        _cmd_invite, parse_args_with_freetext, 1, 2,
        { "/invite jid [message]", "Invite contact to chat room.",
        { "/invite jid [message]",
          "--------------------------",
          "Send a direct invite to the specified contact to the current chat room.",
          "The jid must be a contact in your roster.",
          "If a message is supplied it will be send as the reason for the invite.",
          NULL } } },

    { "/invites",
        _cmd_invites, parse_args_with_freetext, 0, 0,
        { "/invites", "Show outstanding chat room invites.",
        { "/invites",
          "--------",
          "Show all rooms that you have been invited to, and have not yet been accepted or declind.",
          "Use \"/join <room>\" to accept a room invitation.",
          "Use \"/decline <room>\" to decline a room invitation.",
          NULL } } },

    { "/decline",
        _cmd_decline, parse_args_with_freetext, 1, 1,
        { "/decline room", "Decline a chat room invite.",
        { "/decline room",
          "-------------",
          "Decline invitation to a chat room, the room will no longer be in the list of outstanding invites.",
          NULL } } },

    { "/rooms",
        _cmd_rooms, parse_args, 0, 1,
        { "/rooms [conference-service]", "List chat rooms.",
        { "/rooms [conference-service]",
          "---------------------------",
          "List the chat rooms available at the specified conference service",
          "If no argument is supplied, the domainpart of the current logged in JID is used,",
          "with a prefix of 'conference'.",
          "",
          "Example : /rooms conference.jabber.org",
          "Example : /rooms (if logged in as me@server.org, is equivalent to /rooms conference.server.org)",
          NULL } } },

    { "/disco",
        _cmd_disco, parse_args, 1, 2,
        { "/disco command entity", "Service discovery.",
        { "/disco command entity",
          "---------------------",
          "Find out information about an entities supported services.",
          "Command may be one of:",
          "info: List protocols and features supported by an entity.",
          "items: List items associated with an entity.",
          "",
          "The entity must be a Jabber ID.",
          "",
          "Example : /disco info myserver.org",
          "Example : /disco items myserver.org",
          "Example : /disco items conference.jabber.org",
          "Example : /disco info myfriend@server.com/laptop",
          NULL } } },

    { "/nick",
        _cmd_nick, parse_args_with_freetext, 1, 1,
        { "/nick nickname", "Change nickname in chat room.",
        { "/nick nickname",
          "--------------",
          "Change the name by which other members of a chat room see you.",
          "This command is only valid when called within a chat room window.",
          "",
          "Example : /nick kai hansen",
          "Example : /nick bob",
          NULL } } },

    { "/wins",
        _cmd_wins, parse_args, 0, 0,
        { "/wins", "List active windows.",
        { "/wins",
          "-----",
          "List all currently active windows and information about their usage.",
          NULL } } },

    { "/sub",
        _cmd_sub, parse_args, 1, 2,
        { "/sub command [jid]", "Manage subscriptions.",
        { "/sub command [jid]",
          "------------------",
          "command : One of the following,",
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
          "",
          "Example : /tiny http://www.google.com",
          NULL } } },

    { "/who",
        _cmd_who, parse_args, 0, 1,
        { "/who [status]", "Show contacts/room participants with chosen status.",
        { "/who [status]",
          "-------------",
          "Show contacts with the specified status, no status shows all contacts.",
          "Possible statuses are: online, offline, away, dnd, xa, chat, available, unavailable.",
          "If in a chat room, the participants with the supplied status are displayed.",
          "",
          "online      : Contacts that are connected, i.e. online, chat, away, xa, dnd",
          "available   : Contacts that are available for chat, i.e. online, chat.",
          "unavailable : Contacts that are not available for chat, i.e. offline, away, xa, dnd.",
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

    { "/clear",
        _cmd_clear, parse_args, 0, 0,
        { "/clear", "Clear current window.",
        { "/clear",
          "------",
          "Clear the current window.",
          NULL } } },

    { "/quit",
        _cmd_quit, parse_args, 0, 0,
        { "/quit", "Quit Profanity.",
        { "/quit",
          "-----",
          "Logout of any current session, and quit Profanity.",
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
          "If the terminal does not support sounds, it may attempt to flash the screen instead.",
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
          "invite  : Notifications for chat room invites.",
          "        : on|off",
          "",
          "sub     : Notifications for subscription requests.",
          "        : on|off",
          "",
          "Example : /notify message on (enable message notifications)",
          "Example : /notify remind 10  (remind every 10 seconds)",
          "Example : /notify remind 0   (switch off reminders)",
          "Example : /notify typing on  (enable typing notifications)",
          "Example : /notify invite on  (enable chat room invite notifications)",
          NULL } } },

    { "/flash",
        _cmd_set_flash, parse_args, 1, 1,
        { "/flash on|off", "Terminal flash on new messages.",
        { "/flash on|off",
          "-------------",
          "Make the terminal flash when incoming messages are recieved.",
          "The flash will only occur if you are not in the chat window associated with the user sending the message.",
          "If the terminal doesn't support flashing, it may attempt to beep.",
          NULL } } },

    { "/intype",
        _cmd_set_intype, parse_args, 1, 1,
        { "/intype on|off", "Show when contact is typing.",
        { "/intype on|off",
          "--------------",
          "Show when a contact is typing in the console, and in active message window.",
          NULL } } },

    { "/splash",
        _cmd_set_splash, parse_args, 1, 1,
        { "/splash on|off", "Splash logo on startup and /about command.",
        { "/splash on|off",
          "--------------",
          "Switch on or off the ascii logo on start up and when the /about command is called.",
          NULL } } },

    { "/vercheck",
        _cmd_vercheck, parse_args, 0, 1,
        { "/vercheck [on|off]", "Check for a new release.",
        { "/vercheck [on|off]",
          "------------------",
          "Without a parameter will check for a new release.",
          "Switching on or off will enable/disable a version check when Profanity starts, and each time the /about command is run.",
          NULL  } } },

    { "/titlebar",
        _cmd_set_titlebar, parse_args, 2, 2,
        { "/titlebar property on|off", "Show various properties in the window title bar.",
        { "/titlebar property on|off",
          "-------------------------",
          "Show various properties in the window title bar.",
          "Currently The only supported property is 'version'.",
          NULL  } } },

    { "/mouse",
        _cmd_set_mouse, parse_args, 1, 1,
        { "/mouse on|off", "Use profanity mouse handling.",
        { "/mouse on|off",
          "-------------",
          "If set to 'on', profanity will handle mouse actions, which enables scrolling the main window with the mouse wheel.",
          "To select text, use the shift key while selcting an area.",
          "If set to 'off', profanity leaves mouse handling to the terminal implementation.",
          "This feature is experimental, certain mouse click events may occasionally freeze",
          "Profanity until a key is pressed or another mouse event is received",
          "The default is 'off'.",
          NULL } } },

    { "/chlog",
        _cmd_set_chlog, parse_args, 1, 1,
        { "/chlog on|off", "Chat logging to file",
        { "/chlog on|off",
          "-------------",
          "Switch chat logging on or off.",
          "This setting will be enabled if /history is set to on.",
          "When disabling this option, /history will also be disabled.",
          NULL } } },

    { "/states",
        _cmd_set_states, parse_args, 1, 1,
        { "/states on|off", "Send chat states during a chat session.",
        { "/states on|off",
          "--------------",
          "Sending of chat state notifications during chat sessions.",
          "Such as whether you have become inactive, or have closed the chat window.",
          NULL } } },

    { "/outtype",
        _cmd_set_outtype, parse_args, 1, 1,
        { "/outtype on|off", "Send typing notification to recipient.",
        { "/outtype on|off",
          "---------------",
          "Send an indication that you are typing to the chat recipient.",
          "Chat states (/states) will be enabled if this setting is set.",
          NULL } } },

    { "/gone",
        _cmd_set_gone, parse_args, 1, 1,
        { "/gone minutes", "Send 'gone' state to recipient after a period.",
        { "/gone minutes",
          "-------------",
          "Send a 'gone' state to the recipient after the specified number of minutes."
          "This indicates to the recipient's client that you have left the conversation.",
          "A value of 0 will disable sending this chat state.",
          "Chat states (/states) will be enabled if this setting is set.",
          NULL } } },

    { "/history",
        _cmd_set_history, parse_args, 1, 1,
        { "/history on|off", "Chat history in message windows.",
        { "/history on|off",
          "---------------",
          "Switch chat history on or off, requires /chlog will automatically be enabled when this setting in on.",
          "When history is enabled, previous messages are shown in chat windows.",
          NULL } } },

    { "/log",
        _cmd_set_log, parse_args, 2, 2,
        { "/log maxsize value", "Manage system logging settings.",
        { "/log maxsize value",
          "------------------",
          "maxsize : When log file size exceeds this value it will be automatically",
          "          rotated (file will be renamed). Default value is 1048580 (1MB)",
          NULL } } },

    { "/reconnect",
        _cmd_set_reconnect, parse_args, 1, 1,
        { "/reconnect seconds", "Set reconnect interval.",
        { "/reconnect seconds",
          "------------------",
          "Set the reconnect attempt interval in seconds for when the connection is lost.",
          "A value of 0 will switch of reconnect attempts.",
          NULL } } },

    { "/autoping",
        _cmd_set_autoping, parse_args, 1, 1,
        { "/autoping seconds", "Server ping interval.",
        { "/autoping seconds",
          "-----------------",
          "Set the number of seconds between server pings, so ensure connection kept alive.",
          "A value of 0 will switch off autopinging the server.",
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
          NULL } } },

    { "/priority",
        _cmd_set_priority, parse_args, 1, 1,
        { "/priority value", "Set priority for the current account.",
        { "/priority value",
          "---------------",
          "Set priority for the current account, presence will be sent when calling this command.",
          "See the /account command for more specific priority settings per presence status.",
          "value : Number between -128 and 127. Default value is 0.",
          NULL } } },

    { "/statuses",
        _cmd_set_statuses, parse_args, 1, 1,
        { "/statuses on|off", "Set notifications for status messages.",
        { "/statuses on|off",
          "----------------",
          "Show status updates from contacts, such as online/offline/away etc.",
          "When disabled, status updates are not displayed.",
          "The default is 'on'.",
          NULL } } }
};

static struct cmd_t presence_commands[] =
{
    { "/away",
        _cmd_away, parse_args_with_freetext, 0, 1,
        { "/away [msg]", "Set status to away.",
        { "/away [msg]",
          "-----------",
          "Set your status to 'away' with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /away Gone for lunch",
          NULL } } },

    { "/chat",
        _cmd_chat, parse_args_with_freetext, 0, 1,
        { "/chat [msg]", "Set status to chat (available for chat).",
        { "/chat [msg]",
          "-----------",
          "Set your status to 'chat', meaning 'available for chat', with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /chat Please talk to me!",
          NULL } } },

    { "/dnd",
        _cmd_dnd, parse_args_with_freetext, 0, 1,
        { "/dnd [msg]", "Set status to dnd (do not disturb).",
        { "/dnd [msg]",
          "----------",
          "Set your status to 'dnd', meaning 'do not disturb', with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /dnd I'm in the zone",
          NULL } } },

    { "/online",
        _cmd_online, parse_args_with_freetext, 0, 1,
        { "/online [msg]", "Set status to online.",
        { "/online [msg]",
          "-------------",
          "Set your status to 'online' with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /online Up the Irons!",
          NULL } } },

    { "/xa",
        _cmd_xa, parse_args_with_freetext, 0, 1,
        { "/xa [msg]", "Set status to xa (extended away).",
        { "/xa [msg]",
          "---------",
          "Set your status to 'xa', meaning 'extended away', with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /xa This meeting is going to be a long one",
          NULL } } },
};

static Autocomplete commands_ac;
static Autocomplete who_ac;
static Autocomplete help_ac;
static Autocomplete notify_ac;
static Autocomplete prefs_ac;
static Autocomplete sub_ac;
static Autocomplete log_ac;
static Autocomplete autoaway_ac;
static Autocomplete autoaway_mode_ac;
static Autocomplete titlebar_ac;
static Autocomplete theme_ac;
static Autocomplete theme_load_ac;
static Autocomplete account_ac;
static Autocomplete disco_ac;

/*
 * Initialise command autocompleter and history
 */
void
cmd_init(void)
{
    log_info("Initialising commands");

    commands_ac = autocomplete_new();
    who_ac = autocomplete_new();

    prefs_ac = autocomplete_new();
    autocomplete_add(prefs_ac, strdup("ui"));
    autocomplete_add(prefs_ac, strdup("desktop"));
    autocomplete_add(prefs_ac, strdup("chat"));
    autocomplete_add(prefs_ac, strdup("log"));
    autocomplete_add(prefs_ac, strdup("conn"));
    autocomplete_add(prefs_ac, strdup("presence"));

    help_ac = autocomplete_new();
    autocomplete_add(help_ac, strdup("list"));
    autocomplete_add(help_ac, strdup("basic"));
    autocomplete_add(help_ac, strdup("presence"));
    autocomplete_add(help_ac, strdup("settings"));
    autocomplete_add(help_ac, strdup("navigation"));

    notify_ac = autocomplete_new();
    autocomplete_add(notify_ac, strdup("message"));
    autocomplete_add(notify_ac, strdup("typing"));
    autocomplete_add(notify_ac, strdup("remind"));
    autocomplete_add(notify_ac, strdup("invite"));
    autocomplete_add(notify_ac, strdup("sub"));
    autocomplete_add(notify_ac, strdup("status"));

    sub_ac = autocomplete_new();
    autocomplete_add(sub_ac, strdup("request"));
    autocomplete_add(sub_ac, strdup("allow"));
    autocomplete_add(sub_ac, strdup("deny"));
    autocomplete_add(sub_ac, strdup("show"));
    autocomplete_add(sub_ac, strdup("sent"));
    autocomplete_add(sub_ac, strdup("received"));

    titlebar_ac = autocomplete_new();
    autocomplete_add(titlebar_ac, strdup("version"));

    log_ac = autocomplete_new();
    autocomplete_add(log_ac, strdup("maxsize"));

    autoaway_ac = autocomplete_new();
    autocomplete_add(autoaway_ac, strdup("mode"));
    autocomplete_add(autoaway_ac, strdup("time"));
    autocomplete_add(autoaway_ac, strdup("message"));
    autocomplete_add(autoaway_ac, strdup("check"));

    autoaway_mode_ac = autocomplete_new();
    autocomplete_add(autoaway_mode_ac, strdup("away"));
    autocomplete_add(autoaway_mode_ac, strdup("idle"));
    autocomplete_add(autoaway_mode_ac, strdup("off"));

    theme_ac = autocomplete_new();
    autocomplete_add(theme_ac, strdup("list"));
    autocomplete_add(theme_ac, strdup("set"));

    disco_ac = autocomplete_new();
    autocomplete_add(disco_ac, strdup("info"));
    autocomplete_add(disco_ac, strdup("items"));

    account_ac = autocomplete_new();
    autocomplete_add(account_ac, strdup("list"));
    autocomplete_add(account_ac, strdup("show"));
    autocomplete_add(account_ac, strdup("add"));
    autocomplete_add(account_ac, strdup("enable"));
    autocomplete_add(account_ac, strdup("disable"));
    autocomplete_add(account_ac, strdup("rename"));
    autocomplete_add(account_ac, strdup("set"));

    theme_load_ac = NULL;

    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(main_commands); i++) {
        struct cmd_t *pcmd = main_commands+i;
        autocomplete_add(commands_ac, (gchar *)strdup(pcmd->cmd));
        autocomplete_add(help_ac, (gchar *)strdup(pcmd->cmd+1));
    }

    for (i = 0; i < ARRAY_SIZE(setting_commands); i++) {
        struct cmd_t *pcmd = setting_commands+i;
        autocomplete_add(commands_ac, (gchar *)strdup(pcmd->cmd));
        autocomplete_add(help_ac, (gchar *)strdup(pcmd->cmd+1));
    }

    for (i = 0; i < ARRAY_SIZE(presence_commands); i++) {
        struct cmd_t *pcmd = presence_commands+i;
        autocomplete_add(commands_ac, (gchar *)strdup(pcmd->cmd));
        autocomplete_add(help_ac, (gchar *)strdup(pcmd->cmd+1));
        autocomplete_add(who_ac, (gchar *)strdup(pcmd->cmd+1));
    }

    autocomplete_add(who_ac, strdup("offline"));
    autocomplete_add(who_ac, strdup("available"));
    autocomplete_add(who_ac, strdup("unavailable"));

    cmd_history_init();
}

void
cmd_close(void)
{
    autocomplete_free(commands_ac);
    autocomplete_free(who_ac);
    autocomplete_free(help_ac);
    autocomplete_free(notify_ac);
    autocomplete_free(sub_ac);
    autocomplete_free(log_ac);
    autocomplete_free(prefs_ac);
    autocomplete_free(autoaway_ac);
    autocomplete_free(autoaway_mode_ac);
    autocomplete_free(theme_ac);
    if (theme_load_ac != NULL) {
        autocomplete_free(theme_load_ac);
    }
    autocomplete_free(account_ac);
    autocomplete_free(disco_ac);
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
        found = autocomplete_complete(commands_ac, inp_cpy);
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
    muc_reset_invites_ac();
    accounts_reset_all_search();
    accounts_reset_enabled_search();
    prefs_reset_boolean_choice();
    autocomplete_reset(help_ac);
    autocomplete_reset(notify_ac);
    autocomplete_reset(sub_ac);

    if (ui_current_win_type() == WIN_MUC) {
        Autocomplete nick_ac = muc_get_roster_ac(ui_current_recipient());
        if (nick_ac != NULL) {
            autocomplete_reset(nick_ac);
        }
    }

    autocomplete_reset(who_ac);
    autocomplete_reset(prefs_ac);
    autocomplete_reset(log_ac);
    autocomplete_reset(commands_ac);
    autocomplete_reset(autoaway_ac);
    autocomplete_reset(autoaway_mode_ac);
    autocomplete_reset(theme_ac);
    if (theme_load_ac != NULL) {
        autocomplete_reset(theme_load_ac);
        theme_load_ac = NULL;
    }
    autocomplete_reset(account_ac);
    autocomplete_reset(disco_ac);
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
            if (ui_current_win_type() == WIN_CHAT) {
                char usage[strlen(cmd->help.usage) + 8];
                sprintf(usage, "Usage: %s", cmd->help.usage);
                ui_current_print_line(usage);
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
    win_type_t win_type = ui_current_win_type();
    jabber_conn_status_t status = jabber_get_connection_status();
    char *recipient = ui_current_recipient();

    switch (win_type)
    {
        case WIN_MUC:
            if (status != JABBER_CONNECTED) {
                ui_current_print_line("You are not currently connected.");
            } else {
                message_send_groupchat(inp, recipient);
                free(recipient);
            }
            break;

        case WIN_CHAT:
            if (status != JABBER_CONNECTED) {
                ui_current_print_line("You are not currently connected.");
            } else {
                message_send(inp, recipient);

                if (prefs_get_boolean(PREF_CHLOG)) {
                    const char *jid = jabber_get_jid();
                    Jid *jidp = jid_create(jid);
                    chat_log_chat(jidp->barejid, recipient, inp, PROF_OUT_LOG, NULL);
                    jid_destroy(jidp);
                }

                ui_outgoing_msg("me", recipient, inp);
                free(recipient);
            }
            break;

        case WIN_PRIVATE:
            if (status != JABBER_CONNECTED) {
                ui_current_print_line("You are not currently connected.");
            } else {
                message_send(inp, recipient);
                ui_outgoing_msg("me", recipient, inp);
                free(recipient);
            }
            break;

        case WIN_CONSOLE:
            cons_show("Unknown command: %s", inp);
            break;
        default:
            break;
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
    _parameter_autocomplete(input, size, "/mouse",
        prefs_autocomplete_boolean_choice);
    _parameter_autocomplete(input, size, "/history",
        prefs_autocomplete_boolean_choice);
    _parameter_autocomplete(input, size, "/vercheck",
        prefs_autocomplete_boolean_choice);
    _parameter_autocomplete(input, size, "/statuses",
        prefs_autocomplete_boolean_choice);

    if (ui_current_win_type() == WIN_MUC) {
        Autocomplete nick_ac = muc_get_roster_ac(ui_current_recipient());
        if (nick_ac != NULL) {
            _parameter_autocomplete_with_ac(input, size, "/msg", nick_ac);
            _parameter_autocomplete_with_ac(input, size, "/info", nick_ac);
            _parameter_autocomplete_with_ac(input, size, "/caps", nick_ac);
            _parameter_autocomplete_with_ac(input, size, "/status", nick_ac);
            _parameter_autocomplete_with_ac(input, size, "/software", nick_ac);
        }
    } else {
        _parameter_autocomplete(input, size, "/msg",
            contact_list_find_contact);
        _parameter_autocomplete(input, size, "/info",
            contact_list_find_contact);
        _parameter_autocomplete(input, size, "/caps",
            contact_list_find_resource);
        _parameter_autocomplete(input, size, "/status",
            contact_list_find_contact);
        _parameter_autocomplete(input, size, "/software",
            contact_list_find_resource);
    }

    _parameter_autocomplete(input, size, "/invite", contact_list_find_contact);
    _parameter_autocomplete(input, size, "/decline", muc_find_invite);
    _parameter_autocomplete(input, size, "/join", muc_find_invite);


    _parameter_autocomplete(input, size, "/connect",
        accounts_find_enabled);
    _parameter_autocomplete_with_ac(input, size, "/sub", sub_ac);
    _parameter_autocomplete_with_ac(input, size, "/help", help_ac);
    _parameter_autocomplete_with_ac(input, size, "/who", who_ac);
    _parameter_autocomplete_with_ac(input, size, "/prefs", prefs_ac);
    _parameter_autocomplete_with_ac(input, size, "/log", log_ac);
    _parameter_autocomplete_with_ac(input, size, "/disco", disco_ac);

    _notify_autocomplete(input, size);
    _autoaway_autocomplete(input, size);
    _titlebar_autocomplete(input, size);
    _theme_autocomplete(input, size);
    _account_autocomplete(input, size);
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
        char *jid;

        status_bar_get_password();
        status_bar_refresh();
        char passwd[21];
        inp_block();
        inp_get_password(passwd);
        inp_non_block();

        ProfAccount *account = accounts_get_account(lower);
        if (account != NULL) {
            if (account->resource != NULL) {
                jid = create_fulljid(account->jid, account->resource);
            } else {
                jid = strdup(account->jid);
            }
            cons_show("Connecting with account %s as %s", account->name, jid);
            conn_status = jabber_connect_with_account(account, passwd);
        } else {
            jid = strdup(lower);
            cons_show("Connecting as %s", jid);
            conn_status = jabber_connect_with_details(jid, passwd, altdomain);
        }

        if (conn_status == JABBER_DISCONNECTED) {
            cons_show_error("Connection attempt for %s failed.", jid);
            log_debug("Connection attempt for %s failed", jid);
        }

        accounts_free_account(account);
        free(jid);

        result = TRUE;
    }

    return result;
}

static gboolean
_cmd_account(gchar **args, struct cmd_help_t help)
{
    char *command = args[0];

    if (command == NULL) {
        if (jabber_get_connection_status() != JABBER_CONNECTED) {
            cons_show("Usage: %s", help.usage);
        } else {
            ProfAccount *account = accounts_get_account(jabber_get_account_name());
            cons_show_account(account);
            accounts_free_account(account);
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
                accounts_free_account(account);
            }
        }
    } else if (strcmp(command, "add") == 0) {
        char *account_name = args[1];
        if (account_name == NULL) {
            cons_show("Usage: %s", help.usage);
        } else {
            accounts_add(account_name, NULL);
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
                cons_show("Account %s doesn't exist");
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
                } else if (strcmp(property, "resource") == 0) {
                    accounts_set_resource(account_name, value);
                    cons_show("Updated resource for account %s: %s", account_name, value);
                    cons_show("");
                } else if (strcmp(property, "status") == 0) {
                    if (!valid_resource_presence_string(value) && (strcmp(value, "last") != 0)) {
                        cons_show("Invalud status: %s", value);
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
                            resource_presence_t last_presence = accounts_get_last_presence(jabber_get_account_name());
                            if (conn_status == JABBER_CONNECTED && presence_type == last_presence) {
                                presence_update(last_presence, jabber_get_presence_message(), 0);
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
    } else {
        cons_show("");
    }

    return TRUE;
}

static gboolean
_cmd_sub(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();
    win_type_t win_type = ui_current_win_type();

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
        cons_show_sent_subs();
        return TRUE;
    }

    if (strcmp(subcmd, "received") == 0) {
        cons_show_received_subs();
        return TRUE;
    }

    if ((win_type != WIN_CHAT) && (jid == NULL)) {
        cons_show("You must specify a contact.");
        return TRUE;
    }

    if (jid != NULL) {
        jid = strdup(jid);
    } else {
        jid = ui_current_recipient();
    }

    bare_jid = strtok(jid, "/");

    if (strcmp(subcmd, "allow") == 0) {
        presence_subscription(bare_jid, PRESENCE_SUBSCRIBED);
        cons_show("Accepted subscription for %s", bare_jid);
        log_info("Accepted subscription for %s", bare_jid);
    } else if (strcmp(subcmd, "deny") == 0) {
        presence_subscription(bare_jid, PRESENCE_UNSUBSCRIBED);
        cons_show("Deleted/denied subscription for %s", bare_jid);
        log_info("Deleted/denied subscription for %s", bare_jid);
    } else if (strcmp(subcmd, "request") == 0) {
        presence_subscription(bare_jid, PRESENCE_SUBSCRIBE);
        cons_show("Sent subscription request to %s.", bare_jid);
        log_info("Sent subscription request to %s.", bare_jid);
    } else if (strcmp(subcmd, "show") == 0) {
        PContact contact = contact_list_get_contact(bare_jid);
        if ((contact == NULL) || (p_contact_subscription(contact) == NULL)) {
            if (win_type == WIN_CHAT) {
                ui_current_print_line("No subscription information for %s.", bare_jid);
            } else {
                cons_show("No subscription information for %s.", bare_jid);
            }
        } else {
            if (win_type == WIN_CHAT) {
                if (p_contact_pending_out(contact)) {
                    ui_current_print_line("%s subscription status: %s, request pending.",
                        bare_jid, p_contact_subscription(contact));
                } else {
                    ui_current_print_line("%s subscription status: %s.", bare_jid,
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
        cons_show("Presence commands:");
        cons_show_time();
        for (i = 0; i < ARRAY_SIZE(presence_commands); i++) {
            cons_show_word( (presence_commands+i)->cmd );
            if (i < ARRAY_SIZE(presence_commands) - 1) {
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
    if (ui_current_win_type() != WIN_CONSOLE) {
        status_bar_new(0);
    }
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

static gboolean
_cmd_who(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();
    win_type_t win_type = ui_current_win_type();

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
            if (win_type == WIN_MUC) {
                char *room = ui_current_recipient();
                GList *list = muc_get_roster(room);

                // no arg, show all contacts
                if (presence == NULL) {
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
                        if (p_contact_is_available(contact)) {
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
                        if (!p_contact_is_available(contact)) {
                            filtered = g_slist_append(filtered, contact);
                        }
                        list = g_slist_next(list);
                    }

                    cons_show_contacts(filtered);

                // online, available resources
                } else if (strcmp("online", presence) == 0) {
                    cons_show("Contacts (%s):", presence);
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
                } else if (strcmp("online", presence) == 0) {
                    cons_show("Contacts (%s):", presence);
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

    if (win_type != WIN_CONSOLE) {
        status_bar_new(0);
    }

    return TRUE;
}

static gboolean
_cmd_msg(gchar **args, struct cmd_help_t help)
{
    char *usr = args[0];
    char *msg = args[1];

    jabber_conn_status_t conn_status = jabber_get_connection_status();
    win_type_t win_type = ui_current_win_type();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (ui_windows_full()) {
        cons_show_error("Windows all used, close a window and try again.");
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
        if (msg != NULL) {
            message_send(msg, usr);
            ui_outgoing_msg("me", usr, msg);

            if (((win_type == WIN_CHAT) || (win_type == WIN_CONSOLE)) && prefs_get_boolean(PREF_CHLOG)) {
                const char *jid = jabber_get_jid();
                Jid *jidp = jid_create(jid);
                chat_log_chat(jidp->barejid, usr, msg, PROF_OUT_LOG, NULL);
                jid_destroy(jidp);
            }

            return TRUE;
        } else {
            ui_new_chat_win(usr);
            return TRUE;
        }
    }
}

static gboolean
_cmd_status(gchar **args, struct cmd_help_t help)
{
    char *usr = args[0];

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
                cons_show_status(usr);
            } else {
                cons_show("Usage: %s", help.usage);
            }
            break;
        default:
            break;
    }

    return TRUE;
}

static gboolean
_cmd_info(gchar **args, struct cmd_help_t help)
{
    char *usr = args[0];

    jabber_conn_status_t conn_status = jabber_get_connection_status();
    win_type_t win_type = ui_current_win_type();
    PContact pcontact = NULL;

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    switch (win_type)
    {
        case WIN_MUC:
            if (usr != NULL) {
                pcontact = muc_get_participant(ui_current_recipient(), usr);
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
                pcontact = contact_list_get_contact(ui_current_recipient());
                if (pcontact != NULL) {
                    cons_show_info(pcontact);
                } else {
                    cons_show("No such contact \"%s\" in roster.", ui_current_recipient());
                }
            }
            break;
        case WIN_PRIVATE:
            if (usr != NULL) {
                ui_current_print_line("No parameter required when in chat.");
            } else {
                Jid *jid = jid_create(ui_current_recipient());
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
                pcontact = contact_list_get_contact(usr);
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

static gboolean
_cmd_caps(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();
    win_type_t win_type = ui_current_win_type();
    PContact pcontact = NULL;

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    switch (win_type)
    {
        case WIN_MUC:
            if (args[0] != NULL) {
                pcontact = muc_get_participant(ui_current_recipient(), args[0]);
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
                    pcontact = contact_list_get_contact(jid->barejid);
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
                Jid *jid = jid_create(ui_current_recipient());
                pcontact = muc_get_participant(jid->barejid, jid->resourcepart);
                Resource *resource = p_contact_get_resource(pcontact, jid->resourcepart);
                cons_show_caps(jid->resourcepart, resource);
            }
            break;
        default:
            break;
    }

    return TRUE;
}


static gboolean
_cmd_software(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();
    win_type_t win_type = ui_current_win_type();
    PContact pcontact = NULL;

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    switch (win_type)
    {
        case WIN_MUC:
            if (args[0] != NULL) {
                pcontact = muc_get_participant(ui_current_recipient(), args[0]);
                if (pcontact != NULL) {
                    Jid *jid = jid_create_from_bare_and_resource(ui_current_recipient(), args[0]);
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

                if (jid->fulljid == NULL) {
                    cons_show("You must provide a full jid to the /software command.");
                } else {
                    iq_send_software_version(jid->fulljid);
                }
            } else {
                cons_show("You must provide a jid to the /software command.");
            }
            break;
        case WIN_PRIVATE:
            if (args[0] != NULL) {
                cons_show("No parameter needed to /software when in private chat.");
            } else {
                iq_send_software_version(ui_current_recipient());
            }
            break;
        default:
            break;
    }

    return TRUE;
}

static gboolean
_cmd_join(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (ui_windows_full()) {
        cons_show_error("Windows all used, close a window and try again.");
        return TRUE;
    }

    int num_args = g_strv_length(args);
    char *room = NULL;
    char *nick = NULL;
    Jid *room_arg = jid_create(args[0]);
    GString *room_str = g_string_new("");
    Jid *my_jid = jid_create(jabber_get_jid());

    // full room jid supplied (room@server)
    if (room_arg->localpart != NULL) {
        room = args[0];

    // server not supplied (room), guess conference.<users-domain-part>
    } else {
        g_string_append(room_str, args[0]);
        g_string_append(room_str, "@conference.");
        g_string_append(room_str, strdup(my_jid->domainpart));
        room = room_str->str;
    }

    // nick supplied
    if (num_args == 2) {
        nick = args[1];

    // use localpart for nick
    } else {
        nick = my_jid->localpart;
    }

    Jid *room_jid = jid_create_from_bare_and_resource(room, nick);

    if (!muc_room_is_active(room_jid)) {
        presence_join_room(room_jid);
    }
    ui_room_join(room_jid);
    muc_remove_invite(room);

    jid_destroy(room_jid);
    jid_destroy(my_jid);
    g_string_free(room_str, TRUE);

    return TRUE;
}

static gboolean
_cmd_invite(gchar **args, struct cmd_help_t help)
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

    room = ui_current_recipient();
    message_send_invite(room, contact, reason);
    if (reason != NULL) {
        cons_show("Room invite sent, contact: %s, room: %s, reason: \"%s\".",
            contact, room, reason);
    } else {
        cons_show("Room invite sent, contact: %s, room: %s.",
            contact, room);
    }

    return TRUE;
}

static gboolean
_cmd_invites(gchar **args, struct cmd_help_t help)
{
    GSList *invites = muc_get_invites();
    cons_show_room_invites(invites);
    g_slist_free_full(invites, g_free);
    return TRUE;
}

static gboolean
_cmd_decline(gchar **args, struct cmd_help_t help)
{
    if (!muc_invites_include(args[0])) {
        cons_show("No such invite exists.");
    } else {
        muc_remove_invite(args[0]);
        cons_show("Declined invite to %s.", args[0]);
    }

    return TRUE;
}

static gboolean
_cmd_rooms(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are currenlty connect.");
        return TRUE;
    }

    if (args[0] == NULL) {
        Jid *jid = jid_create(jabber_get_jid());
        GString *conference_node = g_string_new("conference.");
        g_string_append(conference_node, strdup(jid->domainpart));
        jid_destroy(jid);
        iq_room_list_request(conference_node->str);
        g_string_free(conference_node, TRUE);
    } else {
        iq_room_list_request(args[0]);
    }

    return TRUE;
}

static gboolean
_cmd_disco(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are currenlty connect.");
        return TRUE;
    }

    GString *jid = g_string_new("");
    if (args[1] != NULL) {
        jid = g_string_append(jid, args[1]);
    } else {
        Jid *jidp = jid_create(jabber_get_jid());
        jid = g_string_append(jid, strdup(jidp->domainpart));
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

static gboolean
_cmd_nick(gchar **args, struct cmd_help_t help)
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

static gboolean
_cmd_tiny(gchar **args, struct cmd_help_t help)
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
                message_send(tiny, recipient);

                if (prefs_get_boolean(PREF_CHLOG)) {
                    const char *jid = jabber_get_jid();
                    Jid *jidp = jid_create(jid);
                    chat_log_chat(jidp->barejid, recipient, tiny, PROF_OUT_LOG, NULL);
                    jid_destroy(jidp);
                }

                ui_outgoing_msg("me", recipient, tiny);
                free(recipient);
            } else if (win_type == WIN_PRIVATE) {
                char *recipient = ui_current_recipient();
                message_send(tiny, recipient);
                ui_outgoing_msg("me", recipient, tiny);
                free(recipient);
            } else { // groupchat
                char *recipient = ui_current_recipient();
                message_send_groupchat(tiny, recipient);
                free(recipient);
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

static gboolean
_cmd_clear(gchar **args, struct cmd_help_t help)
{
    ui_clear_current();
    return TRUE;
}

static gboolean
_cmd_close(gchar **args, struct cmd_help_t help)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();
    win_type_t win_type = ui_current_win_type();

    // cannot close console window
    if (win_type == WIN_CONSOLE) {
        cons_show("Cannot close console window.");
        return TRUE;
    }

    // handle leaving rooms, or chat
    if (conn_status == JABBER_CONNECTED) {
        if (win_type == WIN_MUC) {
            char *room_jid = ui_current_recipient();
            presence_leave_chat_room(room_jid);
        } else if ((win_type == WIN_CHAT) || (win_type == WIN_PRIVATE)) {

            if (prefs_get_boolean(PREF_STATES)) {
                char *recipient = ui_current_recipient();

                // send <gone/> chat state before closing
                if (chat_session_get_recipient_supports(recipient)) {
                    chat_session_set_gone(recipient);
                    message_send_gone(recipient);
                    chat_session_end(recipient);
                }
            }
        }
    }

    // close the window
    ui_close_current();

    return TRUE;
}

static gboolean
_cmd_set_beep(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help, "Sound", PREF_BEEP);
}

static gboolean
_cmd_set_states(gchar **args, struct cmd_help_t help)
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

static gboolean
_cmd_set_titlebar(gchar **args, struct cmd_help_t help)
{
    if (strcmp(args[0], "version") != 0) {
        cons_show("Usage: %s", help.usage);
        return TRUE;
    } else {
        return _cmd_set_boolean_preference(args[1], help,
        "Show version in window title", PREF_TITLEBARVERSION);
    }
}

static gboolean
_cmd_set_outtype(gchar **args, struct cmd_help_t help)
{
    gboolean result = _cmd_set_boolean_preference(args[0], help,
        "Sending typing notifications", PREF_OUTTYPE);

    // if enabled, enable states
    if (result == TRUE && (strcmp(args[0], "on") == 0)) {
        prefs_set_boolean(PREF_STATES, TRUE);
    }

    return result;
}

static gboolean
_cmd_set_gone(gchar **args, struct cmd_help_t help)
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


static gboolean
_cmd_set_notify(gchar **args, struct cmd_help_t help)
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

static gboolean
_cmd_set_priority(gchar **args, struct cmd_help_t help)
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

static gboolean
_cmd_set_statuses(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Status notifications", PREF_STATUSES);
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
            "Version checking", PREF_VERCHECK);
    }
}

static gboolean
_cmd_set_flash(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Screen flash", PREF_FLASH);
}

static gboolean
_cmd_set_intype(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Show contact typing", PREF_INTYPE);
}

static gboolean
_cmd_set_splash(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Splash screen", PREF_SPLASH);
}

static gboolean
_cmd_set_chlog(gchar **args, struct cmd_help_t help)
{
    gboolean result = _cmd_set_boolean_preference(args[0], help,
        "Chat logging", PREF_CHLOG);

    // if set to off, disable history
    if (result == TRUE && (strcmp(args[0], "off") == 0)) {
        prefs_set_boolean(PREF_HISTORY, FALSE);
    }

    return result;
}

static gboolean
_cmd_set_mouse(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Mouse handling", PREF_MOUSE);
}

static gboolean
_cmd_set_history(gchar **args, struct cmd_help_t help)
{
    gboolean result = _cmd_set_boolean_preference(args[0], help,
        "Chat history", PREF_HISTORY);

    // if set to on, set chlog
    if (result == TRUE && (strcmp(args[0], "on") == 0)) {
        prefs_set_boolean(PREF_CHLOG, TRUE);
    }

    return result;
}

static gboolean
_cmd_away(gchar **args, struct cmd_help_t help)
{
    _update_presence(RESOURCE_AWAY, "away", args);
    return TRUE;
}

static gboolean
_cmd_online(gchar **args, struct cmd_help_t help)
{
    _update_presence(RESOURCE_ONLINE, "online", args);
    return TRUE;
}

static gboolean
_cmd_dnd(gchar **args, struct cmd_help_t help)
{
    _update_presence(RESOURCE_DND, "dnd", args);
    return TRUE;
}

static gboolean
_cmd_chat(gchar **args, struct cmd_help_t help)
{
    _update_presence(RESOURCE_CHAT, "chat", args);
    return TRUE;
}

static gboolean
_cmd_xa(gchar **args, struct cmd_help_t help)
{
    _update_presence(RESOURCE_XA, "xa", args);
    return TRUE;
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
        title_bar_set_status(contact_presence);

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

    if (strcmp(arg, "on") == 0) {
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
    Autocomplete ac)
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
        found = autocomplete_complete(ac, inp_cpy);
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
    } else if ((strncmp(input, "/notify invite ", 15) == 0) && (*size > 15)) {
        for(i = 15; i < *size; i++) {
            inp_cpy[i-15] = input[i];
        }
        inp_cpy[(*size) - 15] = '\0';
        found = prefs_autocomplete_boolean_choice(inp_cpy);
        if (found != NULL) {
            auto_msg = (char *) malloc((15 + (strlen(found) + 1)) * sizeof(char));
            strcpy(auto_msg, "/notify invite ");
            strcat(auto_msg, found);
            inp_replace_input(input, auto_msg, size);
            free(auto_msg);
            free(found);
        }
    } else if ((strncmp(input, "/notify sub ", 12) == 0) && (*size > 12)) {
        for(i = 12; i < *size; i++) {
            inp_cpy[i-12] = input[i];
        }
        inp_cpy[(*size) - 12] = '\0';
        found = prefs_autocomplete_boolean_choice(inp_cpy);
        if (found != NULL) {
            auto_msg = (char *) malloc((12 + (strlen(found) + 1)) * sizeof(char));
            strcpy(auto_msg, "/notify sub ");
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

static void
_theme_autocomplete(char *input, int *size)
{
    if ((strncmp(input, "/theme set ", 11) == 0) && (*size > 11)) {
        if (theme_load_ac == NULL) {
            theme_load_ac = autocomplete_new();
            GSList *themes = theme_list();
            while (themes != NULL) {
                autocomplete_add(theme_load_ac, strdup(themes->data));
                themes = g_slist_next(themes);
            }
            g_slist_free(themes);
            autocomplete_add(theme_load_ac, "default");
         }
        _parameter_autocomplete_with_ac(input, size, "/theme set", theme_load_ac);
    } else if ((strncmp(input, "/theme ", 7) == 0) && (*size > 7)) {
        _parameter_autocomplete_with_ac(input, size, "/theme", theme_ac);
    }
}

static void
_account_autocomplete(char *input, int *size)
{
    if ((strncmp(input, "/account set ", 13) == 0) && (*size > 13)) {
        _parameter_autocomplete(input, size, "/account set", accounts_find_all);
    } else if ((strncmp(input, "/account show ", 14) == 0) && (*size > 14)) {
        _parameter_autocomplete(input, size, "/account show", accounts_find_all);
    } else if ((strncmp(input, "/account enable ", 16) == 0) && (*size > 16)) {
        _parameter_autocomplete(input, size, "/account enable", accounts_find_all);
    } else if ((strncmp(input, "/account disable ", 17) == 0) && (*size > 17)) {
        _parameter_autocomplete(input, size, "/account disable", accounts_find_all);
    } else if ((strncmp(input, "/account rename ", 16) == 0) && (*size > 16)) {
        _parameter_autocomplete(input, size, "/account rename", accounts_find_all);
    } else if ((strncmp(input, "/account ", 9) == 0) && (*size > 9)) {
        _parameter_autocomplete_with_ac(input, size, "/account", account_ac);
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
