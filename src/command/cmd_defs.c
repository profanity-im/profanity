/*
 * cmd_defs.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
 */

#define _GNU_SOURCE 1

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <dirent.h>
#include <sys/types.h>

#include <glib.h>

#include "profanity.h"
#include "log.h"
#include "common.h"
#include "command/cmd_defs.h"
#include "command/cmd_funcs.h"
#include "command/cmd_ac.h"
#include "config/accounts.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "config/tlscerts.h"
#include "config/scripts.h"
#include "plugins/plugins.h"
#include "tools/autocomplete.h"
#include "tools/parser.h"
#include "tools/tinyurl.h"
#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/xmpp.h"
#include "xmpp/contact.h"
#include "xmpp/roster_list.h"
#include "xmpp/jid.h"
#include "xmpp/chat_session.h"
#include "xmpp/muc.h"

#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif

#ifdef HAVE_LIBGPGME
#include "pgp/gpg.h"
#endif

#define CMD_TAG_CHAT        "chat"
#define CMD_TAG_GROUPCHAT   "groupchat"
#define CMD_TAG_ROSTER      "roster"
#define CMD_TAG_PRESENCE    "presence"
#define CMD_TAG_CONNECTION  "connection"
#define CMD_TAG_DISCOVERY   "discovery"
#define CMD_TAG_UI          "ui"
#define CMD_TAG_PLUGINS     "plugins"

#define CMD_MAINFUNC(func)  func,
#define CMD_NOMAINFUNC      NULL,
#define CMD_SUBFUNCS(...)   { __VA_ARGS__, { NULL, NULL } },
#define CMD_NOSUBFUNCS      { { NULL, NULL } },

#define CMD_NOTAGS          { { NULL },
#define CMD_TAGS(...)       { { __VA_ARGS__, NULL },
#define CMD_SYN(...)        { __VA_ARGS__, NULL },
#define CMD_DESC(desc)      desc,
#define CMD_NOARGS          { { NULL, NULL } },
#define CMD_ARGS(...)       { __VA_ARGS__, { NULL, NULL } },
#define CMD_NOEXAMPLES      { NULL } }
#define CMD_EXAMPLES(...)   { __VA_ARGS__, NULL } }

GHashTable *commands = NULL;

static gboolean _cmd_has_tag(Command *pcmd, const char *const tag);

/*
 * Command list
 */
