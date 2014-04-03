/*
 * command.c
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

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "chat_session.h"
#include "command/command.h"
#include "command/commands.h"
#include "command/history.h"
#include "common.h"
#include "config/accounts.h"
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
#include "xmpp/xmpp.h"
#include "xmpp/bookmark.h"

typedef char*(*autocompleter)(char*, int*);

static void _cmd_complete_parameters(char *input, int *size);

static char * _sub_autocomplete(char *input, int *size);
static char * _notify_autocomplete(char *input, int *size);
static char * _theme_autocomplete(char *input, int *size);
static char * _autoaway_autocomplete(char *input, int *size);
static char * _autoconnect_autocomplete(char *input, int *size);
static char * _account_autocomplete(char *input, int *size);
static char * _who_autocomplete(char *input, int *size);
static char * _roster_autocomplete(char *input, int *size);
static char * _group_autocomplete(char *input, int *size);
static char * _bookmark_autocomplete(char *input, int *size);
static char * _otr_autocomplete(char *input, int *size);
static char * _connect_autocomplete(char *input, int *size);
static char * _statuses_autocomplete(char *input, int *size);
static char * _alias_autocomplete(char *input, int *size);
static char * _join_autocomplete(char *input, int *size);

GHashTable *commands = NULL;

/*
 * Command list
 */
static struct cmd_t command_defs[] =
{
    { "/help",
        cmd_help, parse_args, 0, 1, NULL,
        { "/help [area|command]", "Get help on using Profanity.",
        { "/help [area|command]",
          "-------------------------",
          "Use with no arguments to get a help summary.",
          "Supply an area to see help for commands related to specific features.",
          "Supply a command (without the leading slash) to see help for that command.",
          "",
          "Example : /help commands",
          "Example : /help presence",
          "Example : /help who",
          "",
          "For more detailed help, see the user guide at http://www.profanity.im/userguide.html.",
          NULL } } },

    { "/about",
        cmd_about, parse_args, 0, 0, NULL,
        { "/about", "About Profanity.",
        { "/about",
          "------",
          "Show version and license information.",
          NULL  } } },

    { "/connect",
        cmd_connect, parse_args, 1, 5, NULL,
        { "/connect account [server value] [port value]", "Login to a chat service.",
        { "/connect account [server value] [port value]",
          "--------------------------------------------",
          "Connect to an XMPP service using the specified account.",
          "Use the server property to specify a server if required.",
          "Change the default port (5222, or 5223 for SSL) with the port property.",
          "An account is automatically created if one does not exist.",
          "See the /account command for more details.",
          "",
          "Example: /connect myuser@gmail.com",
          "Example: /connect myuser@mycompany.com server talk.google.com",
          "Example: /connect bob@someplace port 5678",
          "Example: /connect me@chatty server chatty.com port 5443",
          NULL  } } },

    { "/disconnect",
        cmd_disconnect, parse_args, 0, 0, NULL,
        { "/disconnect", "Logout of current session.",
        { "/disconnect",
          "-----------",
          "Disconnect from the current chat service.",
          NULL  } } },

    { "/msg",
        cmd_msg, parse_args_with_freetext, 1, 2, NULL,
        { "/msg contact|nick [message]", "Start chat with user.",
        { "/msg contact|nick [message]",
          "---------------------------",
          "Open a chat window for the contact and send the message if one is supplied.",
          "When in a chat room, supply a nickname to start private chat with a room member.",
          "Use quotes if the nickname includes spaces.",
          "",
          "Example : /msg myfriend@server.com Hey, here's a message!",
          "Example : /msg otherfriend@server.com",
          "Example : /msg Bob Here is a private message",
          "Example : /msg \"My Friend\" Hi, how are you?",
          NULL } } },

    { "/roster",
        cmd_roster, parse_args_with_freetext, 0, 3, NULL,
        { "/roster [add|remove|nick|clearnick] [jid] [nickname]", "Manage your roster.",
        { "/roster [add|remove|nick|clearnick] [jid] [nickname]",
          "----------------------------------------------------",
          "View, add to, and remove from your roster.",
          "Passing no arguments lists all contacts in your roster.",
          "The 'add' command will add a new item, jid is required, nickname is optional.",
          "The 'remove' command removes a contact, jid is required.",
          "The 'nick' command changes a contacts nickname, both jid and nickname are required,",
          "The 'clearnick' command removes the current nickname, jid is required.",
          "",
          "Example : /roster (show your roster)",
          "Example : /roster add someone@contacts.org (add the contact)",
          "Example : /roster add someone@contacts.org Buddy (add the contact with nickname 'Buddy')",
          "Example : /roster remove someone@contacts.org (remove the contact)",
          "Example : /roster nick myfriend@chat.org My Friend",
          "Example : /roster clearnick kai@server.com (clears nickname)",
          NULL } } },

    { "/group",
        cmd_group, parse_args_with_freetext, 0, 3, NULL,
        { "/group [show|add|remove] [group] [contact]", "Manage roster groups.",
        { "/group [show|add|remove] [group] [contact]",
          "------------------------------------------",
          "View, add to, and remove from roster groups.",
          "Passing no argument will list all roster groups.",
          "The 'show' command takes 'group' as an argument, and lists all roster items in that group.",
          "The 'add' command takes 'group' and 'contact' arguments, and adds the contact to the group.",
          "The 'remove' command takes 'group' and 'contact' arguments and removes the contact from the group,",
          "",
          "Example : /group",
          "Example : /group show friends",
          "Example : /group add friends newfriend@server.org",
          "Example : /group add family Brother (using contacts nickname)",
          "Example : /group remove colleagues boss@work.com",
          NULL } } },

    { "/info",
        cmd_info, parse_args, 0, 1, NULL,
        { "/info [contact|nick]", "Show basic information about a contact, or room member.",
        { "/info [contact|nick]",
          "--------------------",
          "Show information including current subscription status and summary information for each connected resource.",
          "If in a chat window the parameter is not required, the current recipient will be used.",
          "",
          "Example : /info mybuddy@chat.server.org",
          "Example : /info kai",
          NULL } } },

