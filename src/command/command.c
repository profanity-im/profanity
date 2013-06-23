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
#include "jid.h"
#include "log.h"
#include "muc.h"
#include "profanity.h"
#include "tools/autocomplete.h"
#include "tools/tinyurl.h"
#include "ui/ui.h"
#include "xmpp/xmpp.h"

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

typedef char*(*autocompleter)(char*, int*);

static struct cmd_t * _cmd_get_command(const char * const command);
static void _update_presence(const resource_presence_t presence,
    const char * const show, gchar **args);
static gboolean _cmd_set_boolean_preference(gchar *arg, struct cmd_help_t help,
    const char * const display, preference_t pref);

static void _cmd_complete_parameters(char *input, int *size);

static char * _sub_autocomplete(char *input, int *size);
static char * _notify_autocomplete(char *input, int *size);
static char * _titlebar_autocomplete(char *input, int *size);
static char * _theme_autocomplete(char *input, int *size);
static char * _autoaway_autocomplete(char *input, int *size);
static char * _account_autocomplete(char *input, int *size);
static char * _who_autocomplete(char *input, int *size);
static char * _roster_autocomplete(char *input, int *size);
static char * _group_autocomplete(char *input, int *size);

static int _strtoi(char *str, int *saveptr, int min, int max);

// command prototypes
static gboolean _cmd_about(gchar **args, struct cmd_help_t help);
static gboolean _cmd_account(gchar **args, struct cmd_help_t help);
static gboolean _cmd_autoaway(gchar **args, struct cmd_help_t help);
static gboolean _cmd_autoping(gchar **args, struct cmd_help_t help);
static gboolean _cmd_away(gchar **args, struct cmd_help_t help);
static gboolean _cmd_beep(gchar **args, struct cmd_help_t help);
static gboolean _cmd_caps(gchar **args, struct cmd_help_t help);
static gboolean _cmd_chat(gchar **args, struct cmd_help_t help);
static gboolean _cmd_chlog(gchar **args, struct cmd_help_t help);
static gboolean _cmd_clear(gchar **args, struct cmd_help_t help);
static gboolean _cmd_close(gchar **args, struct cmd_help_t help);
static gboolean _cmd_connect(gchar **args, struct cmd_help_t help);
static gboolean _cmd_decline(gchar **args, struct cmd_help_t help);
static gboolean _cmd_disco(gchar **args, struct cmd_help_t help);
static gboolean _cmd_disconnect(gchar **args, struct cmd_help_t help);
static gboolean _cmd_dnd(gchar **args, struct cmd_help_t help);
static gboolean _cmd_duck(gchar **args, struct cmd_help_t help);
static gboolean _cmd_flash(gchar **args, struct cmd_help_t help);
static gboolean _cmd_gone(gchar **args, struct cmd_help_t help);
static gboolean _cmd_grlog(gchar **args, struct cmd_help_t help);
static gboolean _cmd_group(gchar **args, struct cmd_help_t help);
static gboolean _cmd_help(gchar **args, struct cmd_help_t help);
static gboolean _cmd_history(gchar **args, struct cmd_help_t help);
static gboolean _cmd_info(gchar **args, struct cmd_help_t help);
static gboolean _cmd_intype(gchar **args, struct cmd_help_t help);
static gboolean _cmd_invite(gchar **args, struct cmd_help_t help);
static gboolean _cmd_invites(gchar **args, struct cmd_help_t help);
static gboolean _cmd_join(gchar **args, struct cmd_help_t help);
static gboolean _cmd_leave(gchar **args, struct cmd_help_t help);
static gboolean _cmd_log(gchar **args, struct cmd_help_t help);
static gboolean _cmd_mouse(gchar **args, struct cmd_help_t help);
static gboolean _cmd_msg(gchar **args, struct cmd_help_t help);
static gboolean _cmd_nick(gchar **args, struct cmd_help_t help);
static gboolean _cmd_notify(gchar **args, struct cmd_help_t help);
static gboolean _cmd_online(gchar **args, struct cmd_help_t help);
static gboolean _cmd_outtype(gchar **args, struct cmd_help_t help);
static gboolean _cmd_prefs(gchar **args, struct cmd_help_t help);
static gboolean _cmd_priority(gchar **args, struct cmd_help_t help);
static gboolean _cmd_quit(gchar **args, struct cmd_help_t help);
static gboolean _cmd_reconnect(gchar **args, struct cmd_help_t help);
static gboolean _cmd_rooms(gchar **args, struct cmd_help_t help);
static gboolean _cmd_roster(gchar **args, struct cmd_help_t help);
static gboolean _cmd_software(gchar **args, struct cmd_help_t help);
static gboolean _cmd_splash(gchar **args, struct cmd_help_t help);
static gboolean _cmd_states(gchar **args, struct cmd_help_t help);
static gboolean _cmd_status(gchar **args, struct cmd_help_t help);
static gboolean _cmd_statuses(gchar **args, struct cmd_help_t help);
static gboolean _cmd_sub(gchar **args, struct cmd_help_t help);
static gboolean _cmd_theme(gchar **args, struct cmd_help_t help);
static gboolean _cmd_tiny(gchar **args, struct cmd_help_t help);
static gboolean _cmd_titlebar(gchar **args, struct cmd_help_t help);
static gboolean _cmd_vercheck(gchar **args, struct cmd_help_t help);
static gboolean _cmd_who(gchar **args, struct cmd_help_t help);
static gboolean _cmd_wins(gchar **args, struct cmd_help_t help);
static gboolean _cmd_xa(gchar **args, struct cmd_help_t help);

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

    { "/roster",
        _cmd_roster, parse_args_with_freetext, 0, 3,
        { "/roster [add|remove|nick] [jid] [handle]", "Manage your roster.",
        { "/roster [add|remove|nick] [jid] [handle]",
          "----------------------------------------",
          "View, add to, and remove from your roster.",
          "Passing no arguments lists all contacts in your roster.",
          "The 'add' command will add a new item, the jid is required, the handle is an optional nickname",
          "The 'remove' command removes a contact, the jid is required.",
          "The 'nick' command changes a contacts nickname, the jid is required,",
          "if no handle is supplied, the current one is removed.",
          "",
          "Example : /roster (show your roster)",
          "Example : /roster add someone@contacts.org (add the contact)",
          "Example : /roster add someone@contacts.org Buddy (add the contact with nickname 'Buddy')",
          "Example : /roster remove someone@contacts.org (remove the contact)",
          "Example : /roster nick myfriend@chat.org My Friend",
          "Example : /roster nick kai@server.com (clears handle)",
          NULL } } },

    { "/group",
        _cmd_group, parse_args_with_freetext, 0, 3,
        { "/group show|add|remove [group] [contact]", "Manage roster groups.",
        { "/group show|add|remove [group] [contact]",
          "-------------------------------------",
          "View, add to, and remove from roster groups.",
          "Passing no argument will list all roster groups.",
          "The 'show' command takes 'group' as an argument, and lists all roster items in that group.",
          "The 'add' command takes 'group' and 'contact' arguments, and add the contact to the group.",
          "The 'remove' command takes 'group' and 'contact' arguments and removed the contact from the group,",
          "",
          "Example : /group",
          "Example : /group show friends",
          "Example : /group add friends newfriend@server.org",
          "Example : /group add family brother (using contacts nickname)",
          "Example : /group remove colleagues boss@work.com",
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
        { "/status [jid|nick]", "Find out your contacts presence information.",
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

    { "/leave",
        _cmd_leave, parse_args, 0, 0,
        { "/leave", "Leave a chat room.",
        { "/leave",
          "------",
          "Leave the current chat room.",
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
        _cmd_wins, parse_args, 0, 1,
        { "/wins [tidy|prune]", "List or tidy active windows.",
        { "/wins [tidy|prune]",
          "------------------",
          "Passing no argument will list all currently active windows and information about their usage.",
          "tidy  : Shuffle windows so there are no gaps between used windows.",
          "prune : Close all windows with no unread messages, and then tidy as above.",
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

    { "/duck",
        _cmd_duck, parse_args_with_freetext, 1, 1,
        { "/duck query", "Perform search using DuckDuckGo chatbot.",
        { "/duck query",
          "-----------",
          "Send a search query to the DuckDuckGo chatbot.",
          "Your chat service must be federated, i.e. allow message to be sent/received outside of its domain.",
          "",
          "Example : /duck dennis ritchie",
          NULL } } },

    { "/who",
        _cmd_who, parse_args, 0, 2,
        { "/who [status] [group]", "Show contacts/room participants with chosen status.",
        { "/who [status] [group]",
          "---------------------",
          "Show contacts with the specified status, no status shows all contacts.",
          "Possible statuses are: online, offline, away, dnd, xa, chat, available, unavailable.",
          "The groups argument will show only contacts in that group.",
          "If in a chat room, the participants with the supplied status are displayed.",
          "",
          "online      : Contacts that are connected, i.e. online, chat, away, xa, dnd",
          "available   : Contacts that are available for chat, i.e. online, chat.",
          "unavailable : Contacts that are not available for chat, i.e. offline, away, xa, dnd.",
          "any         : Contacts with any status (same as calling with no argument.",
          NULL } } },

    { "/close",
        _cmd_close, parse_args, 0, 1,
        { "/close [win|read|all]", "Close a window window.",
        { "/close [win|read|all]",
          "---------------------",
          "Passing no argument will close the current window.",
          "2,3,4,5,6,7,8,9 or 0 : Close the specified window.",
          "all                  : Close all currently open windows.",
          "read                 : Close all windows that have no new messages.",
          "The console window cannot be closed.",
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
        _cmd_beep, parse_args, 1, 1,
        { "/beep on|off", "Terminal beep on new messages.",
        { "/beep on|off",
          "------------",
          "Switch the terminal bell on or off.",
          "The bell will sound when incoming messages are received.",
          "If the terminal does not support sounds, it may attempt to flash the screen instead.",
          NULL } } },

    { "/notify",
        _cmd_notify, parse_args, 2, 2,
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
        _cmd_flash, parse_args, 1, 1,
        { "/flash on|off", "Terminal flash on new messages.",
        { "/flash on|off",
          "-------------",
          "Make the terminal flash when incoming messages are recieved.",
          "The flash will only occur if you are not in the chat window associated with the user sending the message.",
          "If the terminal doesn't support flashing, it may attempt to beep.",
          NULL } } },

    { "/intype",
        _cmd_intype, parse_args, 1, 1,
        { "/intype on|off", "Show when contact is typing.",
        { "/intype on|off",
          "--------------",
          "Show when a contact is typing in the console, and in active message window.",
          NULL } } },

    { "/splash",
        _cmd_splash, parse_args, 1, 1,
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
        _cmd_titlebar, parse_args, 2, 2,
        { "/titlebar property on|off", "Show various properties in the window title bar.",
        { "/titlebar property on|off",
          "-------------------------",
          "Show various properties in the window title bar.",
          "Currently The only supported property is 'version'.",
          NULL  } } },

    { "/mouse",
        _cmd_mouse, parse_args, 1, 1,
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
        _cmd_chlog, parse_args, 1, 1,
        { "/chlog on|off", "Chat logging to file",
        { "/chlog on|off",
          "-------------",
          "Switch chat logging on or off.",
          "This setting will be enabled if /history is set to on.",
          "When disabling this option, /history will also be disabled.",
          "See the /grlog setting for enabling logging of chat room (groupchat) messages.",
          NULL } } },

    { "/grlog",
        _cmd_grlog, parse_args, 1, 1,
        { "/grlog on|off", "Chat logging of chat rooms to file",
        { "/grlog on|off",
          "-------------",
          "Switch chat room logging on or off.",
          "See the /chlog setting for enabling logging of one to one chat.",
          NULL } } },

    { "/states",
        _cmd_states, parse_args, 1, 1,
        { "/states on|off", "Send chat states during a chat session.",
        { "/states on|off",
          "--------------",
          "Sending of chat state notifications during chat sessions.",
          "Such as whether you have become inactive, or have closed the chat window.",
          NULL } } },

    { "/outtype",
        _cmd_outtype, parse_args, 1, 1,
        { "/outtype on|off", "Send typing notification to recipient.",
        { "/outtype on|off",
          "---------------",
          "Send an indication that you are typing to the chat recipient.",
          "Chat states (/states) will be enabled if this setting is set.",
          NULL } } },

    { "/gone",
        _cmd_gone, parse_args, 1, 1,
        { "/gone minutes", "Send 'gone' state to recipient after a period.",
        { "/gone minutes",
          "-------------",
          "Send a 'gone' state to the recipient after the specified number of minutes."
          "This indicates to the recipient's client that you have left the conversation.",
          "A value of 0 will disable sending this chat state.",
          "Chat states (/states) will be enabled if this setting is set.",
          NULL } } },

    { "/history",
        _cmd_history, parse_args, 1, 1,
        { "/history on|off", "Chat history in message windows.",
        { "/history on|off",
          "---------------",
          "Switch chat history on or off, requires /chlog will automatically be enabled when this setting in on.",
          "When history is enabled, previous messages are shown in chat windows.",
          NULL } } },

    { "/log",
        _cmd_log, parse_args, 2, 2,
        { "/log maxsize value", "Manage system logging settings.",
        { "/log maxsize value",
          "------------------",
          "maxsize : When log file size exceeds this value it will be automatically",
          "          rotated (file will be renamed). Default value is 1048580 (1MB)",
          NULL } } },

    { "/reconnect",
        _cmd_reconnect, parse_args, 1, 1,
        { "/reconnect seconds", "Set reconnect interval.",
        { "/reconnect seconds",
          "------------------",
          "Set the reconnect attempt interval in seconds for when the connection is lost.",
          "A value of 0 will switch of reconnect attempts.",
          NULL } } },

    { "/autoping",
        _cmd_autoping, parse_args, 1, 1,
        { "/autoping seconds", "Server ping interval.",
        { "/autoping seconds",
          "-----------------",
          "Set the number of seconds between server pings, so ensure connection kept alive.",
          "A value of 0 will switch off autopinging the server.",
          NULL } } },

    { "/autoaway",
        _cmd_autoaway, parse_args_with_freetext, 2, 2,
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
        _cmd_priority, parse_args, 1, 1,
        { "/priority value", "Set priority for the current account.",
        { "/priority value",
          "---------------",
          "Set priority for the current account, presence will be sent when calling this command.",
          "See the /account command for more specific priority settings per presence status.",
          "value : Number between -128 and 127. Default value is 0.",
          NULL } } },

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


    { "/statuses",
        _cmd_statuses, parse_args, 1, 1,
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
static Autocomplete close_ac;
static Autocomplete wins_ac;
static Autocomplete roster_ac;
static Autocomplete group_ac;

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

    close_ac = autocomplete_new();
    autocomplete_add(close_ac, strdup("read"));
    autocomplete_add(close_ac, strdup("all"));

    wins_ac = autocomplete_new();
    autocomplete_add(wins_ac, strdup("prune"));
    autocomplete_add(wins_ac, strdup("tidy"));

    roster_ac = autocomplete_new();
    autocomplete_add(roster_ac, strdup("add"));
    autocomplete_add(roster_ac, strdup("nick"));
    autocomplete_add(roster_ac, strdup("remove"));

    group_ac = autocomplete_new();
    autocomplete_add(group_ac, strdup("show"));
    autocomplete_add(group_ac, strdup("add"));
    autocomplete_add(group_ac, strdup("remove"));

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
    autocomplete_add(who_ac, strdup("any"));

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
    autocomplete_free(close_ac);
    autocomplete_free(wins_ac);
    autocomplete_free(roster_ac);
    autocomplete_free(group_ac);
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
    roster_reset_search_attempts();
    muc_reset_invites_ac();
    accounts_reset_all_search();
    accounts_reset_enabled_search();
    prefs_reset_boolean_choice();
    presence_reset_sub_request_search();
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
    autocomplete_reset(close_ac);
    autocomplete_reset(wins_ac);
    autocomplete_reset(roster_ac);
    autocomplete_reset(group_ac);
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
                    const char *jid = jabber_get_fulljid();
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

        case WIN_DUCK:
            if (status != JABBER_CONNECTED) {
                ui_current_print_line("You are not currently connected.");
            } else {
                message_send_duck(inp);
                ui_duck(inp);
                free(recipient);
            }
            break;

        default:
            break;
    }

    return TRUE;
}

static void
_cmd_complete_parameters(char *input, int *size)
{
    int i;
    char *result = NULL;

    // autocomplete boolean settings
    gchar *boolean_choices[] = { "/beep", "/intype", "/states", "/outtype",
        "/flash", "/splash", "/chlog", "/grlog", "/mouse", "/history",
        "/vercheck", "/statuses" };

    for (i = 0; i < ARRAY_SIZE(boolean_choices); i++) {
        result = autocomplete_param_with_func(input, size, boolean_choices[i],
            prefs_autocomplete_boolean_choice);
        if (result != NULL) {
            inp_replace_input(input, result, size);
            g_free(result);
            return;
        }
    }

    // autocomplete nickname in chat rooms
    if (ui_current_win_type() == WIN_MUC) {
        Autocomplete nick_ac = muc_get_roster_ac(ui_current_recipient());
        if (nick_ac != NULL) {
            gchar *nick_choices[] = { "/msg", "/info", "/caps", "/status", "/software" } ;

            for (i = 0; i < ARRAY_SIZE(nick_choices); i++) {
                result = autocomplete_param_with_ac(input, size, nick_choices[i],
                    nick_ac);
                if (result != NULL) {
                    inp_replace_input(input, result, size);
                    g_free(result);
                    return;
                }
            }
        }

    // otherwise autocomple using roster
    } else {
        gchar *contact_choices[] = { "/msg", "/info", "/status" };
        for (i = 0; i < ARRAY_SIZE(contact_choices); i++) {
            result = autocomplete_param_with_func(input, size, contact_choices[i],
                roster_find_contact);
            if (result != NULL) {
                inp_replace_input(input, result, size);
                g_free(result);
                return;
            }
        }

        gchar *resource_choices[] = { "/caps", "/software" };
        for (i = 0; i < ARRAY_SIZE(resource_choices); i++) {
            result = autocomplete_param_with_func(input, size, resource_choices[i],
                roster_find_resource);
            if (result != NULL) {
                inp_replace_input(input, result, size);
                g_free(result);
                return;
            }
        }
    }

    result = autocomplete_param_with_func(input, size, "/invite", roster_find_contact);
    if (result != NULL) {
        inp_replace_input(input, result, size);
        g_free(result);
        return;
    }

    gchar *invite_choices[] = { "/decline", "/join" };
    for (i = 0; i < ARRAY_SIZE(invite_choices); i++) {
        result = autocomplete_param_with_func(input, size, invite_choices[i],
            muc_find_invite);
        if (result != NULL) {
            inp_replace_input(input, result, size);
            g_free(result);
            return;
        }
    }

    result = autocomplete_param_with_func(input, size, "/connect", accounts_find_enabled);
    if (result != NULL) {
        inp_replace_input(input, result, size);
        g_free(result);
        return;
    }

    gchar *commands[] = { "/help", "/prefs", "/log", "/disco", "/close", "/wins" };
    Autocomplete completers[] = { help_ac, prefs_ac, log_ac, disco_ac, close_ac, wins_ac };

    for (i = 0; i < ARRAY_SIZE(commands); i++) {
        result = autocomplete_param_with_ac(input, size, commands[i], completers[i]);
        if (result != NULL) {
            inp_replace_input(input, result, size);
            g_free(result);
            return;
        }
    }

    autocompleter acs[] = { _who_autocomplete, _sub_autocomplete, _notify_autocomplete,
        _autoaway_autocomplete, _titlebar_autocomplete, _theme_autocomplete,
        _account_autocomplete, _roster_autocomplete, _group_autocomplete };

    for (i = 0; i < ARRAY_SIZE(acs); i++) {
        result = acs[i](input, size);
        if (result != NULL) {
            inp_replace_input(input, result, size);
            g_free(result);
            return;
        }
    }

    return;
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
        PContact contact = roster_get_contact(bare_jid);
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
        char *jid = strdup(jabber_get_fulljid());
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
    if (args[0] == NULL) {
        cons_show_wins();
    } else if (strcmp(args[0], "tidy") == 0) {
        ui_tidy_wins();
    } else if (strcmp(args[0], "prune") == 0) {
        ui_prune_wins();
    }
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
        char *usr_jid = roster_barejid_from_name(usr);
        if (usr_jid == NULL) {
            usr_jid = usr;
        }
        if (msg != NULL) {
            message_send(msg, usr_jid);
            ui_outgoing_msg("me", usr_jid, msg);

            if (((win_type == WIN_CHAT) || (win_type == WIN_CONSOLE)) && prefs_get_boolean(PREF_CHLOG)) {
                const char *jid = jabber_get_fulljid();
                Jid *jidp = jid_create(jid);
                chat_log_chat(jidp->barejid, usr_jid, msg, PROF_OUT_LOG, NULL);
                jid_destroy(jidp);
            }

            return TRUE;
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
            return TRUE;
        }
    }
}

static gboolean
_cmd_group(gchar **args, struct cmd_help_t help)
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

        roster_add_to_group(group, barejid);

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

        roster_remove_from_group(group, barejid);

        return TRUE;
    }

    cons_show("Usage: %s", help.usage);
    return TRUE;
}

static gboolean
_cmd_roster(gchar **args, struct cmd_help_t help)
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
    }

    // add contact
    if (strcmp(args[0], "add") == 0) {

        if (args[1] == NULL) {
            cons_show("Usage: %s", help.usage);
            return TRUE;
        }

        char *jid = args[1];
        char *name = args[2];

        roster_add_new(jid, name);

        return TRUE;
    }

    // remove contact
    if (strcmp(args[0], "remove") == 0) {

        if (args[1] == NULL) {
            cons_show("Usage: %s", help.usage);
            return TRUE;
        }

        char *jid = args[1];

        roster_remove(jid);

        return TRUE;
    }

    // change nickname
    if (strcmp(args[0], "nick") == 0) {

        if (args[1] == NULL) {
            cons_show("Usage: %s", help.usage);
            return TRUE;
        }

        char *jid = args[1];
        char *name = args[2];

        // contact does not exist
        PContact contact = roster_get_contact(jid);
        if (contact == NULL) {
            cons_show("Contact not found in roster: %s", jid);
            return TRUE;
        }

        roster_change_name(jid, name);

        if (name == NULL) {
            cons_show("Nickname for %s removed.", jid);
        } else {
            cons_show("Nickname for %s set to: %s.", jid, name);
        }

        return TRUE;
    }

    cons_show("Usage: %s", help.usage);
    return TRUE;
}

static gboolean
_cmd_duck(gchar **args, struct cmd_help_t help)
{
    char *query = args[0];

    jabber_conn_status_t conn_status = jabber_get_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (ui_windows_full()) {
        cons_show_error("Windows all used, close a window and try again.");
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

static gboolean
_cmd_status(gchar **args, struct cmd_help_t help)
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

static gboolean
_cmd_info(gchar **args, struct cmd_help_t help)
{
    char *usr = args[0];
    char *usr_jid = NULL;

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
                pcontact = roster_get_contact(ui_current_recipient());
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
    Jid *my_jid = jid_create(jabber_get_fulljid());

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
        Jid *jid = jid_create(jabber_get_fulljid());
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
        Jid *jidp = jid_create(jabber_get_fulljid());
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
                    const char *jid = jabber_get_fulljid();
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
    int index = 0;
    int curr = 0;
    int count = 0;

    if (args[0] == NULL) {
        index = ui_current_win_index();
    } else if (strcmp(args[0], "all") == 0) {
        for (curr = 1; curr <= 9; curr++) {
            if (ui_win_exists(curr)) {
                if (conn_status == JABBER_CONNECTED) {
                    ui_close_connected_win(curr);
                }
                ui_close_win(curr);
                count++;
            }
        }
        if (count == 0) {
            cons_show("No windows to close.");
        } else if (count == 1) {
            cons_show("Closed 1 window.");
        } else {
            cons_show("Closed %d windows.", count);
        }
        return TRUE;
    } else if (strcmp(args[0], "read") == 0) {
        for (curr = 1; curr <= 9; curr++) {
            if (ui_win_exists(curr) && (ui_win_unread(curr) == 0)) {
                if (conn_status == JABBER_CONNECTED) {
                    ui_close_connected_win(curr);
                }
                ui_close_win(curr);
                count++;
            }
        }
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
        if (index == 0) {
            index = 9;
        } else if (index != 10) {
            index--;
        }
    }

    if (index == 0) {
        cons_show("Cannot close console window.");
        return TRUE;
    }

    if (index > 9 || index < 0) {
        cons_show("No such window exists.");
        return TRUE;
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
    int ui_index = index + 1;
    if (ui_index == 10) {
        ui_index = 0;
    }
    cons_show("Closed window %d", ui_index);

    return TRUE;
}

static gboolean
_cmd_leave(gchar **args, struct cmd_help_t help)
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

static gboolean
_cmd_beep(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help, "Sound", PREF_BEEP);
}

static gboolean
_cmd_states(gchar **args, struct cmd_help_t help)
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
_cmd_titlebar(gchar **args, struct cmd_help_t help)
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
_cmd_outtype(gchar **args, struct cmd_help_t help)
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
_cmd_gone(gchar **args, struct cmd_help_t help)
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
_cmd_notify(gchar **args, struct cmd_help_t help)
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
_cmd_log(gchar **args, struct cmd_help_t help)
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
_cmd_reconnect(gchar **args, struct cmd_help_t help)
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
_cmd_autoping(gchar **args, struct cmd_help_t help)
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
_cmd_autoaway(gchar **args, struct cmd_help_t help)
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
_cmd_priority(gchar **args, struct cmd_help_t help)
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
_cmd_statuses(gchar **args, struct cmd_help_t help)
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
_cmd_flash(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Screen flash", PREF_FLASH);
}

static gboolean
_cmd_intype(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Show contact typing", PREF_INTYPE);
}

static gboolean
_cmd_splash(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Splash screen", PREF_SPLASH);
}

static gboolean
_cmd_chlog(gchar **args, struct cmd_help_t help)
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
_cmd_grlog(gchar **args, struct cmd_help_t help)
{
    gboolean result = _cmd_set_boolean_preference(args[0], help,
        "Groupchat logging", PREF_GRLOG);

    return result;
}

static gboolean
_cmd_mouse(gchar **args, struct cmd_help_t help)
{
    return _cmd_set_boolean_preference(args[0], help,
        "Mouse handling", PREF_MOUSE);
}

static gboolean
_cmd_history(gchar **args, struct cmd_help_t help)
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

static char *
_sub_autocomplete(char *input, int *size)
{
    char *result = NULL;
    result = autocomplete_param_with_func(input, size, "/sub allow", presence_sub_request_find);
    if (result != NULL) {
        return result;
    }
    result = autocomplete_param_with_func(input, size, "/sub deny", presence_sub_request_find);
    if (result != NULL) {
        return result;
    }
    result = autocomplete_param_with_ac(input, size, "/sub", sub_ac);
    if (result != NULL) {
        return result;
    }

    return NULL;
}

static char *
_who_autocomplete(char *input, int *size)
{
    int i = 0;
    char *result = NULL;
    gchar *group_commands[] = { "/who any", "/who online", "/who offline",
        "/who chat", "/who away", "/who xa", "/who dnd", "/who available",
        "/who unavailable" };

    for (i = 0; i < ARRAY_SIZE(group_commands); i++) {
        result = autocomplete_param_with_func(input, size, group_commands[i], roster_find_group);
        if (result != NULL) {
            return result;
        }
    }

    result = autocomplete_param_with_ac(input, size, "/who", who_ac);
    if (result != NULL) {
        return result;
    }

    return NULL;
}

static char *
_roster_autocomplete(char *input, int *size)
{
    char *result = NULL;
    result = autocomplete_param_with_func(input, size, "/roster nick", roster_find_jid);
    if (result != NULL) {
        return result;
    }
    result = autocomplete_param_with_func(input, size, "/roster remove", roster_find_jid);
    if (result != NULL) {
        return result;
    }
    result = autocomplete_param_with_ac(input, size, "/roster", roster_ac);
    if (result != NULL) {
        return result;
    }

    return NULL;
}

static char *
_group_autocomplete(char *input, int *size)
{
    char *result = NULL;
    result = autocomplete_param_with_func(input, size, "/group show", roster_find_group);
    if (result != NULL) {
        return result;
    }

    result = autocomplete_param_no_with_func(input, size, "/group add", 4, roster_find_contact);
    if (result != NULL) {
        return result;
    }
    result = autocomplete_param_no_with_func(input, size, "/group remove", 4, roster_find_contact);
    if (result != NULL) {
        return result;
    }
    result = autocomplete_param_with_func(input, size, "/group add", roster_find_group);
    if (result != NULL) {
        return result;
    }
    result = autocomplete_param_with_func(input, size, "/group remove", roster_find_group);
    if (result != NULL) {
        return result;
    }
    result = autocomplete_param_with_ac(input, size, "/group", group_ac);
    if (result != NULL) {
        return result;
    }

    return NULL;
}

static char *
_notify_autocomplete(char *input, int *size)
{
    int i = 0;
    char *result = NULL;

    gchar *boolean_choices[] = { "/notify message", "/notify typing",
        "/notify invite", "/notify sub" };
    for (i = 0; i < ARRAY_SIZE(boolean_choices); i++) {
        result = autocomplete_param_with_func(input, size, boolean_choices[i],
            prefs_autocomplete_boolean_choice);
        if (result != NULL) {
            return result;
        }
    }

    result = autocomplete_param_with_ac(input, size, "/notify", notify_ac);
    if (result != NULL) {
        return result;
    }

    return NULL;
}

static char *
_titlebar_autocomplete(char *input, int *size)
{
    char *result = NULL;
    result = autocomplete_param_with_func(input, size, "/titlebar version",
        prefs_autocomplete_boolean_choice);
    if (result != NULL) {
        return result;
    }
    result = autocomplete_param_with_ac(input, size, "/titlebar", titlebar_ac);
    if (result != NULL) {
        return result;
    }

    return NULL;
}

static char *
_autoaway_autocomplete(char *input, int *size)
{
    char *result = NULL;

    result = autocomplete_param_with_ac(input, size, "/autoaway mode", autoaway_mode_ac);
    if (result != NULL) {
        return result;
    }
    result = autocomplete_param_with_func(input, size, "/autoaway check",
        prefs_autocomplete_boolean_choice);
    if (result != NULL) {
        return result;
    }
    result = autocomplete_param_with_ac(input, size, "/autoaway", autoaway_ac);
    if (result != NULL) {
        return result;
    }

    return NULL;
}

static char *
_theme_autocomplete(char *input, int *size)
{
    char *result = NULL;
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
        result = autocomplete_param_with_ac(input, size, "/theme set", theme_load_ac);
        if (result != NULL) {
            return result;
        }
    }
    result = autocomplete_param_with_ac(input, size, "/theme", theme_ac);
    if (result != NULL) {
        return result;
    }

    return NULL;
}

static char *
_account_autocomplete(char *input, int *size)
{
    char *result = NULL;
    int i = 0;
    gchar *account_choice[] = { "/account set", "/account show", "/account enable",
        "/account disable", "/account rename" };

    for (i = 0; i < ARRAY_SIZE(account_choice); i++) {
        result = autocomplete_param_with_func(input, size, account_choice[i],
            accounts_find_all);
        if (result != NULL) {
            return result;
        }
    }

    result = autocomplete_param_with_ac(input, size, "/account", account_ac);
    if (result != NULL) {
        return result;
    }

    return NULL;
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