static struct cmd_t command_defs[] =
{
    { "/help",
        parse_args_with_freetext, 0, 2, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_help)
        CMD_NOTAGS
        CMD_SYN(
            "/help [<area>|<command>|search_all|search_any] [<search_terms>]")
        CMD_DESC(
            "Help on using Profanity. Passing no arguments list help areas. "
            "For command help, optional arguments are shown using square brackets, "
            "arguments representing variables rather than a literal name are surrounded by angle brackets. "
            "Arguments that may be one of a number of values are separated by a pipe "
            "e.g. val1|val2|val3.")
        CMD_ARGS(
            { "<area>",                     "Summary help for commands in a certain area of functionality." },
            { "<command>",                  "Full help for a specific command, for example '/help connect'." },
            { "search_all <search_terms>",  "Search commands for returning matches that contain all of the search terms." },
            { "search_any <search_terms>",  "Search commands for returning matches that contain any of the search terms." })
        CMD_EXAMPLES(
            "/help search_all presence online",
            "/help commands",
            "/help presence",
            "/help who")
    },

    { "/about",
        parse_args, 0, 0, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_about)
        CMD_NOTAGS
        CMD_SYN(
            "/about")
        CMD_DESC(
            "Show version and license information.")
        CMD_NOARGS
        CMD_NOEXAMPLES
    },

    { "/connect",
        parse_args, 0, 7, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_connect)
        CMD_TAGS(
            CMD_TAG_CONNECTION)
        CMD_SYN(
            "/connect [<account>]",
            "/connect <account> [server <server>] [port <port>] [tls force|allow|legacy|disable]")
        CMD_DESC(
            "Login to a chat service. "
            "If no account is specified, the default is used if one is configured. "
            "A local account is created with the JID as it's name if it doesn't already exist.")
        CMD_ARGS(
            { "<account>",         "The local account you wish to connect with, or a JID if connecting for the first time." },
            { "server <server>",   "Supply a server if it is different to the domain part of your JID." },
            { "port <port>",       "The port to use if different to the default (5222, or 5223 for SSL)." },
            { "tls force",         "Force TLS connection, and fail if one cannot be established, this is default behaviour." },
            { "tls allow",         "Use TLS for the connection if it is available." },
            { "tls legacy",        "Use legacy TLS for the connection. It means server doesn't support STARTTLS and TLS is forced just after TCP connection is established." },
            { "tls disable",       "Disable TLS for the connection." })
        CMD_EXAMPLES(
            "/connect",
            "/connect myuser@gmail.com",
            "/connect myuser@mycompany.com server talk.google.com",
            "/connect bob@someplace port 5678",
            "/connect me@localhost.test.org server 127.0.0.1 tls disable",
            "/connect me@chatty server chatty.com port 5443")
        },

    { "/tls",
        parse_args, 1, 3, NULL,
        CMD_SUBFUNCS(
            { "certpath",   cmd_tls_certpath },
            { "trust",      cmd_tls_trust },
            { "trusted",    cmd_tls_trusted },
            { "revoke",     cmd_tls_revoke },
            { "show",       cmd_tls_show },
            { "cert",       cmd_tls_cert })
        CMD_NOMAINFUNC
        CMD_TAGS(
            CMD_TAG_CONNECTION,
            CMD_TAG_UI)
        CMD_SYN(
            "/tls allow",
            "/tls always",
            "/tls deny",
            "/tls cert [<fingerprint>]",
            "/tls trust",
            "/tls trusted",
            "/tls revoke <fingerprint>",
            "/tls certpath",
            "/tls certpath set <path>",
            "/tls certpath clear",
            "/tls certpath default",
            "/tls show on|off")
        CMD_DESC(
            "Handle TLS certificates. ")
        CMD_ARGS(
            { "allow",                "Allow connection to continue with TLS certificate." },
            { "always",               "Always allow connections with TLS certificate." },
            { "deny",                 "Abort connection." },
            { "cert",                 "Show the current TLS certificate." },
            { "cert <fingerprint>",   "Show details of trusted certificate." },
            { "trust",                "Add the current TLS certificate to manually trusted certificates." },
            { "trusted",              "List summary of manually trusted certificates (with '/tls always' or '/tls trust')." },
            { "revoke <fingerprint>", "Remove a manually trusted certificate." },
            { "certpath",             "Show the trusted certificate path." },
            { "certpath set <path>",  "Specify filesystem path containing trusted certificates." },
            { "certpath clear",       "Clear the trusted certificate path." },
            { "certpath default",     "Use default system certificate path, if it can be found." },
            { "show on|off",          "Show or hide the TLS indicator in the titlebar." })
        CMD_NOEXAMPLES
    },

    { "/disconnect",
        parse_args, 0, 0, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_disconnect)
        CMD_TAGS(
            CMD_TAG_CONNECTION)
        CMD_SYN(
            "/disconnect")
        CMD_DESC(
            "Disconnect from the current chat service.")
        CMD_NOARGS
        CMD_NOEXAMPLES
    },

    { "/msg",
        parse_args_with_freetext, 1, 2, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_msg)
        CMD_TAGS(
            CMD_TAG_CHAT)
        CMD_SYN(
            "/msg <contact> [<message>]",
            "/msg <nick> [<message>]")
        CMD_DESC(
            "Send a one to one chat message, or a private message to a chat room occupant. "
            "If the message is omitted, a new chat window will be opened without sending a message. "
            "Use quotes if the nickname includes spaces.")
        CMD_ARGS(
            { "<contact>",             "Open chat window with contact, by JID or nickname." },
            { "<contact> [<message>]", "Send message to contact, by JID or nickname." },
            { "<nick>",                "Open private chat window with chat room occupant." },
            { "<nick> [<message>]",    "Send a private message to a chat room occupant." })
        CMD_EXAMPLES(
            "/msg myfriend@server.com Hey, here's a message!",
            "/msg otherfriend@server.com",
            "/msg Bob Here is a private message",
            "/msg \"My Friend\" Hi, how are you?")
    },

    { "/roster",
        parse_args_with_freetext, 0, 4, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_roster)
        CMD_TAGS(
            CMD_TAG_ROSTER,
            CMD_TAG_UI)
        CMD_SYN(
            "/roster",
            "/roster online",
            "/roster show [offline|resource|presence|status|empty|priority|contacts|rooms]",
            "/roster hide [offline|resource|presence|status|empty|priority|contacts|rooms]",
            "/roster by group|presence|none",
            "/roster count unread|items|off",
            "/roster count zero on|off",
            "/roster order name|presence",
            "/roster unread before|after|off",
            "/roster room char <char>|none",
            "/roster room private char <char>|none",
            "/roster room position first|last",
            "/roster room by service|none",
            "/roster room order name|unread",
            "/roster room unread before|after|off",
            "/roster private room|group|off",
            "/roster private char <char>|none",
            "/roster header char <char>|none",
            "/roster presence indent <indent>",
            "/roster contact char <char>|none",
            "/roster contact indent <indent>",
            "/roster resource char <char>|none",
            "/roster resource indent <indent>",
            "/roster resource join on|off",
            "/roster size <percent>",
            "/roster wrap on|off",
            "/roster add <jid> [<nick>]",
            "/roster remove <jid>",
            "/roster remove_all contacts",
            "/roster nick <jid> <nick>",
            "/roster clearnick <jid>")
        CMD_DESC(
            "Manage your roster, and roster display settings. "
            "Passing no arguments lists all contacts in your roster.")
        CMD_ARGS(
            { "online",                     "Show all online contacts in console." },
            { "show",                       "Show the roster panel." },
            { "show offline",               "Show offline contacts in roster panel." },
            { "show resource",              "Show contact's connected resources in roster panel." },
            { "show presence",              "Show contact's presence in roster panel." },
            { "show status",                "Show contact's status message in roster panel." },
            { "show empty",                 "Show empty groups in roster panel." },
            { "show priority",              "Show resource priority in roster panel." },
            { "show contacts",              "Show contacts in roster panel." },
            { "show rooms",                 "Show chat rooms in roster panel." },
            { "hide",                       "Hide the roster panel." },
            { "hide offline",               "Hide offline contacts in roster panel." },
            { "hide resource",              "Hide contact's connected resources in roster panel." },
            { "hide presence",              "Hide contact's presence in roster panel." },
            { "hide status",                "Hide contact's status message in roster panel." },
            { "hide empty",                 "Hide empty groups in roster panel." },
            { "hide priority",              "Hide resource priority in roster panel." },
            { "hide contacts",              "Hide contacts in roster panel." },
            { "hide rooms",                 "Hide chat rooms in roster panel." },
            { "by group",                   "Group contacts in roster panel by roster group." },
            { "by presence",                "Group contacts in roster panel by presence." },
            { "by none",                    "No grouping in roster panel." },
            { "count unread",               "Show unread message count with roster headers." },
            { "count items",                "Show item count with roster headers." },
            { "count off",                  "Do not show any count with roster headers." },
            { "count zero on",              "Show roster header count when 0." },
            { "count zero off",             "Hide roster header count when 0." },
            { "order name",                 "Order roster contacts by name only." },
            { "order presence",             "Order roster contacts by presence, and then by name." },
            { "unread before",              "Show unread message count before contact." },
            { "unread after",               "Show unread message count after contact." },
            { "unread off",                 "Do not show unread message count for contacts." },
            { "room char <char>",           "Prefix rooms with specified character." },
            { "room char none",             "Remove room character prefix." },
            { "room private char <char>",   "Prefix private room chat with specified character when displayed with room." },
            { "room private char none",     "Remove private room chat character prefix when displayed with room." },
            { "room position first",        "Show rooms first in roster." },
            { "room position last",         "Show rooms last in roster." },
            { "room by service",            "Group rooms by chat service." },
            { "room by none",               "Do not group rooms." },
            { "room order name",            "Order rooms by name." },
            { "room order unread",          "Order rooms by unread messages, and then by name." },
            { "room unread before",         "Show unread message count before room." },
            { "room unread after",          "Show unread message count after room." },
            { "room unread off",            "Do not show unread message count for rooms." },
            { "private room",               "Show room private chats with the room." },
            { "private group",              "Show room private chats as a separate roster group." },
            { "private off",                "Do not show room private chats." },
            { "private char <char>",        "Prefix private room chats with specified character when displayed in separate group." },
            { "private char none",          "Remove private room chat character prefix." },
            { "header char <char>",         "Prefix roster headers with specified character." },
            { "header char none",           "Remove roster header character prefix." },
            { "contact char <char>",        "Prefix roster contacts with specified character." },
            { "contact char none",          "Remove roster contact character prefix." },
            { "contact indent <indent>",    "Indent contact line by <indent> spaces (0 to 10)." },
            { "resource char <char>",       "Prefix roster resources with specified character." },
            { "resource char none",         "Remove roster resource character prefix." },
            { "resource indent <indent>",   "Indent resource line by <indent> spaces (0 to 10)." },
            { "resource join on|off",       "Join resource with previous line when only one available resource." },
            { "presence indent <indent>",   "Indent presence line by <indent> spaces (-1 to 10), a value of -1 will show presence on the previous line." },
            { "size <precent>",             "Percentage of the screen taken up by the roster (1-99)." },
            { "wrap on|off",                "Enable or disable line wrapping in roster panel." },
            { "add <jid> [<nick>]",         "Add a new item to the roster." },
            { "remove <jid>",               "Removes an item from the roster." },
            { "remove_all contacts",        "Remove all items from roster." },
            { "nick <jid> <nick>",          "Change a contacts nickname." },
            { "clearnick <jid>",            "Removes the current nickname." })
        CMD_EXAMPLES(
            "/roster",
            "/roster add someone@contacts.org",
            "/roster add someone@contacts.org Buddy",
            "/roster remove someone@contacts.org",
            "/roster nick myfriend@chat.org My Friend",
            "/roster clearnick kai@server.com",
            "/roster size 15")
    },

    { "/blocked",
        parse_args, 0, 2, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_blocked)
        CMD_TAGS(
            CMD_TAG_ROSTER,
            CMD_TAG_CHAT)
        CMD_SYN(
            "/blocked",
            "/blocked add [<jid>]",
            "/blocked remove <jid>")
        CMD_DESC(
            "Manage blocked users, calling with no arguments shows the current list of blocked users.")
        CMD_ARGS(
            { "add [<jid>]",    "Block the specified Jabber ID. If in a chat window and no jid is specified, the current recipient will be blocked." },
            { "remove <jid>",   "Remove the specified Jabber ID from the blocked list." })
        CMD_EXAMPLES(
            "/blocked add spammer@spam.org")
    },

    { "/group",
        parse_args_with_freetext, 0, 3, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_group)
        CMD_TAGS(
            CMD_TAG_ROSTER,
            CMD_TAG_UI)
        CMD_SYN(
            "/group",
            "/group show <group>",
            "/group add <group> <contat>",
            "/group remove <group> <contact>")
        CMD_DESC(
            "View, add to, and remove from roster groups. "
            "Passing no argument will list all roster groups.")
        CMD_ARGS(
            { "show <group>",             "List all roster items a group." },
            { "add <group> <contact>",    "Add a contact to a group." },
            { "remove <group> <contact>", "Remove a contact from a group." })
        CMD_EXAMPLES(
            "/group",
            "/group show friends",
            "/group add friends newfriend@server.org",
            "/group add family Brother",
            "/group remove colleagues boss@work.com")
    },

    { "/info",
        parse_args, 0, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_info)
        CMD_TAGS(
            CMD_TAG_ROSTER,
            CMD_TAG_CHAT,
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/info",
            "/info <contact>|<nick>")
        CMD_DESC(
            "Show information about a contact, room, or room member. "
            "Passing no argument in a chat window will use the current recipient. "
            "Passing no argument in a chat room will display information about the room.")
        CMD_ARGS(
            { "<contact>", "The contact you wish to view information about." },
            { "<nick>",    "When in a chat room, the occupant you wish to view information about." })
        CMD_EXAMPLES(
            "/info mybuddy@chat.server.org",
            "/info kai")
    },

    { "/caps",
        parse_args, 0, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_caps)
        CMD_TAGS(
            CMD_TAG_DISCOVERY,
            CMD_TAG_CHAT,
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/caps",
            "/caps <fulljid>|<nick>")
        CMD_DESC(
            "Find out a contacts, or room members client software capabilities. "
            "If in private chat initiated from a chat room, no parameter is required.")
        CMD_ARGS(
            { "<fulljid>", "If in the console or a chat window, the full JID for which you wish to see capabilities." },
            { "<nick>",    "If in a chat room, nickname for which you wish to see capabilities." })
        CMD_EXAMPLES(
            "/caps mybuddy@chat.server.org/laptop",
            "/caps mybuddy@chat.server.org/phone",
            "/caps bruce")
    },

    { "/software",
        parse_args, 0, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_software)
        CMD_TAGS(
            CMD_TAG_DISCOVERY,
            CMD_TAG_CHAT,
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/software",
            "/software <fulljid>|<nick>")
        CMD_DESC(
            "Find out a contact, or room members software version information. "
            "If in private chat initiated from a chat room, no parameter is required. "
            "If the contact's software does not support software version requests, nothing will be displayed.")
        CMD_ARGS(
            { "<fulljid>", "If in the console or a chat window, the full JID for which you wish to see software information." },
            { "<nick>",    "If in a chat room, nickname for which you wish to see software information." })
        CMD_EXAMPLES(
            "/software mybuddy@chat.server.org/laptop",
            "/software mybuddy@chat.server.org/phone",
            "/software bruce")
    },

    { "/status",
        parse_args, 0, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_status)
        CMD_TAGS(
            CMD_TAG_CHAT,
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/status",
            "/status <contact>|<nick>")
        CMD_DESC(
            "Find out a contact, or room members presence information. "
            "If in a chat window the parameter is not required, the current recipient will be used.")
        CMD_ARGS(
            { "<contact>", "The contact who's presence you which to see." },
            { "<nick>",    "If in a chat room, the occupant who's presence you wish to see." })
        CMD_EXAMPLES(
            "/status buddy@server.com",
            "/status jon")
    },

    { "/resource",
        parse_args, 1, 2, &cons_resource_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_resource)
        CMD_TAGS(
            CMD_TAG_CHAT,
            CMD_TAG_UI)
        CMD_SYN(
            "/resource set <resource>",
            "/resource off",
            "/resource title on|off",
            "/resource message on|off")
        CMD_DESC(
            "Override chat session resource, and manage resource display settings.")
        CMD_ARGS(
            { "set <resource>", "Set the resource to which messages will be sent." },
            { "off",            "Let the server choose which resource to route messages to." },
            { "title on|off",   "Show or hide the current resource in the titlebar." },
            { "message on|off", "Show or hide the resource when showing an incoming message." })
        CMD_NOEXAMPLES
    },

    { "/join",
        parse_args, 0, 5, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_join)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/join",
            "/join <room> [nick <nick>] [password <password>]")
        CMD_DESC(
            "Join a chat room at the conference server. "
            "If no room is supplied, a generated name will be used with the format private-chat-[UUID]. "
            "If the domain part is not included in the room name, the account preference 'muc.service' will be used. "
            "If no nickname is specified the account preference 'muc.nick' will be used which by default is the localpart of your JID. "
            "If the room doesn't exist, and the server allows it, a new one will be created.")
        CMD_ARGS(
            { "<room>",              "The chat room to join." },
            { "nick <nick>",         "Nickname to use in the room." },
            { "password <password>", "Password if the room requires one." })
        CMD_EXAMPLES(
            "/join",
            "/join jdev@conference.jabber.org",
            "/join jdev@conference.jabber.org nick mynick",
            "/join private@conference.jabber.org nick mynick password mypassword",
            "/join jdev")
    },

    { "/leave",
        parse_args, 0, 0, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_leave)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/leave")
        CMD_DESC(
            "Leave the current chat or room.")
        CMD_NOARGS
        CMD_NOEXAMPLES
    },

    { "/invite",
        parse_args_with_freetext, 1, 2, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_invite)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/invite <contact> [<message>]")
        CMD_DESC(
            "Send an invite to a contact for the current chat room.")
        CMD_ARGS(
            { "<contact>", "The contact you wish to invite." },
            { "<message>", "An optional message to send with the invite." })
        CMD_NOEXAMPLES
    },

    { "/invites",
        parse_args_with_freetext, 0, 0, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_invites)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/invites")
        CMD_DESC(
            "Show all rooms that you have been invited to, and not accepted or declined.")
        CMD_NOARGS
        CMD_NOEXAMPLES
    },

    { "/decline",
        parse_args_with_freetext, 1, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_decline)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/decline <room>")
        CMD_DESC(
            "Decline a chat room invitation.")
        CMD_ARGS(
            { "<room>", "The room for the invite you wish to decline." })
        CMD_NOEXAMPLES
    },

    { "/room",
        parse_args, 1, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_room)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/room accept|destroy|config")
        CMD_DESC(
            "Chat room configuration.")
        CMD_ARGS(
            { "accept",  "Accept default room configuration." },
            { "destroy", "Reject default room configuration, and destroy the room." },
            { "config",  "Edit room configuration." })
        CMD_NOEXAMPLES
    },

    { "/kick",
        parse_args_with_freetext, 1, 2, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_kick)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/kick <nick> [<reason>]")
        CMD_DESC(
            "Kick occupant from chat room.")
        CMD_ARGS(
            { "<nick>",   "Nickname of the occupant to kick from the room." },
            { "<reason>", "Optional reason for kicking the occupant." })
        CMD_NOEXAMPLES
    },

    { "/ban",
        parse_args_with_freetext, 1, 2, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_ban)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/ban <jid> [<reason>]")
        CMD_DESC(
            "Ban user from chat room.")
        CMD_ARGS(
            { "<jid>",    "Bare JID of the user to ban from the room." },
            { "<reason>", "Optional reason for banning the user." })
        CMD_NOEXAMPLES
    },

    { "/subject",
        parse_args_with_freetext, 0, 2, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_subject)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/subject set <subject>",
            "/subject edit <subject>",
            "/subject prepend <text>",
            "/subject append <text>",
            "/subject clear")
        CMD_DESC(
            "Set, modify, or clear room subject.")
        CMD_ARGS(
            { "set <subject>",  "Set the room subject." },
            { "edit <subject>", "Edit the current room subject, tab autocompletion will display the subject to edit." },
            { "prepend <text>", "Prepend text to the current room subject, use double quotes if a trailing space is needed." },
            { "append <text>",  "Append text to the current room subject, use double quotes if a preceding space is needed." },
            { "clear",          "Clear the room subject." })
        CMD_NOEXAMPLES
    },

    { "/affiliation",
        parse_args_with_freetext, 1, 4, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_affiliation)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/affiliation set <affiliation> <jid> [<reason>]",
            "/affiliation list [<affiliation>]")
        CMD_DESC(
            "Manage room affiliations. "
            "Affiliation may be one of owner, admin, member, outcast or none.")
        CMD_ARGS(
            { "set <affiliation> <jid> [<reason>]", "Set the affiliation of user with jid, with an optional reason." },
            { "list [<affiliation>]",               "List all users with the specified affiliation, or all if none specified." })
        CMD_NOEXAMPLES
    },

    { "/role",
        parse_args_with_freetext, 1, 4, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_role)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/role set <role> <nick> [<reason>]",
            "/role list [<role>]")
        CMD_DESC(
            "Manage room roles. "
            "Role may be one of moderator, participant, visitor or none.")
        CMD_ARGS(
            { "set <role> <nick> [<reason>]", "Set the role of occupant with nick, with an optional reason." },
            { "list [<role>]",                "List all occupants with the specified role, or all if none specified." })
        CMD_NOEXAMPLES
    },

    { "/occupants",
        parse_args, 1, 3, cons_occupants_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_occupants)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT,
            CMD_TAG_UI)
        CMD_SYN(
            "/occupants show|hide [jid]",
            "/occupants default show|hide [jid]",
            "/occupants size [<percent>]")
        CMD_DESC(
            "Show or hide room occupants, and occupants panel display settings.")
        CMD_ARGS(
            { "show",                  "Show the occupants panel in current room." },
            { "hide",                  "Hide the occupants panel in current room." },
            { "show jid",              "Show jid in the occupants panel in current room." },
            { "hide jid",              "Hide jid in the occupants panel in current room." },
            { "default show|hide",     "Whether occupants are shown by default in new rooms." },
            { "default show|hide jid", "Whether occupants jids are shown by default in new rooms." },
            { "size <percent>",        "Percentage of the screen taken by the occupants list in rooms (1-99)." })
        CMD_NOEXAMPLES
    },

    { "/form",
        parse_args, 1, 2, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_form)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/form show",
            "/form submit",
            "/form cancel",
            "/form help [<tag>]")
        CMD_DESC(
            "Form configuration.")
        CMD_ARGS(
            { "show",         "Show the current form." },
            { "submit",       "Submit the current form." },
            { "cancel",       "Cancel changes to the current form." },
            { "help [<tag>]", "Display help for form, or a specific field." })
        CMD_NOEXAMPLES
    },

    { "/rooms",
        parse_args, 0, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_rooms)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/rooms [<service>]")
        CMD_DESC(
            "List the chat rooms available at the specified conference service. "
            "If no argument is supplied, the account preference 'muc.service' is used, 'conference.<domain-part>' by default.")
        CMD_ARGS(
            { "<service>", "The conference service to query." })
        CMD_EXAMPLES(
            "/rooms conference.jabber.org")
    },

    { "/bookmark",
        parse_args, 0, 8, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_bookmark)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/bookmark",
            "/bookmark list",
            "/bookmark add [<room>] [nick <nick>] [password <password>] [autojoin on|off]",
            "/bookmark update <room> [nick <nick>] [password <password>] [autojoin on|off]",
            "/bookmark remove [<room>]",
            "/bookmark join <room>",
            "/bookmark invites on|off")
        CMD_DESC(
            "Manage bookmarks and join bookmarked rooms. "
            "In a chat room, no arguments will bookmark the current room, setting autojoin to \"on\".")
        CMD_ARGS(
            { "list", "List all bookmarks." },
            { "add [<room>]", "Add a bookmark, passing no room will bookmark the current room, setting autojoin to \"on\"." },
            { "remove [<room>]", "Remove a bookmark, passing no room will remove the bookmark for the current room, if one exists." },
            { "update <room>", "Update the properties associated with a bookmark." },
            { "nick <nick>", "Nickname used in the chat room." },
            { "password <password>", "Password if required, may be stored in plaintext on your server." },
            { "autojoin on|off", "Whether to join the room automatically on login." },
            { "join <room>", "Join room using the properties associated with the bookmark." },
            { "invites on|off", "Whether or not to bookmark accepted room invites, defaults to 'on'."})
        CMD_NOEXAMPLES
    },

    { "/disco",
        parse_args, 1, 2, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_disco)
        CMD_TAGS(
            CMD_TAG_DISCOVERY)
        CMD_SYN(
            "/disco info [<jid>]",
            "/disco items [<jid>]")
        CMD_DESC(
            "Find out information about an entities supported services. "
            "Calling with no arguments will query the server you are currently connected to.")
        CMD_ARGS(
            { "info [<jid>]", "List protocols and features supported by an entity." },
            { "items [<jid>]", "List items associated with an entity." })
        CMD_EXAMPLES(
            "/disco info",
            "/disco items myserver.org",
            "/disco items conference.jabber.org",
            "/disco info myfriend@server.com/laptop")
    },

    { "/sendfile",
        parse_args_with_freetext, 1, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_sendfile)
        CMD_TAGS(
            CMD_TAG_CHAT,
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/sendfile <file>")
        CMD_DESC(
            "Send a file using XEP-0363 HTTP file transfer.")
        CMD_ARGS(
            { "<file>", "Path to the file." })
        CMD_EXAMPLES(
            "/sendfile /etc/hosts",
            "/sendfile ~/images/sweet_cat.jpg")
    },

    { "/lastactivity",
        parse_args, 0, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_lastactivity)
        CMD_TAGS(
            CMD_TAG_PRESENCE)
        CMD_SYN(
            "/lastactivity on|off",
            "/lastactivity [<jid>]")
        CMD_DESC(
            "Enable/disable sending last activity, and send last activity requests.")
        CMD_ARGS(
            { "on|off", "Enable or disable sending of last activity." },
            { "<jid>",  "The JID of the entity to query, omitting the JID will query your server." })
        CMD_EXAMPLES(
            "/lastactivity",
            "/lastactivity off",
            "/lastactivity alice@securechat.org",
            "/lastactivity alice@securechat.org/laptop",
            "/lastactivity someserver.com")
    },

    { "/nick",
        parse_args_with_freetext, 1, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_nick)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/nick <nickname>")
        CMD_DESC(
            "Change your nickname in the current chat room.")
        CMD_ARGS(
            { "<nickname>", "Your new nickname." })
        CMD_NOEXAMPLES
    },

    { "/win",
        parse_args, 1, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_win)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/win console",
            "/win <num>",
            "/win <barejid>",
            "/win <nick>",
            "/win <roomjid>",
            "/win <roomoccupantjid>",
            "/win xmlconsole",
            "/win <plugin>")
        CMD_DESC(
            "Move to the specified window.")
        CMD_ARGS(
            { "console",            "Focus the Console window." },
            { "<num>",              "Focus specified window number." },
            { "<barejid>",          "Focus chat window with contact by JID if open." },
            { "<nick>",             "Focus chat window with contact by nickname if open." },
            { "<roomjid>",          "Focus chat room window with roomjid if open." },
            { "<roomoccupantjid>",  "Focus private chat roomoccupantjid if open." },
            { "xmlconsole",         "Focus the XML Console window if open." },
            { "<plugin>",           "Focus the plugin window."})
        CMD_EXAMPLES(
            "/win console",
            "/win 4",
            "/win friend@chat.org",
            "/win Eddie",
            "/win bigroom@conference.chat.org",
            "/win bigroom@conference.chat.org/bruce",
            "/win wikipedia")
    },

    { "/wins",
        parse_args, 0, 3, NULL,
        CMD_SUBFUNCS(
            { "unread",     cmd_wins_unread },
            { "tidy",       cmd_wins_tidy },
            { "prune",      cmd_wins_prune },
            { "swap",       cmd_wins_swap },
            { "autotidy",   cmd_wins_autotidy })
        CMD_MAINFUNC(cmd_wins)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/wins",
            "/wins unread",
            "/wins tidy",
            "/wins autotidy on|off",
            "/wins prune",
            "/wins swap <source> <target>")
        CMD_DESC(
            "Manage windows. "
            "Passing no argument will list all currently active windows and information about their usage.")
        CMD_ARGS(
            { "unread",                 "List windows with unread messages." },
            { "tidy",                   "Move windows so there are no gaps." },
            { "autotidy on|off",        "Automatically remove gaps when closing windows." },
            { "prune",                  "Close all windows with no unread messages, and then tidy so there are no gaps." },
            { "swap <source> <target>", "Swap windows, target may be an empty position." })
        CMD_NOEXAMPLES
    },

    { "/sub",
        parse_args, 1, 2, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_sub)
        CMD_TAGS(
            CMD_TAG_ROSTER)
        CMD_SYN(
            "/sub request [<jid>]",
            "/sub allow [<jid>]",
            "/sub deny [<jid>]",
            "/sub show [<jid>]",
            "/sub sent",
            "/sub received")
        CMD_DESC(
            "Manage subscriptions to contact presence. "
            "If jid is omitted, the contact of the current window is used.")
        CMD_ARGS(
            { "request [<jid>]", "Send a subscription request to the user." },
            { "allow [<jid>]",   "Approve a contact's subscription request." },
            { "deny [<jid>]",    "Remove subscription for a contact, or deny a request." },
            { "show [<jid>]",    "Show subscription status for a contact." },
            { "sent",            "Show all sent subscription requests pending a response." },
            { "received",        "Show all received subscription requests awaiting your response." })
        CMD_EXAMPLES(
            "/sub request myfriend@jabber.org",
            "/sub allow myfriend@jabber.org",
            "/sub request",
            "/sub sent")
    },

    { "/tiny",
        parse_args, 1, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_tiny)
        CMD_TAGS(
            CMD_TAG_CHAT,
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/tiny <url>")
        CMD_DESC(
            "Send url as tinyurl in current chat.")
        CMD_ARGS(
            { "<url>", "The url to make tiny." })
        CMD_EXAMPLES(
            "Example: /tiny http://www.profanity.im")
    },

    { "/who",
        parse_args, 0, 2, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_who)
        CMD_TAGS(
            CMD_TAG_CHAT,
            CMD_TAG_GROUPCHAT,
            CMD_TAG_ROSTER)
        CMD_SYN(
            "/who",
            "/who online|offline|away|dnd|xa|chat|available|unavailable|any [<group>]",
            "/who moderator|participant|visitor",
            "/who owner|admin|member")
        CMD_DESC(
            "Show contacts or room occupants with chosen status, role or affiliation")
        CMD_ARGS(
            { "offline|away|dnd|xa|chat", "Show contacts or room occupants with specified presence." },
            { "online", "Contacts that are online, chat, away, xa, dnd." },
            { "available", "Contacts that are available for chat - online, chat." },
            { "unavailable", "Contacts that are not available for chat - offline, away, xa, dnd." },
            { "any", "Contacts with any status (same as calling with no argument)." },
            { "<group>", "Filter the results by the specified roster group, not applicable in chat rooms." },
            { "moderator|participant|visitor", "Room occupants with the specified role." },
            { "owner|admin|member", "Room occupants with the specified affiliation." })
        CMD_EXAMPLES(
            "/who",
            "/who xa",
            "/who online friends",
            "/who any family",
            "/who participant",
            "/who admin")
    },

    { "/close",
        parse_args, 0, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_close)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/close",
            "/close <num>",
            "/close <barejid>",
            "/close <nick>",
            "/close <roomjid>",
            "/close <roomoccupantjid>",
            "/close xmlconsole",
            "/close all|read")
        CMD_DESC(
            "Close windows. "
            "Passing no argument closes the current window.")
        CMD_ARGS(
            { "<num>",              "Close specified window number." },
            { "<barejid>",          "Close chat window with contact by JID if open." },
            { "<nick>",             "Close chat window with contact by nickname if open." },
            { "<roomjid>",          "Close chat room window with roomjid if open." },
            { "<roomoccupantjid>",  "Close private chat roomoccupantjid if open." },
            { "xmlconsole",         "Close the XML Console window if open." },
            { "all",                "Close all windows." },
            { "read",               "Close all windows that have no unread messages." })
        CMD_NOEXAMPLES
    },

    { "/clear",
        parse_args, 0, 0, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_clear)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/clear")
        CMD_DESC(
            "Clear the current window.")
        CMD_NOARGS
        CMD_NOEXAMPLES
    },

    { "/quit",
        parse_args, 0, 0, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_quit)
        CMD_NOTAGS
        CMD_SYN(
            "/quit")
        CMD_DESC(
            "Logout of any current session, and quit Profanity.")
        CMD_NOARGS
        CMD_NOEXAMPLES
    },

    { "/privileges",
        parse_args, 1, 1, &cons_privileges_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_privileges)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT,
            CMD_TAG_UI)
        CMD_SYN(
            "/privileges on|off")
        CMD_DESC(
            "Group occupants panel by role, and show role information in chat rooms.")
        CMD_ARGS(
            { "on|off", "Enable or disable privilege information." })
        CMD_NOEXAMPLES
    },

    { "/charset",
        parse_args, 0, 0, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_charset)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/charset")
        CMD_DESC(
            "Display information about the current character set supported by the terminal. ")
        CMD_NOARGS
        CMD_NOEXAMPLES
    },

    { "/beep",
        parse_args, 1, 1, &cons_beep_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_beep)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/beep on|off")
        CMD_DESC(
            "Switch the terminal bell on or off. "
            "The bell will sound when incoming messages are received. "
            "If the terminal does not support sounds, it may attempt to flash the screen instead.")
        CMD_ARGS(
            { "on|off", "Enable or disable terminal bell." })
        CMD_NOEXAMPLES
    },

    { "/console",
        parse_args, 2, 2, &cons_console_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_console)
        CMD_TAGS(
            CMD_TAG_UI,
            CMD_TAG_CHAT,
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/console chat all|first|none",
            "/console muc all|first|none",
            "/console private all|first|none")
        CMD_DESC(
            "Configure what is displayed in the console window when messages are received. "
            "The default is set to 'all' for all types of messages.")
        CMD_ARGS(
            { "chat all",       "Indicate all new chat messages in the console." },
            { "chat first",     "Indicate only the first new message per chat in the console." },
            { "chat none",      "Do not show any new chat messages in the console window." },
            { "muc all",        "Indicate all new chat room messages in the console." },
            { "muc first",      "Indicate only the first new message in each room in the console." },
            { "muc none",       "Do not show any new chat room messages in the console window." },
            { "private all",    "Indicate all new private room messages in the console." },
            { "private first",  "Indicate only the first private room message in the console." },
            { "private none",   "Do not show any new private room messages in the console window." })
        CMD_NOEXAMPLES
    },

    { "/encwarn",
        parse_args, 1, 1, &cons_encwarn_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_encwarn)
        CMD_TAGS(
            CMD_TAG_CHAT,
            CMD_TAG_UI)
        CMD_SYN(
            "/encwarn on|off")
        CMD_DESC(
            "Titlebar encryption warning.")
        CMD_ARGS(
            { "on|off", "Enable or disable the unencrypted warning message in the titlebar." })
        CMD_NOEXAMPLES
    },

    { "/presence",
        parse_args, 2, 2, &cons_presence_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_presence)
        CMD_TAGS(
            CMD_TAG_UI,
            CMD_TAG_CHAT,
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/presence titlebar on|off",
            "/presence console all|online|none",
            "/presence chat all|online|none",
            "/presence room all|online|none")
        CMD_DESC(
            "Show the contacts presence in the titlebar and configure presence messages in different window types.")
        CMD_ARGS(
            { "titlebar on|off", "Switch display of the contacts presence in the titlebar on or off." },
            { "console all",     "Show all presence changes in the console window." },
            { "console online",  "Show only online/offline presence changes in the console window." },
            { "console none",    "Don't show any presence changes in the console window." },
            { "chat all",        "Show all presence changes in the chat windows." },
            { "chat online",     "Show only online/offline presence changes in chat windows." },
            { "chat none",       "Don't show any presence changes in chat windows." },
            { "room all",        "Show all presence changes in chat room windows." },
            { "room online",     "Show only online/offline presence changes in chat room windows." },
            { "room none",       "Don't show any presence changes in chat room windows." })
        CMD_EXAMPLES(
            "/presence titlebar off",
            "/presence console none",
            "/presence chat online",
            "/presence room all")
    },

    { "/wrap",
        parse_args, 1, 1, &cons_wrap_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_wrap)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/wrap on|off")
        CMD_DESC(
            "Word wrapping.")
        CMD_ARGS(
            { "on|off", "Enable or disable word wrapping in the main window." })
        CMD_NOEXAMPLES
    },

    { "/time",
        parse_args, 1, 3, &cons_time_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_time)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/time console|chat|muc|mucconfig|private|xml set <format>",
            "/time console|chat|muc|mucconfig|private|xml off",
            "/time statusbar set <format>",
            "/time statusbar off",
            "/time lastactivity set <format>")
        CMD_DESC(
            "Configure time display preferences. "
            "Time formats are strings supported by g_date_time_format. "
            "See https://developer.gnome.org/glib/stable/glib-GDateTime.html#g-date-time-format for more details. "
            "Setting the format to an unsupported string, will display the string. "
            "If the format contains spaces, it must be surrounded with double quotes.")
        CMD_ARGS(
            { "console set <format>",      "Set time format for console window." },
            { "console off",               "Do not show time in console window." },
            { "chat set <format>",         "Set time format for chat windows." },
            { "chat off",                  "Do not show time in chat windows." },
            { "muc set <format>",          "Set time format for chat room windows." },
            { "muc off",                   "Do not show time in chat room windows." },
            { "mucconfig set <format>",    "Set time format for chat room config windows." },
            { "mucconfig off",             "Do not show time in chat room config windows." },
            { "private set <format>",      "Set time format for private chat windows." },
            { "private off",               "Do not show time in private chat windows." },
            { "xml set <format>",          "Set time format for XML console window." },
            { "xml off",                   "Do not show time in XML console window." },
            { "statusbar set <format>",    "Change time format in statusbar." },
            { "statusbar off",             "Do not show time in status bar." },
            { "lastactivity set <format>", "Change time format for last activity." })
        CMD_EXAMPLES(
            "/time console set %H:%M:%S",
            "/time chat set \"%d-%m-%y %H:%M:%S\"",
            "/time xml off",
            "/time statusbar set %H:%M",
            "/time lastactivity set \"%d-%m-%y %H:%M:%S\"")
    },

    { "/inpblock",
        parse_args, 2, 2, &cons_inpblock_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_inpblock)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/inpblock timeout <millis>",
            "/inpblock dynamic on|off")
        CMD_DESC(
            "How long to wait for keyboard input before checking for new messages or checking for state changes such as 'idle'.")
        CMD_ARGS(
            { "timeout <millis>", "Time to wait (1-1000) in milliseconds before reading input from the terminal buffer, default: 1000." },
            { "dynamic on|off", "Start with 0 millis and dynamically increase up to timeout when no activity, default: on." })
        CMD_NOEXAMPLES
    },

    { "/titlebar",
        parse_args, 1, 1, &cons_winpos_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_titlebar)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/titlebar up",
            "/titlebar down")
        CMD_DESC(
            "Move the title bar.")
        CMD_ARGS(
            { "up", "Move the title bar up the screen." },
            { "down", "Move the title bar down the screen." })
        CMD_NOEXAMPLES
    },

    { "/mainwin",
        parse_args, 1, 1, &cons_winpos_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_mainwin)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/mainwin up",
            "/mainwin down")
        CMD_DESC(
            "Move the main window.")
        CMD_ARGS(
            { "up", "Move the main window up the screen." },
            { "down", "Move the main window down the screen." })
        CMD_NOEXAMPLES
    },

    { "/statusbar",
        parse_args, 1, 1, &cons_winpos_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_statusbar)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/statusbar up",
            "/statusbar down")
        CMD_DESC(
            "Move the status bar.")
        CMD_ARGS(
            { "up", "Move the status bar up the screen." },
            { "down", "Move the status bar down the screen." })
        CMD_NOEXAMPLES
    },

    { "/inputwin",
        parse_args, 1, 1, &cons_winpos_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_inputwin)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/inputwin up",
            "/inputwin down")
        CMD_DESC(
            "Move the input window.")
        CMD_ARGS(
            { "up", "Move the input window up the screen." },
            { "down", "Move the input window down the screen." })
        CMD_NOEXAMPLES
    },

    { "/notify",
        parse_args_with_freetext, 0, 4, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_notify)
        CMD_TAGS(
            CMD_TAG_UI,
            CMD_TAG_CHAT,
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/notify chat on|off",
            "/notify chat current on|off",
            "/notify chat text on|off",
            "/notify room on|off",
            "/notify room mention on|off",
            "/notify room mention case_sensitive|case_insensitive",
            "/notify room mention word_whole|word_part",
            "/notify room current on|off",
            "/notify room text on|off",
            "/notify room trigger add <text>",
            "/notify room trigger remove <text>",
            "/notify room trigger list",
            "/notify room trigger on|off",
            "/notify on|off",
            "/notify mention on|off",
            "/notify trigger on|off",
            "/notify reset",
            "/notify remind <seconds>",
            "/notify typing on|off",
            "/notify typing current on|off",
            "/notify invite on|off",
            "/notify sub on|off")
        CMD_DESC(
            "Configure desktop notifications. "
            "To configure presence update messages in the console, chat and chat room windows, see '/help presence'.")
        CMD_ARGS(
            { "chat on|off",                    "Notifications for regular chat messages." },
            { "chat current on|off",            "Whether to show regular chat message notifications when the window is focussed." },
            { "chat text on|off",               "Show message text in regular message notifications." },
            { "room on|off",                    "Notifications for all chat room messages." },
            { "room mention on|off",            "Notifications for chat room messages when your nick is mentioned." },
            { "room mention case_sensitive",    "Set room mention notifications as case sensitive." },
            { "room mention case_insensitive",  "Set room mention notifications as case insensitive." },
            { "room mention word_whole",        "Set room mention notifications only on whole word match, i.e. when nickname is not part of a larger word." },
            { "room mention word_part",         "Set room mention notifications on partial word match, i.e. nickname may be part of a larger word." },
            { "room current on|off",            "Whether to show all chat room messages notifications when the window is focussed." },
            { "room text on|off",               "Show message text in chat room message notifications." },
            { "room trigger add <text>",        "Notify when specified text included in all chat room messages." },
            { "room trigger remove <text>",     "Remove chat room notification trigger." },
            { "room trigger list",              "List all chat room triggers." },
            { "room trigger on|off",            "Enable or disable all chat room notification triggers." },
            { "on|off",                         "Override the global message setting for the current chat room." },
            { "mention on|off",                 "Override the global 'mention' setting for the current chat room." },
            { "trigger on|off",                 "Override the global 'trigger' setting for the current chat room." },
            { "reset",                          "Reset to global notification settings for the current chat room." },
            { "remind <seconds>",               "Notification reminder period for unread messages, use 0 to disable." },
            { "typing on|off",                  "Notifications when contacts are typing." },
            { "typing current on|off",          "Whether typing notifications are triggered for the current window." },
            { "invite on|off",                  "Notifications for chat room invites." },
            { "sub on|off",                     "Notifications for subscription requests." })
        CMD_EXAMPLES(
            "/notify chat on",
            "/notify chat text on",
            "/notify room mention on",
            "/notify room trigger add beer",
            "/notify room trigger on",
            "/notify room current off",
            "/notify room text off",
            "/notify remind 60",
            "/notify typing on",
            "/notify invite on")
    },

    { "/flash",
        parse_args, 1, 1, &cons_flash_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_flash)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/flash on|off")
        CMD_DESC(
            "Make the terminal flash when incoming messages are received in another window. "
            "If the terminal doesn't support flashing, it may attempt to beep.")
        CMD_ARGS(
            { "on|off", "Enable or disable terminal flash." })
        CMD_NOEXAMPLES
    },

    { "/tray",
        parse_args, 1, 2, &cons_tray_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_tray)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/tray on|off",
            "/tray read on|off",
            "/tray timer <seconds>")
        CMD_DESC(
            "Display an icon in the tray that will indicate new messages.")
        CMD_ARGS(
            { "on|off",             "Show tray icon." },
            { "read on|off",        "Show tray icon when no unread messages." },
            { "timer <seconds>",    "Set tray icon timer, seconds must be between 1-10" })
        CMD_NOEXAMPLES
    },

    { "/intype",
        parse_args, 1, 1, &cons_intype_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_intype)
        CMD_TAGS(
            CMD_TAG_UI,
            CMD_TAG_CHAT)
        CMD_SYN(
            "/intype on|off")
        CMD_DESC(
            "Show when a contact is typing in the console, and in active message window.")
        CMD_ARGS(
            { "on|off", "Enable or disable contact typing messages." })
        CMD_NOEXAMPLES
    },

    { "/splash",
        parse_args, 1, 1, &cons_splash_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_splash)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/splash on|off")
        CMD_DESC(
            "Switch on or off the ascii logo on start up and when the /about command is called.")
        CMD_ARGS(
            { "on|off", "Enable or disable splash logo." })
        CMD_NOEXAMPLES
    },

    { "/autoconnect",
        parse_args, 1, 2, &cons_autoconnect_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_autoconnect)
        CMD_TAGS(
            CMD_TAG_CONNECTION)
        CMD_SYN(
            "/autoconnect set <account>",
            "/autoconnect off")
        CMD_DESC(
            "Enable or disable autoconnect on start up. "
            "The setting can be overridden by the -a (--account) command line option.")
        CMD_ARGS(
            { "set <account>", "Connect with account on start up." },
            { "off",           "Disable autoconnect." })
        CMD_EXAMPLES(
            "/autoconnect set jc@stuntteam.org",
            "/autoconnect off")
    },

    { "/vercheck",
        parse_args, 0, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_vercheck)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/vercheck on|off")
        CMD_DESC(
            "Check for new versions when Profanity starts, and when the /about command is run.")
        CMD_ARGS(
            { "on|off", "Enable or disable the version check." })
        CMD_NOEXAMPLES
    },

    { "/wintitle",
        parse_args, 2, 2, &cons_wintitle_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_wintitle)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/wintitle show on|off",
            "/wintitle goodbye on|off")
        CMD_DESC(
            "Allow Profanity to modify the window title bar.")
        CMD_ARGS(
            { "show on|off",    "Show current logged in user, and unread messages as the window title." },
            { "goodbye on|off", "Show a message in the title when exiting profanity." })
        CMD_NOEXAMPLES
    },

    { "/alias",
        parse_args_with_freetext, 1, 3, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_alias)
        CMD_NOTAGS
        CMD_SYN(
            "/alias list",
            "/alias add <name> <value>",
            "/alias remove <name>")
        CMD_DESC(
            "Add, remove or list command aliases.")
        CMD_ARGS(
            { "list",               "List all aliases." },
            { "add <name> <value>", "Add a new command alias." },
            { "remove <name>",      "Remove a command alias." })
        CMD_EXAMPLES(
            "/alias add friends /who online friends",
            "/alias add /q /quit",
            "/alias add a /away \"I'm in a meeting.\"",
            "/alias remove q",
            "/alias list")
    },

    { "/chlog",
        parse_args, 1, 1, &cons_chlog_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_chlog)
        CMD_TAGS(
            CMD_TAG_CHAT)
        CMD_SYN(
            "/chlog on|off")
        CMD_DESC(
            "Switch chat logging on or off. "
            "This setting will be enabled if /history is set to on. "
            "When disabling this option, /history will also be disabled. "
            "See the /grlog setting for enabling logging of chat room (groupchat) messages.")
        CMD_ARGS(
            { "on|off", "Enable or disable chat logging." })
        CMD_NOEXAMPLES
    },

    { "/grlog",
        parse_args, 1, 1, &cons_grlog_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_grlog)
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/grlog on|off")
        CMD_DESC(
            "Switch chat room logging on or off. "
            "See the /chlog setting for enabling logging of one to one chat.")
        CMD_ARGS(
            { "on|off", "Enable or disable chat room logging." })
        CMD_NOEXAMPLES
    },

    { "/states",
        parse_args, 1, 1, &cons_states_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_states)
        CMD_TAGS(
            CMD_TAG_CHAT)
        CMD_SYN(
            "/states on|off")
        CMD_DESC(
            "Send chat state notifications to recipient during chat sessions, such as typing, paused, active, gone.")
        CMD_ARGS(
            { "on|off", "Enable or disable sending of chat state notifications." })
        CMD_NOEXAMPLES
    },

    { "/pgp",
        parse_args, 1, 3, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_pgp)
        CMD_TAGS(
            CMD_TAG_CHAT,
            CMD_TAG_UI)
        CMD_SYN(
            "/pgp libver",
            "/pgp keys",
            "/pgp contacts",
            "/pgp setkey <contact> <keyid>",
            "/pgp start [<contact>]",
            "/pgp end",
            "/pgp log on|off|redact",
            "/pgp char <char>")
        CMD_DESC(
            "Open PGP commands to manage keys, and perform PGP encryption during chat sessions. "
            "See the /account command to set your own PGP key.")
        CMD_ARGS(
            { "libver",                   "Show which version of the libgpgme library is being used." },
            { "keys",                     "List all keys known to the system." },
            { "contacts",                 "Show contacts with assigned public keys." },
            { "setkey <contact> <keyid>", "Manually associate a contact with a public key." },
            { "start [<contact>]",        "Start PGP encrypted chat, current contact will be used if not specified." },
            { "end",                      "End PGP encrypted chat with the current recipient." },
            { "log on|off",               "Enable or disable plaintext logging of PGP encrypted messages." },
            { "log redact",               "Log PGP encrypted messages, but replace the contents with [redacted]. This is the default." },
            { "char <char>",              "Set the character to be displayed next to PGP encrypted messages." })
        CMD_EXAMPLES(
            "/pgp log off",
            "/pgp setkey buddy@buddychat.org BA19CACE5A9592C5",
            "/pgp start buddy@buddychat.org",
            "/pgp end",
            "/pgp char P")
    },

    { "/otr",
        parse_args, 1, 3, NULL,
        CMD_SUBFUNCS(
            { "char",       cmd_otr_char },
            { "log",        cmd_otr_log },
            { "libver",     cmd_otr_libver },
            { "policy",     cmd_otr_policy },
            { "gen",        cmd_otr_gen },
            { "myfp",       cmd_otr_myfp },
            { "theirfp",    cmd_otr_theirfp },
            { "start",      cmd_otr_start },
            { "end",        cmd_otr_end },
            { "trust",      cmd_otr_trust },
            { "untrust",    cmd_otr_untrust },
            { "secret",     cmd_otr_secret },
            { "question",   cmd_otr_question },
            { "answer",     cmd_otr_answer })
        CMD_NOMAINFUNC
        CMD_TAGS(
            CMD_TAG_CHAT,
            CMD_TAG_UI)
        CMD_SYN(
            "/otr libver",
            "/otr gen",
            "/otr myfp|theirfp",
            "/otr start [<contact>]",
            "/otr end",
            "/otr trust|untrust",
            "/otr secret <secret>",
            "/otr question <question> <answer>",
            "/otr answer <answer>",
            "/otr policy manual|opportunistic|always [<contact>]",
            "/otr log on|off|redact",
            "/otr char <char>")
        CMD_DESC(
            "Off The Record (OTR) commands to manage keys, and perform OTR encryption during chat sessions.")
        CMD_ARGS(
            { "libver",                         "Show which version of the libotr library is being used." },
            { "gen",                            "Generate your private key." },
            { "myfp",                           "Show your fingerprint." },
            { "theirfp",                        "Show contacts fingerprint." },
            { "start [<contact>]",              "Start an OTR session with contact, or current recipient if omitted." },
            { "end",                            "End the current OTR session," },
            { "trust|untrust",                  "Indicate whether or not you trust the contact's fingerprint." },
            { "secret <secret>",                "Verify a contact's identity using a shared secret." },
            { "question <question> <answer>",   "Verify a contact's identity using a question and expected answer." },
            { "answer <answer>",                "Respond to a question answer verification request with your answer." },
            { "policy manual",                  "Set the global OTR policy to manual, OTR sessions must be started manually." },
            { "policy manual <contact>",        "Set the OTR policy to manual for a specific contact." },
            { "policy opportunistic",           "Set the global OTR policy to opportunistic, an OTR session will be attempted upon starting a conversation." },
            { "policy opportunistic <contact>", "Set the OTR policy to opportunistic for a specific contact." },
            { "policy always",                  "Set the global OTR policy to always, an error will be displayed if an OTR session cannot be initiated upon starting a conversation." },
            { "policy always <contact>",        "Set the OTR policy to always for a specific contact." },
            { "log on|off",                     "Enable or disable plaintext logging of OTR encrypted messages." },
            { "log redact",                     "Log OTR encrypted messages, but replace the contents with [redacted]. This is the default." },
            { "char <char>",                    "Set the character to be displayed next to OTR encrypted messages." })
        CMD_EXAMPLES(
            "/otr log off",
            "/otr policy manual",
            "/otr policy opportunistic mrfriend@workchat.org",
            "/otr gen",
            "/otr start buddy@buddychat.org",
            "/otr myfp",
            "/otr theirfp",
            "/otr question \"What is the name of my rabbit?\" fiffi",
            "/otr end",
            "/otr char *")
    },

    { "/outtype",
        parse_args, 1, 1, &cons_outtype_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_outtype)
        CMD_TAGS(
            CMD_TAG_CHAT)
        CMD_SYN(
            "/outtype on|off")
        CMD_DESC(
            "Send typing notifications, chat states (/states) will be enabled if this setting is enabled.")
        CMD_ARGS(
            { "on|off", "Enable or disable sending typing notifications." })
        CMD_NOEXAMPLES
    },

    { "/gone",
        parse_args, 1, 1, &cons_gone_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_gone)
        CMD_TAGS(
            CMD_TAG_CHAT)
        CMD_SYN(
            "/gone <minutes>")
        CMD_DESC(
            "Send a 'gone' state to the recipient after the specified number of minutes. "
            "Chat states (/states) will be enabled if this setting is set.")
        CMD_ARGS(
            { "<minutes>", "Number of minutes of inactivity before sending the 'gone' state, a value of 0 will disable sending this state." })
        CMD_NOEXAMPLES
    },

    { "/history",
        parse_args, 1, 1, &cons_history_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_history)
        CMD_TAGS(
            CMD_TAG_UI,
            CMD_TAG_CHAT)
        CMD_SYN(
            "/history on|off")
        CMD_DESC(
            "Switch chat history on or off, /chlog will automatically be enabled when this setting is on. "
            "When history is enabled, previous messages are shown in chat windows.")
        CMD_ARGS(
            { "on|off", "Enable or disable showing chat history." })
        CMD_NOEXAMPLES
    },

    { "/log",
        parse_args, 1, 2, &cons_log_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_log)
        CMD_NOTAGS
        CMD_SYN(
            "/log where",
            "/log rotate on|off",
            "/log maxsize <bytes>",
            "/log shared on|off")
        CMD_DESC(
            "Manage profanity log settings.")
        CMD_ARGS(
            { "where",           "Show the current log file location." },
            { "rotate on|off",   "Rotate log, default on." },
            { "maxsize <bytes>", "With rotate enabled, specifies the max log size, defaults to 1048580 (1MB)." },
            { "shared on|off",   "Share logs between all instances, default: on. When off, the process id will be included in the log filename." })
        CMD_NOEXAMPLES
    },

    { "/carbons",
        parse_args, 1, 1, &cons_carbons_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_carbons)
        CMD_TAGS(
            CMD_TAG_CHAT)
        CMD_SYN(
            "/carbons on|off")
        CMD_DESC(
            "Enable or disable message carbons. "
            "Message carbons ensure that both sides of all conversations are shared with all the user's clients that implement this protocol.")
        CMD_ARGS(
            { "on|off", "Enable or disable message carbons." })
        CMD_NOEXAMPLES
    },

    { "/receipts",
        parse_args, 2, 2, &cons_receipts_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_receipts)
        CMD_TAGS(
            CMD_TAG_CHAT)
        CMD_SYN(
            "/receipts request on|off",
            "/receipts send on|off")
        CMD_DESC(
            "Enable or disable message delivery receipts. The interface will indicate when a message has been received.")
        CMD_ARGS(
            { "request on|off", "Whether or not to request a receipt upon sending a message." },
            { "send on|off",    "Whether or not to send a receipt if one has been requested with a received message." })
        CMD_NOEXAMPLES
    },

    { "/reconnect",
        parse_args, 1, 1, &cons_reconnect_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_reconnect)
        CMD_TAGS(
            CMD_TAG_CONNECTION)
        CMD_SYN(
            "/reconnect <seconds>")
        CMD_DESC(
            "Set the reconnect attempt interval for when the connection is lost.")
        CMD_ARGS(
            { "<seconds>", "Number of seconds before attempting to reconnect, a value of 0 disables reconnect." })
        CMD_NOEXAMPLES
    },

    { "/autoping",
        parse_args, 2, 2, &cons_autoping_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_autoping)
        CMD_TAGS(
            CMD_TAG_CONNECTION)
        CMD_SYN(
            "/autoping set <seconds>",
            "/autoping timeout <seconds>")
        CMD_DESC(
            "Set the interval between sending ping requests to the server to ensure the connection is kept alive.")
        CMD_ARGS(
            { "set <seconds>",      "Number of seconds between sending pings, a value of 0 disables autoping." },
            { "timeout <seconds>",  "Seconds to wait for autoping responses, after which the connection is considered broken." })
        CMD_NOEXAMPLES
    },

    { "/ping",
        parse_args, 0, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_ping)
        CMD_TAGS(
            CMD_TAG_CONNECTION)
        CMD_SYN(
            "/ping [<jid>]")
        CMD_DESC(
            "Sends an IQ ping stanza to the specified JID. "
            "If no JID is supplied, your chat server will be pinged.")
        CMD_ARGS(
            { "<jid>", "The Jabber ID to send the ping request to." })
        CMD_NOEXAMPLES
    },

    { "/autoaway",
        parse_args_with_freetext, 2, 3, &cons_autoaway_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_autoaway)
        CMD_TAGS(
            CMD_TAG_PRESENCE)
        CMD_SYN(
            "/autoaway mode idle|away|off",
            "/autoaway time away|xa <minutes>",
            "/autoaway message away|xa <message>|off",
            "/autoaway check on|off")
        CMD_DESC(
            "Manage autoway settings for idle time.")
        CMD_ARGS(
            { "mode idle",              "Sends idle time, status remains online." },
            { "mode away",              "Sends away and xa presence as well as idle time." },
            { "mode off",               "Disabled (default)." },
            { "time away <minutes>",    "Number of minutes before the away presence is sent, default: 15." },
            { "time xa <minutes>",      "Number of minutes before the xa presence is sent, default: 0 (disabled)." },
            { "message away <message>", "Optional message to send with the away presence, default: off (disabled)." },
            { "message xa <message>",   "Optional message to send with the xa presence, default: off (disabled)." },
            { "message away off",       "Send no message with away presence." },
            { "message xa off",         "Send no message with xa presence." },
            { "check on|off",           "When enabled, checks for activity and sends online presence, default: on." })
        CMD_EXAMPLES(
            "/autoaway mode away",
            "/autoaway time away 30",
            "/autoaway message away Away from computer for a while",
            "/autoaway time xa 120",
            "/autoaway message xa Away from computer for a very long time",
            "/autoaway check off")
    },

    { "/priority",
        parse_args, 1, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_priority)
        CMD_TAGS(
            CMD_TAG_PRESENCE)
        CMD_SYN(
            "/priority <priority>")
        CMD_DESC(
            "Set priority for the current account. "
            "See the /account command for specific priority settings per presence status.")
        CMD_ARGS(
            { "<priority>", "Number between -128 and 127, default: 0." })
        CMD_NOEXAMPLES
    },

    { "/account",
        parse_args, 0, 4, NULL,
        CMD_SUBFUNCS(
            { "list",       cmd_account_list },
            { "show",       cmd_account_show },
            { "add",        cmd_account_add },
            { "remove",     cmd_account_remove },
            { "enable",     cmd_account_enable },
            { "disable",    cmd_account_disable },
            { "rename",     cmd_account_rename },
            { "default",    cmd_account_default },
            { "set",        cmd_account_set },
            { "clear",      cmd_account_clear })
        CMD_MAINFUNC(cmd_account)
        CMD_TAGS(
            CMD_TAG_CONNECTION,
            CMD_TAG_PRESENCE,
            CMD_TAG_CHAT,
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/account",
            "/account list",
            "/account show <account>",
            "/account enable|disable <account>",
            "/account default set <account>",
            "/account default off",
            "/account add <account>",
            "/account remove <account>",
            "/account rename <account> <newaccount>",
            "/account set <account> jid <jid>",
            "/account set <account> server <server>",
            "/account set <account> port <port>",
            "/account set <account> status <presence>",
            "/account set <account> status last",
            "/account set <account> <presence> <priority>",
            "/account set <account> resource <resource>",
            "/account set <account> password <password>",
            "/account set <account> eval_password <command>",
            "/account set <account> muc <service>",
            "/account set <account> nick <nick>",
            "/account set <account> otr <policy>",
            "/account set <account> pgpkeyid <pgpkeyid>",
            "/account set <account> startscript <script>",
            "/account set <account> tls force|allow|legacy|disable",
            "/account set <account> theme <theme>",
            "/account clear <account> password",
            "/account clear <account> eval_password",
            "/account clear <account> server",
            "/account clear <account> port",
            "/account clear <account> otr",
            "/account clear <account> pgpkeyid",
            "/account clear <account> startscript",
            "/account clear <account> muc",
            "/account clear <account> resource")
        CMD_DESC(
            "Commands for creating and managing accounts. "
            "Calling with no arguments will display information for the current account.")
        CMD_ARGS(
            { "list",                                   "List all accounts." },
            { "enable <account>",                       "Enable the account, it will be used for autocompletion." },
            { "show <account>",                         "Show details for the specified account." },
            { "disable <account>",                      "Disable the account." },
            { "default set <account>",                  "Set the default account, used when no argument passed to the /connect command." },
            { "default off",                            "Clear the default account setting." },
            { "add <account>",                          "Create a new account." },
            { "remove <account>",                       "Remove an account." },
            { "rename <account> <newaccount>",          "Rename 'account' to 'newaccount'." },
            { "set <account> jid <jid>",                "Set the Jabber ID for the account, account name will be used if not set." },
            { "set <account> server <server>",          "The chat server, if different to the domainpart of the JID." },
            { "set <account> port <port>",              "The port used for connecting if not the default (5222, or 5223 for SSL)." },
            { "set <account> status <presence>",        "The presence status to use on login." },
            { "set <account> status last",              "Use your last status before logging out, when logging in." },
            { "set <account> <presence> <priority>",    "Set the priority (-128..127) to use for the specified presence." },
            { "set <account> resource <resource>",      "The resource to be used for this account, defaults to 'profanity'." },
            { "set <account> password <password>",      "Password for the account, note this is currently stored in plaintext if set." },
            { "set <account> eval_password <command>",  "Shell command evaluated to retrieve password for the account. Can be used to retrieve password from keyring." },
            { "set <account> muc <service>",            "The default MUC chat service to use, defaults to the servers disco info response." },
            { "set <account> nick <nick>",              "The default nickname to use when joining chat rooms." },
            { "set <account> otr <policy>",             "Override global OTR policy for this account, see /otr." },
            { "set <account> pgpkeyid <pgpkeyid>",      "Set the ID of the PGP key for this account, see /pgp." },
            { "set <account> startscript <script>",     "Set the script to execute after connecting." },
            { "set <account> tls force",                "Force TLS connection, and fail if one cannot be established, this is default behaviour." },
            { "set <account> tls allow",                "Use TLS for the connection if it is available." },
            { "set <account> tls legacy",               "Use legacy TLS for the connection. It means server doesn't support STARTTLS and TLS is forced just after TCP connection is established." },
            { "set <account> tls disable",              "Disable TLS for the connection." },
            { "set <account> <theme>",                  "Set the UI theme for the account." },
            { "clear <account> server",                 "Remove the server setting for this account." },
            { "clear <account> port",                   "Remove the port setting for this account." },
            { "clear <account> password",               "Remove the password setting for this account." },
            { "clear <account> eval_password",          "Remove the eval_password setting for this account." },
            { "clear <account> otr",                    "Remove the OTR policy setting for this account." },
            { "clear <account> pgpkeyid",               "Remove pgpkeyid associated with this account." },
            { "clear <account> startscript",            "Remove startscript associated with this account." },
            { "clear <account> theme",                  "Clear the theme setting for the account, the global theme will be used." },
            { "clear <account> resource",               "Remove the resource setting for this account."},
            { "clear <account> muc",                    "Remove the default MUC service setting."})
        CMD_EXAMPLES(
            "/account add me",
            "/account set me jid me@chatty",
            "/account set me server talk.chat.com",
            "/account set me port 5111",
            "/account set me muc chatservice.mycompany.com",
            "/account set me nick dennis",
            "/account set me status dnd",
            "/account set me dnd -1",
            "/account rename me chattyme")
    },

    { "/plugins",
        parse_args, 0, 3, NULL,
        CMD_SUBFUNCS(
            { "sourcepath",     cmd_plugins_sourcepath },
            { "install",        cmd_plugins_install },
            { "load",           cmd_plugins_load },
            { "unload",         cmd_plugins_unload },
            { "reload",         cmd_plugins_reload },
            { "python_version", cmd_plugins_python_version })
        CMD_MAINFUNC(cmd_plugins)
        CMD_NOTAGS
        CMD_SYN(
            "/plugins",
            "/plugins sourcepath set <path>",
            "/plugins sourcepath clear",
            "/plugins install [<path>]",
            "/plugins unload [<plugin>]",
            "/plugins load [<plugin>]",
            "/plugins reload [<plugin>]",
            "/plugins python_version")
        CMD_DESC(
            "Manage plugins. Passing no arguments lists currently loaded plugins.")
        CMD_ARGS(
            { "sourcepath set <path>",  "Set the default path to install plugins from, will be used if no arg is passed to /plugins install." },
            { "sourcepath clear",       "Clear the default plugins source path." },
            { "install [<path>]",       "Install a plugin, or all plugins found in a directory (recursive). Passing no argument will use the sourcepath if one is set." },
            { "load [<plugin>]",        "Load a plugin that already exists in the plugin directory, passing no argument loads all found plugins." },
            { "unload [<plugin>]",      "Unload a loaded plugin, passing no argument will unload all plugins." },
            { "reload [<plugin>]",      "Reload a plugin, passing no argument will reload all plugins." },
            { "python_version",         "Show the Python interpreter version." })
        CMD_EXAMPLES(
            "/plugins sourcepath set /home/meee/projects/profanity-plugins",
            "/plugins install",
            "/plugins install /home/steveharris/Downloads/metal.py",
            "/plugins load browser.py",
            "/plugins unload say.py",
            "/plugins reload wikipedia.py")
    },

    { "/prefs",
        parse_args, 0, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_prefs)
        CMD_NOTAGS
        CMD_SYN(
            "/prefs [ui|desktop|chat|log|conn|presence|otr|pgp]")
        CMD_DESC(
            "Show preferences for different areas of functionality. "
            "Passing no arguments shows all preferences.")
        CMD_ARGS(
            { "ui",       "User interface preferences." },
            { "desktop",  "Desktop notification preferences." },
            { "chat",     "Chat state preferences." },
            { "log",      "Logging preferences." },
            { "conn",     "Connection handling preferences." },
            { "presence", "Chat presence preferences." },
            { "otr",      "Off The Record preferences." },
            { "pgp",      "OpenPGP preferences." })
        CMD_NOEXAMPLES
    },

    { "/theme",
        parse_args, 1, 2, &cons_theme_setting,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_theme)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/theme list",
            "/theme load <theme>",
            "/theme colours",
            "/theme properties")
        CMD_DESC(
            "Load a theme, includes colours and UI options.")
        CMD_ARGS(
            { "list",           "List all available themes." },
            { "load <theme>",   "Load the specified theme. 'default' will reset to the default theme." },
            { "colours",        "Show colour values as rendered by the terminal." },
            { "properties",     "Show colour settings for current theme." })
        CMD_EXAMPLES(
            "/theme list",
            "/theme load forest")
    },

    { "/xmlconsole",
        parse_args, 0, 0, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_xmlconsole)
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/xmlconsole")
        CMD_DESC(
            "Open the XML console to view incoming and outgoing XMPP traffic.")
        CMD_NOARGS
        CMD_NOEXAMPLES
    },

    { "/away",
        parse_args_with_freetext, 0, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_away)
        CMD_TAGS(
            CMD_TAG_PRESENCE)
        CMD_SYN(
            "/away [<message>]")
        CMD_DESC(
            "Set your status to 'away'.")
        CMD_ARGS(
            { "<message>",  "Optional message to use with the status." })
        CMD_EXAMPLES(
            "/away",
            "/away Gone for lunch")
    },

    { "/chat",
        parse_args_with_freetext, 0, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_chat)
        CMD_TAGS(
            CMD_TAG_PRESENCE)
        CMD_SYN(
            "/chat [<message>]")
        CMD_DESC(
            "Set your status to 'chat' (available for chat).")
        CMD_ARGS(
            { "<message>",  "Optional message to use with the status." })
        CMD_EXAMPLES(
            "/chat",
            "/chat Please talk to me!")
    },

    { "/dnd",
        parse_args_with_freetext, 0, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_dnd)
        CMD_TAGS(
            CMD_TAG_PRESENCE)
        CMD_SYN(
            "/dnd [<message>]")
        CMD_DESC(
            "Set your status to 'dnd' (do not disturb).")
        CMD_ARGS(
            { "<message>",  "Optional message to use with the status." })
        CMD_EXAMPLES(
            "/dnd",
            "/dnd I'm in the zone")
    },

    { "/online",
        parse_args_with_freetext, 0, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_online)
        CMD_TAGS(
            CMD_TAG_PRESENCE)
        CMD_SYN(
            "/online [<message>]")
        CMD_DESC(
            "Set your status to 'online'.")
        CMD_ARGS(
            { "<message>",  "Optional message to use with the status." })
        CMD_EXAMPLES(
            "/online",
            "/online Up the Irons!")
    },

    { "/xa",
        parse_args_with_freetext, 0, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_xa)
        CMD_TAGS(
            CMD_TAG_PRESENCE)
        CMD_SYN(
            "/xa [<message>]")
        CMD_DESC(
            "Set your status to 'xa' (extended away).")
        CMD_ARGS(
            { "<message>",  "Optional message to use with the status." })
        CMD_EXAMPLES(
            "/xa",
            "/xa This meeting is going to be a long one")
    },

    { "/script",
        parse_args, 1, 2, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_script)
        CMD_NOTAGS
        CMD_SYN(
            "/script run <script>",
            "/script list",
            "/script show <script>")
        CMD_DESC(
            "Run command scripts. "
            "Scripts are stored in $XDG_DATA_HOME/profanity/scripts/ which is usually $HOME/.local/share/profanity/scripts/.")
        CMD_ARGS(
            { "script run <script>",    "Execute a script." },
            { "script list",            "List all scripts TODO." },
            { "script show <script>",   "Show the commands in script TODO." })
        CMD_EXAMPLES(
            "/script list",
            "/script run myscript",
            "/script show somescript")
    },

    { "/export",
        parse_args, 1, 1, NULL,
        CMD_NOSUBFUNCS
        CMD_MAINFUNC(cmd_export)
        CMD_NOTAGS
        CMD_SYN(
            "/export <filepath>")
        CMD_DESC(
            "Exports contacts to a csv file.")
        CMD_ARGS(
            { "<filepath>", "Path to the output file." })
        CMD_EXAMPLES(
            "/export /path/to/output.csv",
            "/export ~/contacts.csv")
    }
};