    { "/caps",
        cmd_caps, parse_args, 0, 1, NULL,
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
        cmd_software, parse_args, 0, 1, NULL,
        { "/software [fulljid|nick]", "Find out software version information about a contacts resource.",
        { "/software [fulljid|nick]",
          "------------------------",
          "Find out a contact, or room members software version information, if such requests are supported.",
          "If in the console window or a regular chat window, a full JID is required.",
          "If in a chat room, the nickname is required.",
          "If in private chat, no parameter is required.",
          "If the contact's software does not support software version requests, nothing will be displayed.",
          "",
          "Example : /software mybuddy@chat.server.org/laptop (contact's laptop resource)",
          "Example : /software mybuddy@chat.server.org/phone (contact's phone resource)",
          "Example : /software bruce (room member)",
          NULL } } },

    { "/status",
        cmd_status, parse_args, 0, 1, NULL,
        { "/status [contact|nick]", "Find out a contacts presence information.",
        { "/status [contact|nick]",
          "----------------------",
          "Find out a contact, or room members presence information.",
          "If in a chat window the parameter is not required, the current recipient will be used.",
          "",
          "Example : /status buddy@server.com",
          "Example : /status jon",
          NULL } } },

    { "/join",
        cmd_join, parse_args, 1, 5, NULL,
        { "/join room[@server] [nick value] [password value]", "Join a chat room.",
        { "/join room[@server] [nick value] [password value]",
          "-------------------------------------------------",
          "Join a chat room at the conference server.",
          "If nick is specified you will join with this nickname.",
          "Otherwise the account preference 'muc.nick' will be used which by default is the localpart of your JID (before the @).",
          "If no server is supplied, the account preference 'muc.service' is used, which is 'conference.<domain-part>' by default.",
          "If the room doesn't exist, and the server allows it, a new one will be created.",
          "",
          "Example : /join jdev@conference.jabber.org",
          "Example : /join jdev@conference.jabber.org nick mynick",
          "Example : /join private@conference.jabber.org nick mynick password mypassword",
          "Example : /join jdev (as user@jabber.org will join jdev@conference.jabber.org)",
          NULL } } },

    { "/leave",
        cmd_leave, parse_args, 0, 0, NULL,
        { "/leave", "Leave a chat room.",
        { "/leave",
          "------",
          "Leave the current chat room.",
          NULL } } },

    { "/invite",
        cmd_invite, parse_args_with_freetext, 1, 2, NULL,
        { "/invite contact [message]", "Invite contact to chat room.",
        { "/invite contact [message]",
          "-------------------------",
          "Send a direct invite to the specified contact to the current chat room.",
          "If a message is supplied it will be sent as the reason for the invite.",
          NULL } } },

    { "/invites",
        cmd_invites, parse_args_with_freetext, 0, 0, NULL,
        { "/invites", "Show outstanding chat room invites.",
        { "/invites",
          "--------",
          "Show all rooms that you have been invited to, and have not yet been accepted or declind.",
          "Use \"/join <room>\" to accept a room invitation.",
          "Use \"/decline <room>\" to decline a room invitation.",
          NULL } } },

    { "/decline",
        cmd_decline, parse_args_with_freetext, 1, 1, NULL,
        { "/decline room", "Decline a chat room invite.",
        { "/decline room",
          "-------------",
          "Decline invitation to a chat room, the room will no longer be in the list of outstanding invites.",
          NULL } } },

    { "/rooms",
        cmd_rooms, parse_args, 0, 1, NULL,
        { "/rooms [conference-service]", "List chat rooms.",
        { "/rooms [conference-service]",
          "---------------------------",
          "List the chat rooms available at the specified conference service",
          "If no argument is supplied, the account preference 'muc.service' is used, which is 'conference.<domain-part>' by default.",
          "",
          "Example : /rooms conference.jabber.org",
          "Example : /rooms (if logged in as me@server.org, is equivalent to /rooms conference.server.org)",
          NULL } } },

    { "/bookmark",
        cmd_bookmark, parse_args, 0, 4, NULL,
        { "/bookmark [add|list|remove] [room@server] [autojoin on|off] [nick nickname]",
          "Manage bookmarks.",
        { "/bookmark [add|list|remove] [room@server] [autojoin on|off] [nick nickname]",
          "---------------------------------------------------------------------------",
          "Manage bookmarks.",
          NULL } } },

    { "/disco",
        cmd_disco, parse_args, 1, 2, NULL,
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
        cmd_nick, parse_args_with_freetext, 1, 1, NULL,
        { "/nick nickname", "Change nickname in chat room.",
        { "/nick nickname",
          "--------------",
          "Change the name by which other members of a chat room see you.",
          "This command is only valid when called within a chat room window.",
          "",
          "Example : /nick kai hansen",
          "Example : /nick bob",
          NULL } } },

    { "/win",
        cmd_win, parse_args, 1, 1, NULL,
        { "/win num", "View a window.",
        { "/win num",
          "------------------",
          "Show the contents of a specific window in the main window area.",
          NULL } } },

    { "/wins",
        cmd_wins, parse_args, 0, 1, NULL,
        { "/wins [tidy|prune]", "List or tidy active windows.",
        { "/wins [tidy|prune]",
          "------------------",
          "Passing no argument will list all currently active windows and information about their usage.",
          "tidy  : Shuffle windows so there are no gaps.",
          "prune : Close all windows with no unread messages, and then tidy as above.",
          NULL } } },

    { "/sub",
        cmd_sub, parse_args, 1, 2, NULL,
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
        cmd_tiny, parse_args, 1, 1, NULL,
        { "/tiny url", "Send url as tinyurl in current chat.",
        { "/tiny url",
          "---------",
          "Send the url as a tiny url.",
          "",
          "Example : /tiny http://www.profanity.im",
          NULL } } },