static GHashTable *search_index;

char*
_cmd_index(Command *cmd) {
    GString *index_source = g_string_new("");
    index_source = g_string_append(index_source, cmd->cmd);
    index_source = g_string_append(index_source, " ");
    index_source = g_string_append(index_source, cmd->help.desc);
    index_source = g_string_append(index_source, " ");

    int len = g_strv_length(cmd->help.tags);
    int i = 0;
    for (i = 0; i < len; i++) {
        index_source = g_string_append(index_source, cmd->help.tags[i]);
        index_source = g_string_append(index_source, " ");
    }
    len = g_strv_length(cmd->help.synopsis);
    for (i = 0; i < len; i++) {
        index_source = g_string_append(index_source, cmd->help.synopsis[i]);
        index_source = g_string_append(index_source, " ");
    }
    for (i = 0; cmd->help.args[i][0] != NULL; i++) {
        index_source = g_string_append(index_source, cmd->help.args[i][0]);
        index_source = g_string_append(index_source, " ");
        index_source = g_string_append(index_source, cmd->help.args[i][1]);
        index_source = g_string_append(index_source, " ");
    }

    gchar **tokens = g_str_tokenize_and_fold(index_source->str, NULL, NULL);
    g_string_free(index_source, TRUE);

    GString *index = g_string_new("");
    i = 0;
    for (i = 0; i < g_strv_length(tokens); i++) {
        index = g_string_append(index, tokens[i]);
        index = g_string_append(index, " ");
    }
    g_strfreev(tokens);

    char *res = index->str;
    g_string_free(index, FALSE);

    return res;
}

GList*
cmd_search_index_any(char *term)
{
    GList *results = NULL;

    gchar **processed_terms = g_str_tokenize_and_fold(term, NULL, NULL);
    int terms_len = g_strv_length(processed_terms);

    int i = 0;
    for (i = 0; i < terms_len; i++) {
        GList *index_keys = g_hash_table_get_keys(search_index);
        GList *curr = index_keys;
        while (curr) {
            char *index_entry = g_hash_table_lookup(search_index, curr->data);
            if (g_str_match_string(processed_terms[i], index_entry, FALSE)) {
                results = g_list_append(results, curr->data);
            }
            curr = g_list_next(curr);
        }
        g_list_free(index_keys);
    }

    g_strfreev(processed_terms);

    return results;
}

GList*
cmd_search_index_all(char *term)
{
    GList *results = NULL;

    gchar **terms = g_str_tokenize_and_fold(term, NULL, NULL);
    int terms_len = g_strv_length(terms);

    GList *commands = g_hash_table_get_keys(search_index);
    GList *curr = commands;
    while (curr) {
        char *command = curr->data;
        int matches = 0;
        int i = 0;
        for (i = 0; i < terms_len; i++) {
            char *command_index = g_hash_table_lookup(search_index, command);
            if (g_str_match_string(terms[i], command_index, FALSE)) {
                matches++;
            }
        }
        if (matches == terms_len) {
            results = g_list_append(results, command);
        }
        curr = g_list_next(curr);
    }

    g_list_free(commands);
    g_strfreev(terms);

    return results;
}