    { "/duck",
        cmd_duck, parse_args_with_freetext, 1, 1, NULL,
        { "/duck query", "Perform search using DuckDuckGo chatbot.",
        { "/duck query",
          "-----------",
          "Send a search query to the DuckDuckGo chatbot.",
          "Your chat service must be federated, i.e. allow message to be sent/received outside of its domain.",
          "",
          "Example : /duck dennis ritchie",
          NULL } } },

    { "/who",
        cmd_who, parse_args, 0, 2, NULL,
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
        cmd_close, parse_args, 0, 1, NULL,
        { "/close [win|read|all]", "Close windows.",
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
        cmd_clear, parse_args, 0, 0, NULL,
        { "/clear", "Clear current window.",
        { "/clear",
          "------",
          "Clear the current window.",
          NULL } } },

    { "/quit",
        cmd_quit, parse_args, 0, 0, NULL,
        { "/quit", "Quit Profanity.",
        { "/quit",
          "-----",
          "Logout of any current session, and quit Profanity.",
          NULL } } },

    { "/beep",
        cmd_beep, parse_args, 1, 1, &cons_beep_setting,
        { "/beep on|off", "Terminal beep on new messages.",
        { "/beep on|off",
          "------------",
          "Switch the terminal bell on or off.",
          "The bell will sound when incoming messages are received.",
          "If the terminal does not support sounds, it may attempt to flash the screen instead.",
          NULL } } },

    { "/notify",
        cmd_notify, parse_args, 2, 2, &cons_notify_setting,
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
        cmd_flash, parse_args, 1, 1, &cons_flash_setting,
        { "/flash on|off", "Terminal flash on new messages.",
        { "/flash on|off",
          "-------------",
          "Make the terminal flash when incoming messages are recieved.",
          "The flash will only occur if you are not in the chat window associated with the user sending the message.",
          "If the terminal doesn't support flashing, it may attempt to beep.",
          NULL } } },

    { "/intype",
        cmd_intype, parse_args, 1, 1, &cons_intype_setting,
        { "/intype on|off", "Show when contact is typing.",
        { "/intype on|off",
          "--------------",
          "Show when a contact is typing in the console, and in active message window.",
          NULL } } },

    { "/splash",
        cmd_splash, parse_args, 1, 1, &cons_splash_setting,
        { "/splash on|off", "Splash logo on startup and /about command.",
        { "/splash on|off",
          "--------------",
          "Switch on or off the ascii logo on start up and when the /about command is called.",
          NULL } } },

    { "/autoconnect",
        cmd_autoconnect, parse_args, 1, 2, &cons_autoconnect_setting,
        { "/autoconnect set|off [account]", "Set account to autoconnect with.",
        { "/autoconnect set|off [account]",
          "------------------------------",
          "Enable or disable autoconnect on start up.",
          "The setting can be overridden by the -a (--account) command line option.",
          "",
          "Example: /autoconnect set jc@stuntteam.org (autoconnect with the specified account).",
          "Example: /autoconnect off                  (disable autoconnect).",
          NULL } } },

    { "/vercheck",
        cmd_vercheck, parse_args, 0, 1, NULL,
        { "/vercheck [on|off]", "Check for a new release.",
        { "/vercheck [on|off]",
          "------------------",
          "Without a parameter will check for a new release.",
          "Switching on or off will enable/disable a version check when Profanity starts, and each time the /about command is run.",
          NULL  } } },

    { "/titlebar",
        cmd_titlebar, parse_args, 1, 1, &cons_titlebar_setting,
        { "/titlebar on|off", "Show information in the window title bar.",
        { "/titlebar on|off",
          "----------------",
          "Show information in the window title bar.",
          NULL  } } },

    { "/mouse",
        cmd_mouse, parse_args, 1, 1, &cons_mouse_setting,
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

    { "/alias",
        cmd_alias, parse_args_with_freetext, 1, 3, NULL,
        { "/alias add|remove|list [name value]", "Add your own command aliases.",
        { "/alias add|remove|list [name value]",
          "-----------------------------------",
          "Add, remove or show command aliases.",
          "The alias will be available as a command",
          "Example : /alias add friends /who online friends",
          "Example : /alias add q /quit",
          "Example : /alias a /away \"I'm in a meeting.\"",
          "Example : /alias remove q",
          "Example : /alias list",
          "The above aliases will be available as /friends and /a",
          NULL } } },

    { "/chlog",
        cmd_chlog, parse_args, 1, 1, &cons_chlog_setting,
        { "/chlog on|off", "Chat logging to file.",
        { "/chlog on|off",
          "-------------",
          "Switch chat logging on or off.",
          "This setting will be enabled if /history is set to on.",
          "When disabling this option, /history will also be disabled.",
          "See the /grlog setting for enabling logging of chat room (groupchat) messages.",
          NULL } } },

    { "/grlog",
        cmd_grlog, parse_args, 1, 1, &cons_grlog_setting,
        { "/grlog on|off", "Chat logging of chat rooms to file.",
        { "/grlog on|off",
          "-------------",
          "Switch chat room logging on or off.",
          "See the /chlog setting for enabling logging of one to one chat.",
          NULL } } },

    { "/states",
        cmd_states, parse_args, 1, 1, &cons_states_setting,
        { "/states on|off", "Send chat states during a chat session.",
        { "/states on|off",
          "--------------",
          "Sending of chat state notifications during chat sessions.",
          "Such as whether you have become inactive, or have closed the chat window.",
          NULL } } },

    { "/otr",
        cmd_otr, parse_args, 1, 2, NULL,
        { "/otr gen|myfp|theirfp|start|end|trust|untrust|log|warn|libver", "Off The Record encryption commands.",
        { "/otr gen|myfp|theirfp|start|end|trust|untrust|log|warn|libver",
          "-------------------------------------------------------------",
          "gen - Generate your private key.",
          "myfp - Show your fingerprint.",
          "theirfp - Show contacts fingerprint.",
          "start <contact> - Start an OTR session with the contact, or the current recipient if in a chat window and no argument supplied.",
          "end - End the current OTR session,",
          "trust - Indicate that you have verified the contact's fingerprint.",
          "untrust - Indicate the the contact's fingerprint is not verified,",
          "log - How to log OTR messages, options are 'on', 'off' and 'redact', with redaction being the default.",
          "warn - Show when unencrypted messaging is being used in the title bar, options are 'on' and 'off' with 'on' being the default.",
          "libver - Show which version of the libotr library is being used.",
          NULL } } },

    { "/outtype",
        cmd_outtype, parse_args, 1, 1, &cons_outtype_setting,
        { "/outtype on|off", "Send typing notification to recipient.",
        { "/outtype on|off",
          "---------------",
          "Send an indication that you are typing to the chat recipient.",
          "Chat states (/states) will be enabled if this setting is set.",
          NULL } } },

    { "/gone",
        cmd_gone, parse_args, 1, 1, &cons_gone_setting,
        { "/gone minutes", "Send 'gone' state to recipient after a period.",
        { "/gone minutes",
          "-------------",
          "Send a 'gone' state to the recipient after the specified number of minutes."
          "This indicates to the recipient's client that you have left the conversation.",
          "A value of 0 will disable sending this chat state.",
          "Chat states (/states) will be enabled if this setting is set.",
          NULL } } },

    { "/history",
        cmd_history, parse_args, 1, 1, &cons_history_setting,
        { "/history on|off", "Chat history in message windows.",
        { "/history on|off",
          "---------------",
          "Switch chat history on or off, /chlog will automatically be enabled when this setting is on.",
          "When history is enabled, previous messages are shown in chat windows.",
          NULL } } },

    { "/log",
        cmd_log, parse_args, 2, 2, &cons_log_setting,
        { "/log maxsize value", "Manage system logging settings.",
        { "/log maxsize value",
          "------------------",
          "maxsize : When log file size exceeds this value it will be automatically",
          "          rotated (file will be renamed). Default value is 1048580 (1MB)",
          NULL } } },

    { "/reconnect",
        cmd_reconnect, parse_args, 1, 1, &cons_reconnect_setting,
        { "/reconnect seconds", "Set reconnect interval.",
        { "/reconnect seconds",
          "------------------",
          "Set the reconnect attempt interval in seconds for when the connection is lost.",
          "A value of 0 will switch off reconnect attempts.",
          NULL } } },

    { "/autoping",
        cmd_autoping, parse_args, 1, 1, &cons_autoping_setting,
        { "/autoping seconds", "Server ping interval.",
        { "/autoping seconds",
          "-----------------",
          "Set the number of seconds between server pings, so ensure connection kept alive.",
          "A value of 0 will switch off autopinging the server.",
          NULL } } },

    { "/autoaway",
        cmd_autoaway, parse_args_with_freetext, 2, 2, &cons_autoaway_setting,
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
        cmd_priority, parse_args, 1, 1, &cons_priority_setting,
        { "/priority value", "Set priority for the current account.",
        { "/priority value",
          "---------------",
          "Set priority for the current account, presence will be sent when calling this command.",
          "See the /account command for more specific priority settings per presence status.",
          "value : Number between -128 and 127. Default value is 0.",
          NULL } } },

    { "/account",
        cmd_account, parse_args, 0, 4, NULL,
        { "/account [command] [account] [property] [value]", "Manage accounts.",
        { "/account [command] [account] [property] [value]",
          "-----------------------------------------------",
          "Commands for creating and managing accounts.",
          "list                         : List all accounts.",
          "show account                 : Show information about an account.",
          "enable account               : Enable the account, it will be used for autocomplete.",
          "disable account              : Disable the account.",
          "add account                  : Create a new account.",
          "rename account newname       : Rename account to newname.",
          "set account property value   : Set 'property' of 'account' to 'value'.",
          "clear account property value : Clear 'property' of 'account'.",
          "",
          "When connected, the /account command can be called with no arguments, to show current account settings.",
          "",
          "The set command may use one of the following for 'property'.",
          "jid              : The Jabber ID of the account, the account name will be used if this property is not set.",
          "server           : The chat server, if different to the domainpart of the JID.",
          "port             : The port used for connecting if not the default (5222, or 5223 for SSL).",
          "status           : The presence status to use on login, use 'last' to use whatever your last status was.",
          "online|chat|away",
          "|xa|dnd          : Priority for the specified presence.",
          "resource         : The resource to be used.",
          "password         : Password for the account, note this is currently stored in plaintext if set.",
          "muc              : The default MUC chat service to use.",
          "nick             : The default nickname to use when joining chat rooms.",
          "",
          "The clear command may use one of the following for 'property'.",
          "password         : Clears the password for the account.",
          "",
          "Example : /account add work",
          "        : /account set work jid me@chatty",
          "        : /account set work server talk.chat.com",
          "        : /account set work port 5111",
          "        : /account set work resource desktop",
          "        : /account set work muc chatservice.mycompany.com",
          "        : /account set work nick dennis",
          "        : /account set work status dnd",
          "        : /account set work dnd -1",
          "        : /account set work online 10",
          "        : /account rename work gtalk",
          NULL  } } },

    { "/prefs",
        cmd_prefs, parse_args, 0, 1, NULL,
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
        cmd_theme, parse_args, 1, 2, &cons_theme_setting,
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
        cmd_statuses, parse_args, 2, 2, &cons_statuses_setting,
        { "/statuses console|chat|muc setting", "Set preferences for presence change messages.",
        { "/statuses console|chat|muc setting",
          "----------------------------------",
          "Configure how presence changes are displayed in various windows.",
          "Settings:",
          "  all - Show all presence changes.",
          "  online - Show only online/offline changes.",
          "  none - Don't show any presence changes.",
          "The default is 'all' for all windows.",
          NULL } } },

    { "/away",
        cmd_away, parse_args_with_freetext, 0, 1, NULL,
        { "/away [msg]", "Set status to away.",
        { "/away [msg]",
          "-----------",
          "Set your status to 'away' with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /away Gone for lunch",
          NULL } } },

    { "/chat",
        cmd_chat, parse_args_with_freetext, 0, 1, NULL,
        { "/chat [msg]", "Set status to chat (available for chat).",
        { "/chat [msg]",
          "-----------",
          "Set your status to 'chat', meaning 'available for chat', with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /chat Please talk to me!",
          NULL } } },

    { "/dnd",
        cmd_dnd, parse_args_with_freetext, 0, 1, NULL,
        { "/dnd [msg]", "Set status to dnd (do not disturb).",
        { "/dnd [msg]",
          "----------",
          "Set your status to 'dnd', meaning 'do not disturb', with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /dnd I'm in the zone",
          NULL } } },

    { "/online",
        cmd_online, parse_args_with_freetext, 0, 1, NULL,
        { "/online [msg]", "Set status to online.",
        { "/online [msg]",
          "-------------",
          "Set your status to 'online' with the optional message.",
          "Your current status can be found in the top right of the screen.",
          "",
          "Example : /online Up the Irons!",
          NULL } } },

    { "/xa",
        cmd_xa, parse_args_with_freetext, 0, 1, NULL,
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
static Autocomplete autoconnect_ac;
static Autocomplete titlebar_ac;
static Autocomplete theme_ac;
static Autocomplete theme_load_ac;
static Autocomplete account_ac;
static Autocomplete account_set_ac;
static Autocomplete account_clear_ac;
static Autocomplete disco_ac;
static Autocomplete close_ac;
static Autocomplete wins_ac;
static Autocomplete roster_ac;
static Autocomplete group_ac;
static Autocomplete bookmark_ac;
static Autocomplete otr_ac;
static Autocomplete otr_log_ac;
static Autocomplete connect_property_ac;
static Autocomplete statuses_ac;
static Autocomplete statuses_setting_ac;
static Autocomplete alias_ac;
static Autocomplete aliases_ac;
static Autocomplete join_property_ac;

/*
 * Initialise command autocompleter and history
 */
void
cmd_init(void)
{
    log_info("Initialising commands");

    commands_ac = autocomplete_new();
    aliases_ac = autocomplete_new();

    help_ac = autocomplete_new();
    autocomplete_add(help_ac, "commands");
    autocomplete_add(help_ac, "basic");
    autocomplete_add(help_ac, "chatting");
    autocomplete_add(help_ac, "groupchat");
    autocomplete_add(help_ac, "presence");
    autocomplete_add(help_ac, "contacts");
    autocomplete_add(help_ac, "service");
    autocomplete_add(help_ac, "settings");
    autocomplete_add(help_ac, "other");
    autocomplete_add(help_ac, "navigation");

    // load command defs into hash table
    commands = g_hash_table_new(g_str_hash, g_str_equal);
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(command_defs); i++) {
        Command *pcmd = command_defs+i;

        // add to hash
        g_hash_table_insert(commands, pcmd->cmd, pcmd);

        // add to commands and help autocompleters
        autocomplete_add(commands_ac, pcmd->cmd);
        autocomplete_add(help_ac, pcmd->cmd+1);
    }

    // load aliases
    GList *aliases = prefs_get_aliases();
    GList *curr = aliases;
    while (curr != NULL) {
        ProfAlias *alias = curr->data;
        GString *ac_alias = g_string_new("/");
        g_string_append(ac_alias, alias->name);
        autocomplete_add(commands_ac, ac_alias->str);
        autocomplete_add(aliases_ac, alias->name);
        g_string_free(ac_alias, TRUE);
        curr = g_list_next(curr);
    }

    prefs_ac = autocomplete_new();
    autocomplete_add(prefs_ac, "ui");
    autocomplete_add(prefs_ac, "desktop");
    autocomplete_add(prefs_ac, "chat");
    autocomplete_add(prefs_ac, "log");
    autocomplete_add(prefs_ac, "conn");
    autocomplete_add(prefs_ac, "presence");

    notify_ac = autocomplete_new();
    autocomplete_add(notify_ac, "message");
    autocomplete_add(notify_ac, "typing");
    autocomplete_add(notify_ac, "remind");
    autocomplete_add(notify_ac, "invite");
    autocomplete_add(notify_ac, "sub");

    sub_ac = autocomplete_new();
    autocomplete_add(sub_ac, "request");
    autocomplete_add(sub_ac, "allow");
    autocomplete_add(sub_ac, "deny");
    autocomplete_add(sub_ac, "show");
    autocomplete_add(sub_ac, "sent");
    autocomplete_add(sub_ac, "received");

    titlebar_ac = autocomplete_new();
    autocomplete_add(titlebar_ac, "version");

    log_ac = autocomplete_new();
    autocomplete_add(log_ac, "maxsize");

    autoaway_ac = autocomplete_new();
    autocomplete_add(autoaway_ac, "mode");
    autocomplete_add(autoaway_ac, "time");
    autocomplete_add(autoaway_ac, "message");
    autocomplete_add(autoaway_ac, "check");

    autoaway_mode_ac = autocomplete_new();
    autocomplete_add(autoaway_mode_ac, "away");
    autocomplete_add(autoaway_mode_ac, "idle");
    autocomplete_add(autoaway_mode_ac, "off");

    autoconnect_ac = autocomplete_new();
    autocomplete_add(autoconnect_ac, "set");
    autocomplete_add(autoconnect_ac, "off");

    theme_ac = autocomplete_new();
    autocomplete_add(theme_ac, "list");
    autocomplete_add(theme_ac, "set");

    disco_ac = autocomplete_new();
    autocomplete_add(disco_ac, "info");
    autocomplete_add(disco_ac, "items");

    account_ac = autocomplete_new();
    autocomplete_add(account_ac, "list");
    autocomplete_add(account_ac, "show");
    autocomplete_add(account_ac, "add");
    autocomplete_add(account_ac, "enable");
    autocomplete_add(account_ac, "disable");
    autocomplete_add(account_ac, "rename");
    autocomplete_add(account_ac, "set");
    autocomplete_add(account_ac, "clear");

    account_set_ac = autocomplete_new();
    autocomplete_add(account_set_ac, "jid");
    autocomplete_add(account_set_ac, "server");
    autocomplete_add(account_set_ac, "port");
    autocomplete_add(account_set_ac, "status");
    autocomplete_add(account_set_ac, "online");
    autocomplete_add(account_set_ac, "chat");
    autocomplete_add(account_set_ac, "away");
    autocomplete_add(account_set_ac, "xa");
    autocomplete_add(account_set_ac, "dnd");
    autocomplete_add(account_set_ac, "resource");
    autocomplete_add(account_set_ac, "password");
    autocomplete_add(account_set_ac, "muc");
    autocomplete_add(account_set_ac, "nick");

    account_clear_ac = autocomplete_new();
    autocomplete_add(account_clear_ac, "password");

    close_ac = autocomplete_new();
    autocomplete_add(close_ac, "read");
    autocomplete_add(close_ac, "all");

    wins_ac = autocomplete_new();
    autocomplete_add(wins_ac, "prune");
    autocomplete_add(wins_ac, "tidy");

    roster_ac = autocomplete_new();
    autocomplete_add(roster_ac, "add");
    autocomplete_add(roster_ac, "nick");
    autocomplete_add(roster_ac, "clearnick");
    autocomplete_add(roster_ac, "remove");

    group_ac = autocomplete_new();
    autocomplete_add(group_ac, "show");
    autocomplete_add(group_ac, "add");
    autocomplete_add(group_ac, "remove");

    theme_load_ac = NULL;

    who_ac = autocomplete_new();
    autocomplete_add(who_ac, "chat");
    autocomplete_add(who_ac, "online");
    autocomplete_add(who_ac, "away");
    autocomplete_add(who_ac, "xa");
    autocomplete_add(who_ac, "dnd");
    autocomplete_add(who_ac, "offline");
    autocomplete_add(who_ac, "available");
    autocomplete_add(who_ac, "unavailable");
    autocomplete_add(who_ac, "any");

    bookmark_ac = autocomplete_new();
    autocomplete_add(bookmark_ac, "add");
    autocomplete_add(bookmark_ac, "list");
    autocomplete_add(bookmark_ac, "remove");

    otr_ac = autocomplete_new();
    autocomplete_add(otr_ac, "gen");
    autocomplete_add(otr_ac, "start");
    autocomplete_add(otr_ac, "end");
    autocomplete_add(otr_ac, "myfp");
    autocomplete_add(otr_ac, "theirfp");
    autocomplete_add(otr_ac, "trust");
    autocomplete_add(otr_ac, "untrust");
    autocomplete_add(otr_ac, "log");
    autocomplete_add(otr_ac, "warn");
    autocomplete_add(otr_ac, "libver");

    otr_log_ac = autocomplete_new();
    autocomplete_add(otr_log_ac, "on");
    autocomplete_add(otr_log_ac, "off");
    autocomplete_add(otr_log_ac, "redact");

    connect_property_ac = autocomplete_new();
    autocomplete_add(connect_property_ac, "server");
    autocomplete_add(connect_property_ac, "port");

    join_property_ac = autocomplete_new();
    autocomplete_add(join_property_ac, "nick");
    autocomplete_add(join_property_ac, "password");

    statuses_ac = autocomplete_new();
    autocomplete_add(statuses_ac, "console");
    autocomplete_add(statuses_ac, "chat");
    autocomplete_add(statuses_ac, "muc");

    statuses_setting_ac = autocomplete_new();
    autocomplete_add(statuses_setting_ac, "all");
    autocomplete_add(statuses_setting_ac, "online");
    autocomplete_add(statuses_setting_ac, "none");

    alias_ac = autocomplete_new();
    autocomplete_add(alias_ac, "add");
    autocomplete_add(alias_ac, "remove");
    autocomplete_add(alias_ac, "list");

    cmd_history_init();
}

void
cmd_uninit(void)
{
    autocomplete_free(commands_ac);
    autocomplete_free(who_ac);
    autocomplete_free(help_ac);
    autocomplete_free(notify_ac);
    autocomplete_free(sub_ac);
    autocomplete_free(titlebar_ac);
    autocomplete_free(log_ac);
    autocomplete_free(prefs_ac);
    autocomplete_free(autoaway_ac);
    autocomplete_free(autoaway_mode_ac);
    autocomplete_free(autoconnect_ac);
    autocomplete_free(theme_ac);
    if (theme_load_ac != NULL) {
        autocomplete_free(theme_load_ac);
    }
    autocomplete_free(account_ac);
    autocomplete_free(account_set_ac);
    autocomplete_free(account_clear_ac);
    autocomplete_free(disco_ac);
    autocomplete_free(close_ac);
    autocomplete_free(wins_ac);
    autocomplete_free(roster_ac);
    autocomplete_free(group_ac);
    autocomplete_free(bookmark_ac);
    autocomplete_free(otr_ac);
    autocomplete_free(otr_log_ac);
    autocomplete_free(connect_property_ac);
    autocomplete_free(statuses_ac);
    autocomplete_free(statuses_setting_ac);
    autocomplete_free(alias_ac);
    autocomplete_free(aliases_ac);
    autocomplete_free(join_property_ac);
}

gboolean
cmd_exists(char *cmd)
{
    if (commands_ac == NULL) {
        return FALSE;
    } else {
        return autocomplete_contains(commands_ac, cmd);
    }
}

void
cmd_autocomplete_add(char *value)
{
    if (commands_ac != NULL) {
        autocomplete_add(commands_ac, value);
    }
}

void
cmd_autocomplete_remove(char *value)
{
    if (commands_ac != NULL) {
        autocomplete_remove(commands_ac, value);
    }
}

void
cmd_alias_add(char *value)
{
    if (aliases_ac != NULL) {
        autocomplete_add(aliases_ac, value);
    }
}

void
cmd_alias_remove(char *value)
{
    if (aliases_ac != NULL) {
        autocomplete_remove(aliases_ac, value);
    }
}

// Command autocompletion functions
void
cmd_autocomplete(char *input, int *size)
{
    int i = 0;
    char *found = NULL;
    char inp_cpy[*size];

    // autocomplete command
    if ((strncmp(input, "/", 1) == 0) && (!str_contains(input, *size, ' '))) {
        for(i = 0; i < *size; i++) {
            inp_cpy[i] = input[i];
        }
        inp_cpy[i] = '\0';
        found = autocomplete_complete(commands_ac, inp_cpy);
        if (found != NULL) {
            char *auto_msg = strdup(found);
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
    autocomplete_reset(autoconnect_ac);
    autocomplete_reset(theme_ac);
    if (theme_load_ac != NULL) {
        autocomplete_reset(theme_load_ac);
        theme_load_ac = NULL;
    }
    autocomplete_reset(account_ac);
    autocomplete_reset(account_set_ac);
    autocomplete_reset(account_clear_ac);
    autocomplete_reset(disco_ac);
    autocomplete_reset(close_ac);
    autocomplete_reset(wins_ac);
    autocomplete_reset(roster_ac);
    autocomplete_reset(group_ac);
    autocomplete_reset(bookmark_ac);
    autocomplete_reset(otr_ac);
    autocomplete_reset(otr_log_ac);
    autocomplete_reset(connect_property_ac);
    autocomplete_reset(statuses_ac);
    autocomplete_reset(statuses_setting_ac);
    autocomplete_reset(alias_ac);
    autocomplete_reset(aliases_ac);
    autocomplete_reset(join_property_ac);
    bookmark_autocomplete_reset();
}

// Command execution

gboolean
cmd_execute(const char * const command, const char * const inp)
{
    Command *cmd = g_hash_table_lookup(commands, command);

    if (cmd != NULL) {
        gchar **args = cmd->parser(inp, cmd->min_args, cmd->max_args);
        if ((args == NULL) && (cmd->setting_func != NULL)) {
            cons_show("");
            (*cmd->setting_func)();
            cons_show("Usage: %s", cmd->help.usage);
            return TRUE;
        } else if (args == NULL) {
            cons_show("");
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
        gboolean ran_alias = FALSE;
        gboolean alias_result = cmd_execute_alias(inp, &ran_alias);
        if (!ran_alias) {
            return cmd_execute_default(inp);
        } else {
            return alias_result;
        }
    }
}

gboolean
cmd_execute_alias(const char * const inp, gboolean *ran)
{
    if (inp[0] != '/') {
        ran = FALSE;
        return TRUE;
    } else {
        char *alias = strdup(inp+1);
        char *value = prefs_get_alias(alias);
        free(alias);
        if (value != NULL) {
            *ran = TRUE;
            return process_input(value);
        } else {
            *ran = FALSE;
            return TRUE;
        }
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
            }
            break;

        case WIN_CHAT:
            if (status != JABBER_CONNECTED) {
                ui_current_print_line("You are not currently connected.");
            } else {
#ifdef HAVE_LIBOTR
                if (otr_is_secure(recipient)) {
                    char *encrypted = otr_encrypt_message(recipient, inp);
                    if (encrypted != NULL) {
                        message_send(encrypted, recipient);
                        otr_free_message(encrypted);
                        if (prefs_get_boolean(PREF_CHLOG)) {
                            const char *jid = jabber_get_fulljid();
                            Jid *jidp = jid_create(jid);
                            if (strcmp(prefs_get_string(PREF_OTR_LOG), "on") == 0) {
                                chat_log_chat(jidp->barejid, recipient, inp, PROF_OUT_LOG, NULL);
                            } else if (strcmp(prefs_get_string(PREF_OTR_LOG), "redact") == 0) {
                                chat_log_chat(jidp->barejid, recipient, "[redacted]", PROF_OUT_LOG, NULL);
                            }
                            jid_destroy(jidp);
                        }

                        ui_outgoing_msg("me", recipient, inp);
                    } else {
                        cons_show_error("Failed to send message.");
                    }
                } else {
                    message_send(inp, recipient);
                    if (prefs_get_boolean(PREF_CHLOG)) {
                        const char *jid = jabber_get_fulljid();
                        Jid *jidp = jid_create(jid);
                        chat_log_chat(jidp->barejid, recipient, inp, PROF_OUT_LOG, NULL);
                        jid_destroy(jidp);
                    }

                    ui_outgoing_msg("me", recipient, inp);
                }
#else
                message_send(inp, recipient);
                if (prefs_get_boolean(PREF_CHLOG)) {
                    const char *jid = jabber_get_fulljid();
                    Jid *jidp = jid_create(jid);
                    chat_log_chat(jidp->barejid, recipient, inp, PROF_OUT_LOG, NULL);
                    jid_destroy(jidp);
                }

                ui_outgoing_msg("me", recipient, inp);
#endif
            }
            break;

        case WIN_PRIVATE:
            if (status != JABBER_CONNECTED) {
                ui_current_print_line("You are not currently connected.");
            } else {
                message_send(inp, recipient);
                ui_outgoing_msg("me", recipient, inp);
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
        "/flash", "/splash", "/chlog", "/grlog", "/mouse", "/history", "/titlebar",
        "/vercheck" };

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
        char *recipient = ui_current_recipient();
        Autocomplete nick_ac = muc_get_roster_ac(recipient);
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

    gchar *cmds[] = { "/help", "/prefs", "/log", "/disco", "/close", "/wins" };
    Autocomplete completers[] = { help_ac, prefs_ac, log_ac, disco_ac, close_ac, wins_ac };

    for (i = 0; i < ARRAY_SIZE(cmds); i++) {
        result = autocomplete_param_with_ac(input, size, cmds[i], completers[i]);
        if (result != NULL) {
            inp_replace_input(input, result, size);
            g_free(result);
            return;
        }
    }

    autocompleter acs[] = { _who_autocomplete, _sub_autocomplete, _notify_autocomplete,
        _autoaway_autocomplete, _theme_autocomplete,
        _account_autocomplete, _roster_autocomplete, _group_autocomplete,
        _bookmark_autocomplete, _autoconnect_autocomplete, _otr_autocomplete,
        _connect_autocomplete, _statuses_autocomplete, _alias_autocomplete,
        _join_autocomplete };

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
    result = autocomplete_param_with_func(input, size, "/roster clearnick", roster_find_jid);
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
_bookmark_autocomplete(char *input, int *size)
{
    char *result = NULL;

    if (strcmp(input, "/bookmark add ") == 0) {
        GString *str = g_string_new(input);

        str = g_string_append(str, "autojoin");
        result = str->str;
        g_string_free(str, FALSE);
        return result;
    }

    result = autocomplete_param_with_func(input, size, "/bookmark list", bookmark_find);
    if (result != NULL) {
        return result;
    }
    result = autocomplete_param_with_func(input, size, "/bookmark remove", bookmark_find);
    if (result != NULL) {
        return result;
    }

    result = autocomplete_param_with_ac(input, size, "/bookmark", bookmark_ac);

    return result;
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
_autoconnect_autocomplete(char *input, int *size)
{
    char *result = NULL;

    result = autocomplete_param_with_func(input, size, "/autoconnect set", accounts_find_enabled);
    if (result != NULL) {
        return result;
    }

    result = autocomplete_param_with_ac(input, size, "/autoconnect", autoconnect_ac);
    if (result != NULL) {
        return result;
    }

    return NULL;
}

static char *
_otr_autocomplete(char *input, int *size)
{
    char *result = NULL;

    result = autocomplete_param_with_func(input, size, "/otr start", roster_find_contact);
    if (result != NULL) {
        return result;
    }

    result = autocomplete_param_with_ac(input, size, "/otr log", otr_log_ac);
    if (result != NULL) {
        return result;
    }

    result = autocomplete_param_with_func(input, size, "/otr warn",
        prefs_autocomplete_boolean_choice);
    if (result != NULL) {
        return result;
    }

    result = autocomplete_param_with_ac(input, size, "/otr", otr_ac);
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
                autocomplete_add(theme_load_ac, themes->data);
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
_statuses_autocomplete(char *input, int *size)
{
    char *result = NULL;

    result = autocomplete_param_with_ac(input, size, "/statuses console", statuses_setting_ac);
    if (result != NULL) {
        return result;
    }

    result = autocomplete_param_with_ac(input, size, "/statuses chat", statuses_setting_ac);
    if (result != NULL) {
        return result;
    }

    result = autocomplete_param_with_ac(input, size, "/statuses muc", statuses_setting_ac);
    if (result != NULL) {
        return result;
    }

    result = autocomplete_param_with_ac(input, size, "/statuses", statuses_ac);
    if (result != NULL) {
        return result;
    }

    return NULL;
}

static char *
_alias_autocomplete(char *input, int *size)
{
    char *result = NULL;

    result = autocomplete_param_with_ac(input, size, "/alias remove", aliases_ac);
    if (result != NULL) {
        return result;
    }

    result = autocomplete_param_with_ac(input, size, "/alias", alias_ac);
    if (result != NULL) {
        return result;
    }

    return NULL;
}

static char *
_connect_autocomplete(char *input, int *size)
{
    char *result = NULL;

    input[*size] = '\0';
    gchar **args = parse_args(input, 2, 4);

    if ((strncmp(input, "/connect", 8) == 0) && (args != NULL)) {
        GString *beginning = g_string_new("/connect ");
        g_string_append(beginning, args[0]);
        if (args[1] != NULL && args[2] != NULL) {
            g_string_append(beginning, " ");
            g_string_append(beginning, args[1]);
            g_string_append(beginning, " ");
            g_string_append(beginning, args[2]);
        }
        result = autocomplete_param_with_ac(input, size, beginning->str, connect_property_ac);
        g_string_free(beginning, TRUE);
        if (result != NULL) {
            return result;
        }
    }

    result = autocomplete_param_with_func(input, size, "/connect", accounts_find_enabled);
    if (result != NULL) {
        return result;
    }

    return NULL;
}

static char *
_join_autocomplete(char *input, int *size)
{
    char *result = NULL;

    input[*size] = '\0';
    gchar **args = parse_args(input, 2, 4);

    if ((strncmp(input, "/join", 5) == 0) && (args != NULL)) {
        GString *beginning = g_string_new("/join ");
        g_string_append(beginning, args[0]);
        if (args[1] != NULL && args[2] != NULL) {
            g_string_append(beginning, " ");
            g_string_append(beginning, args[1]);
            g_string_append(beginning, " ");
            g_string_append(beginning, args[2]);
        }
        result = autocomplete_param_with_ac(input, size, beginning->str, join_property_ac);
        g_string_free(beginning, TRUE);
        if (result != NULL) {
            return result;
        }
    }

    return NULL;
}

static char *
_account_autocomplete(char *input, int *size)
{
    char *result = NULL;

    input[*size] = '\0';
    gchar **args = parse_args(input, 3, 3);

    if ((strncmp(input, "/account set", 12) == 0) && (args != NULL)) {
        GString *beginning = g_string_new("/account set ");
        g_string_append(beginning, args[1]);
        result = autocomplete_param_with_ac(input, size, beginning->str, account_set_ac);
        g_string_free(beginning, TRUE);
        if (result != NULL) {
            return result;
        }
    }

    if ((strncmp(input, "/account clear", 14) == 0) && (args != NULL)) {
        GString *beginning = g_string_new("/account clear ");
        g_string_append(beginning, args[1]);
        result = autocomplete_param_with_ac(input, size, beginning->str, account_clear_ac);
        g_string_free(beginning, TRUE);
        if (result != NULL) {
            return result;
        }
    }

    g_strfreev(args);

    int i = 0;
    gchar *account_choice[] = { "/account set", "/account show", "/account enable",
        "/account disable", "/account rename", "/account clear" };

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