/*
 * Initialise command autocompleter and history
 */
void
cmd_init(void)
{
    log_info("Initialising commands");

    cmd_ac_init();

    search_index = g_hash_table_new_full(g_str_hash, g_str_equal, free, g_free);

    // load command defs into hash table
    commands = g_hash_table_new(g_str_hash, g_str_equal);
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(command_defs); i++) {
        Command *pcmd = command_defs+i;

        // add to hash
        g_hash_table_insert(commands, pcmd->cmd, pcmd);

        // add to search index
        g_hash_table_insert(search_index, strdup(pcmd->cmd), _cmd_index(pcmd));

        // add to commands and help autocompleters
        cmd_ac_add_cmd(pcmd);
    }

    // load aliases
    GList *aliases = prefs_get_aliases();
    GList *curr = aliases;
    while (curr) {
        ProfAlias *alias = curr->data;
        cmd_ac_add_alias(alias);
        curr = g_list_next(curr);
    }
    prefs_free_aliases(aliases);
}

void
cmd_uninit(void)
{
    cmd_ac_uninit();
    g_hash_table_destroy(search_index);
}

gboolean
cmd_valid_tag(const char *const str)
{
    return ((g_strcmp0(str, CMD_TAG_CHAT) == 0) ||
        (g_strcmp0(str, CMD_TAG_GROUPCHAT) == 0) ||
        (g_strcmp0(str, CMD_TAG_PRESENCE) == 0) ||
        (g_strcmp0(str, CMD_TAG_ROSTER) == 0) ||
        (g_strcmp0(str, CMD_TAG_DISCOVERY) == 0) ||
        (g_strcmp0(str, CMD_TAG_CONNECTION) == 0) ||
        (g_strcmp0(str, CMD_TAG_UI) == 0) ||
        (g_strcmp0(str, CMD_TAG_PLUGINS) == 0));
}

Command*
cmd_get(const char *const command)
{
    if (commands) {
        return g_hash_table_lookup(commands, command);
    } else {
        return NULL;
    }
}

GList*
cmd_get_ordered(const char *const tag)
{
    GList *ordered_commands = NULL;

    GHashTableIter iter;
    gpointer key;
    gpointer value;

    g_hash_table_iter_init(&iter, commands);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        Command *pcmd = (Command *)value;
        if (tag) {
            if (_cmd_has_tag(pcmd, tag)) {
                ordered_commands = g_list_insert_sorted(ordered_commands, pcmd->cmd, (GCompareFunc)g_strcmp0);
            }
        } else {
            ordered_commands = g_list_insert_sorted(ordered_commands, pcmd->cmd, (GCompareFunc)g_strcmp0);
        }
    }

    return ordered_commands;
}

static gboolean
_cmd_has_tag(Command *pcmd, const char *const tag)
{
    int i = 0;
    for (i = 0; pcmd->help.tags[i] != NULL; i++) {
        if (g_strcmp0(tag, pcmd->help.tags[i]) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

static int
_cmp_command(Command *cmd1, Command *cmd2)
{
    return g_strcmp0(cmd1->cmd, cmd2->cmd);
}

void
command_docgen(void)
{
    GList *cmds = NULL;
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(command_defs); i++) {
        Command *pcmd = command_defs+i;
        cmds = g_list_insert_sorted(cmds, pcmd, (GCompareFunc)_cmp_command);
    }

    FILE *toc_fragment = fopen("toc_fragment.html", "w");
    FILE *main_fragment = fopen("main_fragment.html", "w");

    fputs("<ul><li><ul><li>\n", toc_fragment);
    fputs("<hr>\n", main_fragment);

    GList *curr = cmds;
    while (curr) {
        Command *pcmd = curr->data;

        fprintf(toc_fragment, "<a href=\"#%s\">%s</a>,\n", &pcmd->cmd[1], pcmd->cmd);
        fprintf(main_fragment, "<a name=\"%s\"></a>\n", &pcmd->cmd[1]);
        fprintf(main_fragment, "<h4>%s</h4>\n", pcmd->cmd);

        fputs("<p><b>Synopsis</b></p>\n", main_fragment);
        fputs("<p><pre><code>", main_fragment);
        int i = 0;
        while (pcmd->help.synopsis[i]) {
            char *str1 = str_replace(pcmd->help.synopsis[i], "<", "&lt;");
            char *str2 = str_replace(str1, ">", "&gt;");
            fprintf(main_fragment, "%s\n", str2);
            i++;
        }
        fputs("</code></pre></p>\n", main_fragment);

        fputs("<p><b>Description</b></p>\n", main_fragment);
        fputs("<p>", main_fragment);
        fprintf(main_fragment, "%s\n", pcmd->help.desc);
        fputs("</p>\n", main_fragment);

        if (pcmd->help.args[0][0] != NULL) {
            fputs("<p><b>Arguments</b></p>\n", main_fragment);
            fputs("<table>", main_fragment);
            for (i = 0; pcmd->help.args[i][0] != NULL; i++) {
                fputs("<tr>", main_fragment);
                fputs("<td>", main_fragment);
                fputs("<code>", main_fragment);
                char *str1 = str_replace(pcmd->help.args[i][0], "<", "&lt;");
                char *str2 = str_replace(str1, ">", "&gt;");
                fprintf(main_fragment, "%s", str2);
                fputs("</code>", main_fragment);
                fputs("</td>", main_fragment);
                fputs("<td>", main_fragment);
                fprintf(main_fragment, "%s", pcmd->help.args[i][1]);
                fputs("</td>", main_fragment);
                fputs("</tr>", main_fragment);
            }
            fputs("</table>\n", main_fragment);
        }

        if (pcmd->help.examples[0] != NULL) {
            fputs("<p><b>Examples</b></p>\n", main_fragment);
            fputs("<p><pre><code>", main_fragment);
            int i = 0;
            while (pcmd->help.examples[i]) {
                fprintf(main_fragment, "%s\n", pcmd->help.examples[i]);
                i++;
            }
            fputs("</code></pre></p>\n", main_fragment);
        }

        fputs("<a href=\"#top\"><h5>back to top</h5></a><br><hr>\n", main_fragment);
        fputs("\n", main_fragment);

        curr = g_list_next(curr);
    }

    fputs("</ul></ul>\n", toc_fragment);

    fclose(toc_fragment);
    fclose(main_fragment);
    printf("\nProcessed %d commands.\n\n", g_list_length(cmds));
    g_list_free(cmds);
}
