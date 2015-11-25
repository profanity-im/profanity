/*
 * command.c
 *
 * Copyright (C) 2012 - 2015 James Booth <boothj5@gmail.com>
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

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "chat_session.h"
#include "command/command.h"
#include "command/commands.h"
#include "common.h"
#include "config/accounts.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "config/tlscerts.h"
#include "config/scripts.h"
#include "contact.h"
#include "roster_list.h"
#include "jid.h"
#include "xmpp/form.h"
#include "log.h"
#include "muc.h"
#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif
#ifdef HAVE_LIBGPGME
#include "pgp/gpg.h"
#endif
#include "profanity.h"
#include "tools/autocomplete.h"
#include "tools/parser.h"
#include "tools/tinyurl.h"
#include "xmpp/xmpp.h"
#include "xmpp/bookmark.h"
#include "ui/ui.h"
#include "window_list.h"

static gboolean _cmd_execute(ProfWin *window, const char *const command, const char *const inp);

static char* _cmd_complete_parameters(ProfWin *window, const char *const input);

static char* _script_autocomplete_func(const char *const prefix);

static char* _sub_autocomplete(ProfWin *window, const char *const input);
static char* _notify_autocomplete(ProfWin *window, const char *const input);
static char* _theme_autocomplete(ProfWin *window, const char *const input);
static char* _autoaway_autocomplete(ProfWin *window, const char *const input);
static char* _autoconnect_autocomplete(ProfWin *window, const char *const input);
static char* _account_autocomplete(ProfWin *window, const char *const input);
static char* _who_autocomplete(ProfWin *window, const char *const input);
static char* _roster_autocomplete(ProfWin *window, const char *const input);
static char* _group_autocomplete(ProfWin *window, const char *const input);
static char* _bookmark_autocomplete(ProfWin *window, const char *const input);
static char* _otr_autocomplete(ProfWin *window, const char *const input);
static char* _pgp_autocomplete(ProfWin *window, const char *const input);
static char* _connect_autocomplete(ProfWin *window, const char *const input);
static char* _statuses_autocomplete(ProfWin *window, const char *const input);
static char* _alias_autocomplete(ProfWin *window, const char *const input);
static char* _join_autocomplete(ProfWin *window, const char *const input);
static char* _log_autocomplete(ProfWin *window, const char *const input);
static char* _form_autocomplete(ProfWin *window, const char *const input);
static char* _form_field_autocomplete(ProfWin *window, const char *const input);
static char* _occupants_autocomplete(ProfWin *window, const char *const input);
static char* _kick_autocomplete(ProfWin *window, const char *const input);
static char* _ban_autocomplete(ProfWin *window, const char *const input);
static char* _affiliation_autocomplete(ProfWin *window, const char *const input);
static char* _role_autocomplete(ProfWin *window, const char *const input);
static char* _resource_autocomplete(ProfWin *window, const char *const input);
static char* _titlebar_autocomplete(ProfWin *window, const char *const input);
static char* _inpblock_autocomplete(ProfWin *window, const char *const input);
static char* _time_autocomplete(ProfWin *window, const char *const input);
static char* _receipts_autocomplete(ProfWin *window, const char *const input);
static char* _help_autocomplete(ProfWin *window, const char *const input);
static char* _wins_autocomplete(ProfWin *window, const char *const input);
static char* _tls_autocomplete(ProfWin *window, const char *const input);
static char* _script_autocomplete(ProfWin *window, const char *const input);
static char* _subject_autocomplete(ProfWin *window, const char *const input);

GHashTable *commands = NULL;

#define CMD_TAG_CHAT        "chat"
#define CMD_TAG_GROUPCHAT   "groupchat"
#define CMD_TAG_ROSTER      "roster"
#define CMD_TAG_PRESENCE    "presence"
#define CMD_TAG_CONNECTION  "connection"
#define CMD_TAG_DISCOVERY   "discovery"
#define CMD_TAG_UI          "ui"

#define CMD_NOTAGS          { { NULL },
#define CMD_TAGS(...)       { { __VA_ARGS__, NULL },
#define CMD_SYN(...)        { __VA_ARGS__, NULL },
#define CMD_DESC(desc)      desc,
#define CMD_NOARGS          { { NULL, NULL } },
#define CMD_ARGS(...)       { __VA_ARGS__, { NULL, NULL } },
#define CMD_NOEXAMPLES      { NULL } }
#define CMD_EXAMPLES(...)   { __VA_ARGS__, NULL } }

/*
 * Command list
 */
static struct cmd_t command_defs[] =
{
    { "/help",
        cmd_help, parse_args, 0, 2, NULL,
        CMD_NOTAGS
        CMD_SYN(
            "/help [<area>|<command>]")
        CMD_DESC(
            "Help on using Profanity. Passing no arguments list help areas. "
            "For command help, optional arguments are shown using square brackets e.g. [argument], "
            "arguments representing variables rather than a literal name are surrounded by angle brackets "
            "e.g. <argument>. "
            "Arguments that may be one of a number of values are separated by a pipe "
            "e.g. val1|val2|val3.")
        CMD_ARGS(
            { "<area>",    "Summary help for commands in a certain area of functionality." },
            { "<command>", "Full help for a specific command, for example '/help connect'." })
        CMD_EXAMPLES(
            "/help commands",
            "/help presence",
            "/help who")
    },

    { "/about",
        cmd_about, parse_args, 0, 0, NULL,
        CMD_NOTAGS
        CMD_SYN(
            "/about")
        CMD_DESC(
            "Show version and license information.")
        CMD_NOARGS
        CMD_NOEXAMPLES
    },

    { "/connect",
        cmd_connect, parse_args, 0, 7, NULL,
        CMD_TAGS(
            CMD_TAG_CONNECTION)
        CMD_SYN(
            "/connect [<account>]",
            "/connect <account> [server <server>] [port <port>] [tls force|allow|disable]")
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
        cmd_tls, parse_args, 1, 3, NULL,
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
            "/tls show on|off")
        CMD_DESC(
            "Handle TLS certificates. ")
        CMD_ARGS(
            { "allow",                "Allow connection to continue with TLS certificate." },
            { "always",               "Always allow connections with TLS certificate." },
            { "deny",                 "Abort connection." },
            { "cert",                 "Show the current TLS certificate." },
            { "cert <fingerprint>",   "Show details of trusted certificate." },
            { "trust",                "Add the current TLS certificate to manually trusted certiciates." },
            { "trusted",              "List summary of manually trusted certificates (with '/tls always' or '/tls trust')." },
            { "revoke <fingerprint>", "Remove a manually trusted certificate." },
            { "certpath",             "Show the trusted certificate path." },
            { "certpath set <path>",  "Specify filesystem path containing trusted certificates." },
            { "certpath clear",       "Clear the trusted certificate path." },
            { "show on|off",          "Show or hide the TLS indicator in the titlebar." })
        CMD_NOEXAMPLES
    },

    { "/disconnect",
        cmd_disconnect, parse_args, 0, 0, NULL,
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
        cmd_msg, parse_args_with_freetext, 1, 2, NULL,
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
        cmd_roster, parse_args_with_freetext, 0, 3, NULL,
        CMD_TAGS(
            CMD_TAG_ROSTER,
            CMD_TAG_UI)
        CMD_SYN(
            "/roster",
            "/roster online",
            "/roster show [offline|resource|presence|status|empty|count|priority]",
            "/roster hide [offline|resource|presence|status|empty|count|priority]",
            "/roster by group|presence|none",
            "/roster order name|presence",
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
            { "online",                     "Show all online contacts in your roster." },
            { "show",                       "Show the roster panel." },
            { "show offline",               "Show offline contacts in the roster panel." },
            { "show resource",              "Show contact's connected resources in the roster panel." },
            { "show presence",              "Show contact's presence in the roster panel." },
            { "show status",                "Show contact's status message in the roster panel." },
            { "show empty",                 "When grouping by presence, show empty presence groups." },
            { "show count",                 "Show number of contacts in group/presence." },
            { "show priority",              "Show resource priority." },
            { "hide",                       "Hide the roster panel." },
            { "hide offline",               "Hide offline contacts in the roster panel." },
            { "hide resource",              "Hide contact's connected resources in the roster panel." },
            { "hide presence",              "Hide contact's presence in the roster panel." },
            { "hide status",                "Hide contact's status message in the roster panel." },
            { "hide empty",                 "When grouping by presence, hide empty presence groups." },
            { "hide count",                 "Hide number of contacts in group/presence." },
            { "hide priority",              "Hide resource priority." },
            { "by group",                   "Group contacts in the roster panel by roster group." },
            { "by presence",                "Group contacts in the roster panel by presence." },
            { "by none",                    "No grouping in the roster panel." },
            { "order name",                 "Order roster items by name only." },
            { "order presence",             "Order roster items by presence, and then by name." },
            { "header char <char>",         "Prefix roster headers with specificed character." },
            { "header char none",           "Remove roster header character prefix." },
            { "contact char <char>",        "Prefix roster contacts with specificed character." },
            { "contact char none",          "Remove roster contact character prefix." },
            { "contact indent <indent>",    "Indent contact line by <indent> spaces (0 to 10)." },
            { "resource char <char>",       "Prefix roster resources with specificed character." },
            { "resource char none",         "Remove roster resource character prefix." },
            { "resource indent <indent>",   "Indent resource line by <indent> spaces (0 to 10)." },
            { "resource join on|off",       "Join resource with previous line when only one available resource." },
            { "presence indent <indent>",   "Indent presence line by <indent> spaces (-1 to 10), a value of -1 will show presence on the previous line." },
            { "size <precent>",             "Percentage of the screen taken up by the roster (1-99)." },
            { "wrap on|off",                "Enabled or disanle line wrapping in roster panel." },
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

    { "/group",
        cmd_group, parse_args_with_freetext, 0, 3, NULL,
        CMD_TAGS(
            CMD_TAG_ROSTER,
            CMD_TAG_UI)
        CMD_SYN(
            "/group",
            "/group show <group>",
            "/group add <group> <contat>"
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
        cmd_info, parse_args, 0, 1, NULL,
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
        cmd_caps, parse_args, 0, 1, NULL,
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
        cmd_software, parse_args, 0, 1, NULL,
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
        cmd_status, parse_args, 0, 1, NULL,
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
        cmd_resource, parse_args, 1, 2, &cons_resource_setting,
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
        cmd_join, parse_args, 0, 5, NULL,
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
        cmd_leave, parse_args, 0, 0, NULL,
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/leave")
        CMD_DESC(
            "Leave the current chat room.")
        CMD_NOARGS
        CMD_NOEXAMPLES
    },

    { "/invite",
        cmd_invite, parse_args_with_freetext, 1, 2, NULL,
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
        cmd_invites, parse_args_with_freetext, 0, 0, NULL,
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
        cmd_decline, parse_args_with_freetext, 1, 1, NULL,
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
        cmd_room, parse_args, 1, 1, NULL,
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
        cmd_kick, parse_args_with_freetext, 1, 2, NULL,
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
        cmd_ban, parse_args_with_freetext, 1, 2, NULL,
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
        cmd_subject, parse_args_with_freetext, 0, 2, NULL,
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
            { "append <text>",  "Append text to the current room subject, use double quotes if a preceeding space is needed." },
            { "clear",          "Clear the room subject." })
        CMD_NOEXAMPLES
    },

    { "/affiliation",
        cmd_affiliation, parse_args_with_freetext, 1, 4, NULL,
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/affiliation set <affiliation> <jid> [<reason>]",
            "/list [<affiliation>]")
        CMD_DESC(
            "Manage room affiliations. "
            "Affiliation may be one of owner, admin, member, outcast or none.")
        CMD_ARGS(
            { "set <affiliation> <jid> [<reason>]", "Set the affiliation of user with jid, with an optional reason." },
            { "list [<affiliation>]",               "List all users with the specified affiliation, or all if none specified." })
        CMD_NOEXAMPLES
    },

    { "/role",
        cmd_role, parse_args_with_freetext, 1, 4, NULL,
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/role set <role> <nick> [<reason>]",
            "/list [<role>]")
        CMD_DESC(
            "Manage room roles. "
            "Role may be one of moderator, participant, visitor or none.")
        CMD_ARGS(
            { "set <role> <nick> [<reason>]", "Set the role of occupant with nick, with an optional reason." },
            { "list [<role>]",                "List all occupants with the specified role, or all if none specified." })
        CMD_NOEXAMPLES
    },

    { "/occupants",
        cmd_occupants, parse_args, 1, 3, cons_occupants_setting,
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
        cmd_form, parse_args, 1, 2, NULL,
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
        cmd_rooms, parse_args, 0, 1, NULL,
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
        cmd_bookmark, parse_args, 0, 8, NULL,
        CMD_TAGS(
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/bookmark",
            "/bookmark list",
            "/bookmark add <room> [nick <nick>] [password <password>] [autojoin on|off]",
            "/bookmark update <room> [nick <nick>] [password <password>] [autojoin on|off]",
            "/bookmark remove <room>",
            "/bookmark join <room>")
        CMD_DESC(
            "Manage bookmarks and join bookmarked rooms. "
            "In a chat room, no arguments will bookmark the current room, setting autojoin to \"on\".")
        CMD_ARGS(
            { "list", "List all bookmarks." },
            { "add <room>", "Add a bookmark." },
            { "remove <room>", "Remove a bookmark." },
            { "update <room>", "Update the properties associated with a bookmark." },
            { "nick <nick>", "Nickname used in the chat room." },
            { "password <password>", "Password if required, may be stored in plaintext on your server." },
            { "autojoin on|off", "Whether to join the room automatically on login." },
            { "join <room>", "Join room using the properties associated with the bookmark." })
        CMD_NOEXAMPLES
    },

    { "/disco",
        cmd_disco, parse_args, 1, 2, NULL,
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

    { "/lastactivity",
        cmd_lastactivity, parse_args, 0, 1, NULL,
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
        cmd_nick, parse_args_with_freetext, 1, 1, NULL,
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
        cmd_win, parse_args, 1, 1, NULL,
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/win <num>")
        CMD_DESC(
            "Move to the specified window.")
        CMD_ARGS(
            { "<num>", "Window number to display." })
        CMD_NOEXAMPLES
    },

    { "/wins",
        cmd_wins, parse_args, 0, 3, NULL,
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/wins tidy",
            "/wins autotidy on|off",
            "/wins prune",
            "/wins swap <source> <target>")
        CMD_DESC(
            "Manage windows. "
            "Passing no argument will list all currently active windows and information about their usage.")
        CMD_ARGS(
            { "tidy",                   "Move windows so there are no gaps." },
            { "autotidy on|off",        "Automatically remove gaps when closing windows." },
            { "prune",                  "Close all windows with no unread messages, and then tidy so there are no gaps." },
            { "swap <source> <target>", "Swap windows, target may be an empty position." })
        CMD_NOEXAMPLES
    },

    { "/sub",
        cmd_sub, parse_args, 1, 2, NULL,
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
        cmd_tiny, parse_args, 1, 1, NULL,
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
        cmd_who, parse_args, 0, 2, NULL,
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
        cmd_close, parse_args, 0, 1, NULL,
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/close [<num>]",
            "/close all|read")
        CMD_DESC(
            "Close windows. "
            "Passing no argument closes the current window.")
        CMD_ARGS(
            { "<num>", "Close the specified window." },
            { "all", "Close all windows." },
            { "read", "Close all windows that have no unread messages." })
        CMD_NOEXAMPLES
    },

    { "/clear",
        cmd_clear, parse_args, 0, 0, NULL,
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
        cmd_quit, parse_args, 0, 0, NULL,
        CMD_NOTAGS
        CMD_SYN(
            "/quit")
        CMD_DESC(
            "Logout of any current session, and quit Profanity.")
        CMD_NOARGS
        CMD_NOEXAMPLES
    },

    { "/privileges",
        cmd_privileges, parse_args, 1, 1, &cons_privileges_setting,
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

    { "/beep",
        cmd_beep, parse_args, 1, 1, &cons_beep_setting,
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

    { "/encwarn",
        cmd_encwarn, parse_args, 1, 1, &cons_encwarn_setting,
        CMD_TAGS(
            CMD_TAG_CHAT,
            CMD_TAG_UI)
        CMD_SYN(
            "/encwarn on|off")
        CMD_DESC(
            "Titlebar encryption warning.")
        CMD_ARGS(
            { "on|off", "Enabled or disable the unencrypted warning message in the titlebar." })
        CMD_NOEXAMPLES
    },

    { "/presence",
        cmd_presence, parse_args, 1, 1, &cons_presence_setting,
        CMD_TAGS(
            CMD_TAG_UI,
            CMD_TAG_CHAT)
        CMD_SYN(
            "/presence on|off")
        CMD_DESC(
            "Show the contacts presence in the titlebar.")
        CMD_ARGS(
            { "on|off", "Switch display of the contacts presence in the titlebar on or off." })
        CMD_NOEXAMPLES
    },

    { "/wrap",
        cmd_wrap, parse_args, 1, 1, &cons_wrap_setting,
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
        cmd_time, parse_args, 1, 3, &cons_time_setting,
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
        cmd_inpblock, parse_args, 2, 2, &cons_inpblock_setting,
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

    { "/notify",
        cmd_notify, parse_args_with_freetext, 2, 4, &cons_notify_setting,
        CMD_TAGS(
            CMD_TAG_UI,
            CMD_TAG_CHAT,
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/notify message on|off",
            "/notify message current on|off",
            "/notify message text on|off",
            "/notify message trigger add <text>",
            "/notify message trigger remove <text>",
            "/notify message trigger list",
            "/notify message trigger on|off",
            "/notify room on|off",
            "/notify room mention on|off",
            "/notify room current on|off",
            "/notify room text on|off",
            "/notify room trigger add <text>",
            "/notify room trigger remove <text>",
            "/notify room trigger list",
            "/notify room trigger on|off",
            "/notify remind <seconds>",
            "/notify typing on|off",
            "/notify typing current on|off",
            "/notify invite on|off",
            "/notify sub on|off")
        CMD_DESC(
            "Settings for various kinds of desktop notifications.")
        CMD_ARGS(
            { "message on|off",                 "Notifications for regular chat messages." },
            { "message current on|off",         "Whether messages in the current window trigger notifications." },
            { "message text on|off",            "Show message text in regular message notifications." },
            { "message trigger add <text>",     "Notify when specified text included in regular chat message." },
            { "message trigger remove <text>",  "Remove regular chat notification for specified text." },
            { "message trigger list",           "List all regular chat custom text notifications." },
            { "message trigger on|off",         "Enable or disable all regular chat custom text notifications." },
            { "room on|off",                    "Notifications for chat room messages, mention triggers notifications only when your nick is mentioned." },
            { "room mention on|off",            "Notifications for chat room messages when your nick is mentioned." },
            { "room current on|off",            "Whether chat room messages in the current window trigger notifications." },
            { "room text on|off",               "Show message text in chat room message notifications." },
            { "room trigger add <text>",        "Notify when specified text included in regular chat message." },
            { "room trigger remove <text>",     "Remove regular chat notification for specified text." },
            { "room trigger list",              "List all regular chat custom text notifications." },
            { "room trigger on|off",            "Enable or disable all regular chat custom text notifications." },
            { "remind <seconds>",               "Notification reminder period for unread messages, use 0 to disable." },
            { "typing on|off",                  "Notifications when contacts are typing." },
            { "typing current on|off",          "Whether typing notifications are triggered for the current window." },
            { "invite on|off",                  "Notifications for chat room invites." },
            { "sub on|off",                     "Notifications for subscription requests." })
        CMD_EXAMPLES(
            "/notify message on",
            "/notify message text on",
            "/notify room mention on",
            "/notify room current off",
            "/notify room text off",
            "/notify remind 10",
            "/notify typing on",
            "/notify invite on")
    },

    { "/flash",
        cmd_flash, parse_args, 1, 1, &cons_flash_setting,
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

    { "/intype",
        cmd_intype, parse_args, 1, 1, &cons_intype_setting,
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
        cmd_splash, parse_args, 1, 1, &cons_splash_setting,
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
        cmd_autoconnect, parse_args, 1, 2, &cons_autoconnect_setting,
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
        cmd_vercheck, parse_args, 0, 1, NULL,
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

    { "/titlebar",
        cmd_titlebar, parse_args, 2, 2, &cons_titlebar_setting,
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/titlebar show on|off",
            "/titlebar goodbye on|off")
        CMD_DESC(
            "Allow Profanity to modify the window title bar.")
        CMD_ARGS(
            { "show on|off",    "Show current logged in user, and unread messages as the window title." },
            { "goodbye on|off", "Show a message in the title when exiting profanity." })
        CMD_NOEXAMPLES
    },

    { "/alias",
        cmd_alias, parse_args_with_freetext, 1, 3, NULL,
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
            "/alias a /away \"I'm in a meeting.\"",
            "/alias remove q",
            "/alias list")
    },

    { "/chlog",
        cmd_chlog, parse_args, 1, 1, &cons_chlog_setting,
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
        cmd_grlog, parse_args, 1, 1, &cons_grlog_setting,
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
        cmd_states, parse_args, 1, 1, &cons_states_setting,
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
        cmd_pgp, parse_args, 1, 3, NULL,
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
        cmd_otr, parse_args, 1, 3, NULL,
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
        cmd_outtype, parse_args, 1, 1, &cons_outtype_setting,
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
        cmd_gone, parse_args, 1, 1, &cons_gone_setting,
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
        cmd_history, parse_args, 1, 1, &cons_history_setting,
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
        cmd_log, parse_args, 1, 2, &cons_log_setting,
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
            { "shared on|off",   "Share logs between all instances, default: on. When off, the process id will be included in the log." })
        CMD_NOEXAMPLES
    },

    { "/carbons",
        cmd_carbons, parse_args, 1, 1, &cons_carbons_setting,
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
        cmd_receipts, parse_args, 2, 2, &cons_receipts_setting,
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
        cmd_reconnect, parse_args, 1, 1, &cons_reconnect_setting,
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
        cmd_autoping, parse_args, 1, 1, &cons_autoping_setting,
        CMD_TAGS(
            CMD_TAG_CONNECTION)
        CMD_SYN(
            "/autoping <seconds>")
        CMD_DESC(
            "Set the interval between sending ping requests to the server to ensure the connection is kept alive.")
        CMD_ARGS(
            { "<seconds>", "Number of seconds between sending pings, a value of 0 disables autoping." })
        CMD_NOEXAMPLES
    },

    { "/ping",
        cmd_ping, parse_args, 0, 1, NULL,
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
        cmd_autoaway, parse_args_with_freetext, 2, 3, &cons_autoaway_setting,
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
        cmd_priority, parse_args, 1, 1, NULL,
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
        cmd_account, parse_args, 0, 4, NULL,
        CMD_TAGS(
            CMD_TAG_CONNECTION
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
            "/account set <account> tls force|allow|disable",
            "/account clear <account> password",
            "/account clear <account> eval_password",
            "/account clear <account> server",
            "/account clear <account> port",
            "/account clear <account> otr",
            "/account clear <account> pgpkeyid",
            "/account clear <account> startscript")
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
            { "set <account> resource <resource>",      "The resource to be used for this account." },
            { "set <account> password <password>",      "Password for the account, note this is currently stored in plaintext if set." },
            { "set <account> eval_password <command>",  "Shell command evaluated to retrieve password for the account. Can be used to retrieve password from keyring." },
            { "set <account> muc <service>",            "The default MUC chat service to use, defaults to 'conference.<domainpart>' where the domain part is from the account JID." },
            { "set <account> nick <nick>",              "The default nickname to use when joining chat rooms." },
            { "set <account> otr <policy>",             "Override global OTR policy for this account, see /otr." },
            { "set <account> pgpkeyid <pgpkeyid>",      "Set the ID of the PGP key for this account, see /pgp." },
            { "set <account> startscript <script>",     "Set the script to execute after connecting." },
            { "set <account> tls force",                "Force TLS connection, and fail if one cannot be established, this is default behaviour." },
            { "set <account> tls allow",                "Use TLS for the connection if it is available." },
            { "set <account> tls disable",              "Disable TLS for the connection." },
            { "clear <account> server",                 "Remove the server setting for this account." },
            { "clear <account> port",                   "Remove the port setting for this account." },
            { "clear <account> password",               "Remove the password setting for this account." },
            { "clear <account> eval_password",          "Remove the eval_password setting for this account." },
            { "clear <account> otr",                    "Remove the OTR policy setting for this account." },
            { "clear <account> pgpkeyid",               "Remove pgpkeyid associated with this account." },
            { "clear <account> startscript",            "Remove startscript associated with this account." })
        CMD_EXAMPLES(
            "/account add me",
            "/account set me jid me@chatty",
            "/account set me server talk.chat.com",
            "/account set me port 5111",
            "/account set me muc chatservice.mycompany.com",
            "/account set me nick dennis",
            "/account set me status dnd",
            "/account set me dnd -1",
            "/account rename me gtalk")
    },

    { "/prefs",
        cmd_prefs, parse_args, 0, 1, NULL,
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
        cmd_theme, parse_args, 1, 2, &cons_theme_setting,
        CMD_TAGS(
            CMD_TAG_UI)
        CMD_SYN(
            "/theme list",
            "/theme load <theme>",
            "/theme colours")
        CMD_DESC(
            "Load a theme, includes colours and UI options.")
        CMD_ARGS(
            { "list", "List all available themes." },
            { "load <theme>", "Load the specified theme. 'default' will reset to the default theme." },
            { "colours", "Show the colour values as rendered by the terminal." })
        CMD_EXAMPLES(
            "/theme list",
            "/theme load forest")
    },

    { "/statuses",
        cmd_statuses, parse_args, 2, 2, &cons_statuses_setting,
        CMD_TAGS(
            CMD_TAG_UI,
            CMD_TAG_CHAT,
            CMD_TAG_GROUPCHAT)
        CMD_SYN(
            "/statuses console|chat|muc all|online|none")
        CMD_DESC(
            "Configure which presence changes are displayed in various windows. "
            "The default is 'all' for all windows.")
        CMD_ARGS(
            { "console", "Configure what is displayed in the console window." },
            { "chat",    "Configure what is displayed in chat windows." },
            { "muc",     "Configure what is displayed in chat room windows." },
            { "all",     "Show all presence changes." },
            { "online",  "Show only online/offline changes." },
            { "none",    "Don't show any presence changes." })
        CMD_EXAMPLES(
            "/statuses console none",
            "/statuses chat online",
            "/statuses muc all")
    },

    { "/xmlconsole",
        cmd_xmlconsole, parse_args, 0, 0, NULL,
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
        cmd_away, parse_args_with_freetext, 0, 1, NULL,
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
        cmd_chat, parse_args_with_freetext, 0, 1, NULL,
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
        cmd_dnd, parse_args_with_freetext, 0, 1, NULL,
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
        cmd_online, parse_args_with_freetext, 0, 1, NULL,
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
        cmd_xa, parse_args_with_freetext, 0, 1, NULL,
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
        cmd_script, parse_args, 1, 2, NULL,
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
        cmd_export, parse_args, 1, 1, NULL,
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
    },
};

static Autocomplete commands_ac;
static Autocomplete who_room_ac;
static Autocomplete who_roster_ac;
static Autocomplete help_ac;
static Autocomplete help_commands_ac;
static Autocomplete notify_ac;
static Autocomplete notify_room_ac;
static Autocomplete notify_message_ac;
static Autocomplete notify_typing_ac;
static Autocomplete notify_trigger_ac;
static Autocomplete prefs_ac;
static Autocomplete sub_ac;
static Autocomplete log_ac;
static Autocomplete autoaway_ac;
static Autocomplete autoaway_mode_ac;
static Autocomplete autoaway_presence_ac;
static Autocomplete autoconnect_ac;
static Autocomplete titlebar_ac;
static Autocomplete theme_ac;
static Autocomplete theme_load_ac;
static Autocomplete account_ac;
static Autocomplete account_set_ac;
static Autocomplete account_clear_ac;
static Autocomplete account_default_ac;
static Autocomplete account_status_ac;
static Autocomplete disco_ac;
static Autocomplete close_ac;
static Autocomplete wins_ac;
static Autocomplete roster_ac;
static Autocomplete roster_show_ac;
static Autocomplete roster_by_ac;
static Autocomplete roster_order_ac;
static Autocomplete roster_header_ac;
static Autocomplete roster_contact_ac;
static Autocomplete roster_resource_ac;
static Autocomplete roster_presence_ac;
static Autocomplete roster_char_ac;
static Autocomplete roster_remove_all_ac;
static Autocomplete group_ac;
static Autocomplete bookmark_ac;
static Autocomplete bookmark_property_ac;
static Autocomplete otr_ac;
static Autocomplete otr_log_ac;
static Autocomplete otr_policy_ac;
static Autocomplete connect_property_ac;
static Autocomplete tls_property_ac;
static Autocomplete statuses_ac;
static Autocomplete statuses_setting_ac;
static Autocomplete alias_ac;
static Autocomplete aliases_ac;
static Autocomplete join_property_ac;
static Autocomplete room_ac;
static Autocomplete affiliation_ac;
static Autocomplete role_ac;
static Autocomplete privilege_cmd_ac;
static Autocomplete subject_ac;
static Autocomplete form_ac;
static Autocomplete form_field_multi_ac;
static Autocomplete occupants_ac;
static Autocomplete occupants_default_ac;
static Autocomplete occupants_show_ac;
static Autocomplete time_ac;
static Autocomplete time_format_ac;
static Autocomplete resource_ac;
static Autocomplete inpblock_ac;
static Autocomplete receipts_ac;
static Autocomplete pgp_ac;
static Autocomplete pgp_log_ac;
static Autocomplete tls_ac;
static Autocomplete tls_certpath_ac;
static Autocomplete script_ac;
static Autocomplete script_show_ac;

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
    while (curr) {
        ProfAlias *alias = curr->data;
        GString *ac_alias = g_string_new("/");
        g_string_append(ac_alias, alias->name);
        autocomplete_add(commands_ac, ac_alias->str);
        autocomplete_add(aliases_ac, alias->name);
        g_string_free(ac_alias, TRUE);
        curr = g_list_next(curr);
    }
    prefs_free_aliases(aliases);

    help_commands_ac = autocomplete_new();
    autocomplete_add(help_commands_ac, "chat");
    autocomplete_add(help_commands_ac, "groupchat");
    autocomplete_add(help_commands_ac, "roster");
    autocomplete_add(help_commands_ac, "presence");
    autocomplete_add(help_commands_ac, "discovery");
    autocomplete_add(help_commands_ac, "connection");
    autocomplete_add(help_commands_ac, "ui");

    prefs_ac = autocomplete_new();
    autocomplete_add(prefs_ac, "ui");
    autocomplete_add(prefs_ac, "desktop");
    autocomplete_add(prefs_ac, "chat");
    autocomplete_add(prefs_ac, "log");
    autocomplete_add(prefs_ac, "conn");
    autocomplete_add(prefs_ac, "presence");
    autocomplete_add(prefs_ac, "otr");
    autocomplete_add(prefs_ac, "pgp");

    notify_ac = autocomplete_new();
    autocomplete_add(notify_ac, "message");
    autocomplete_add(notify_ac, "room");
    autocomplete_add(notify_ac, "typing");
    autocomplete_add(notify_ac, "remind");
    autocomplete_add(notify_ac, "invite");
    autocomplete_add(notify_ac, "sub");

    notify_message_ac = autocomplete_new();
    autocomplete_add(notify_message_ac, "on");
    autocomplete_add(notify_message_ac, "off");
    autocomplete_add(notify_message_ac, "current");
    autocomplete_add(notify_message_ac, "text");
    autocomplete_add(notify_message_ac, "trigger");

    notify_room_ac = autocomplete_new();
    autocomplete_add(notify_room_ac, "on");
    autocomplete_add(notify_room_ac, "off");
    autocomplete_add(notify_room_ac, "mention");
    autocomplete_add(notify_room_ac, "current");
    autocomplete_add(notify_room_ac, "text");
    autocomplete_add(notify_room_ac, "trigger");

    notify_typing_ac = autocomplete_new();
    autocomplete_add(notify_typing_ac, "on");
    autocomplete_add(notify_typing_ac, "off");
    autocomplete_add(notify_typing_ac, "current");

    notify_trigger_ac = autocomplete_new();
    autocomplete_add(notify_trigger_ac, "add");
    autocomplete_add(notify_trigger_ac, "remove");
    autocomplete_add(notify_trigger_ac, "list");
    autocomplete_add(notify_trigger_ac, "on");
    autocomplete_add(notify_trigger_ac, "off");

    sub_ac = autocomplete_new();
    autocomplete_add(sub_ac, "request");
    autocomplete_add(sub_ac, "allow");
    autocomplete_add(sub_ac, "deny");
    autocomplete_add(sub_ac, "show");
    autocomplete_add(sub_ac, "sent");
    autocomplete_add(sub_ac, "received");

    titlebar_ac = autocomplete_new();
    autocomplete_add(titlebar_ac, "show");
    autocomplete_add(titlebar_ac, "goodbye");

    log_ac = autocomplete_new();
    autocomplete_add(log_ac, "maxsize");
    autocomplete_add(log_ac, "rotate");
    autocomplete_add(log_ac, "shared");
    autocomplete_add(log_ac, "where");

    autoaway_ac = autocomplete_new();
    autocomplete_add(autoaway_ac, "mode");
    autocomplete_add(autoaway_ac, "time");
    autocomplete_add(autoaway_ac, "message");
    autocomplete_add(autoaway_ac, "check");

    autoaway_mode_ac = autocomplete_new();
    autocomplete_add(autoaway_mode_ac, "away");
    autocomplete_add(autoaway_mode_ac, "idle");
    autocomplete_add(autoaway_mode_ac, "off");

    autoaway_presence_ac = autocomplete_new();
    autocomplete_add(autoaway_presence_ac, "away");
    autocomplete_add(autoaway_presence_ac, "xa");

    autoconnect_ac = autocomplete_new();
    autocomplete_add(autoconnect_ac, "set");
    autocomplete_add(autoconnect_ac, "off");

    theme_ac = autocomplete_new();
    autocomplete_add(theme_ac, "load");
    autocomplete_add(theme_ac, "list");
    autocomplete_add(theme_ac, "colours");

    disco_ac = autocomplete_new();
    autocomplete_add(disco_ac, "info");
    autocomplete_add(disco_ac, "items");

    account_ac = autocomplete_new();
    autocomplete_add(account_ac, "list");
    autocomplete_add(account_ac, "show");
    autocomplete_add(account_ac, "add");
    autocomplete_add(account_ac, "remove");
    autocomplete_add(account_ac, "enable");
    autocomplete_add(account_ac, "disable");
    autocomplete_add(account_ac, "default");
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
    autocomplete_add(account_set_ac, "eval_password");
    autocomplete_add(account_set_ac, "muc");
    autocomplete_add(account_set_ac, "nick");
    autocomplete_add(account_set_ac, "otr");
    autocomplete_add(account_set_ac, "pgpkeyid");
    autocomplete_add(account_set_ac, "startscript");
    autocomplete_add(account_set_ac, "tls");

    account_clear_ac = autocomplete_new();
    autocomplete_add(account_clear_ac, "password");
    autocomplete_add(account_clear_ac, "eval_password");
    autocomplete_add(account_clear_ac, "server");
    autocomplete_add(account_clear_ac, "port");
    autocomplete_add(account_clear_ac, "otr");
    autocomplete_add(account_clear_ac, "pgpkeyid");
    autocomplete_add(account_clear_ac, "startscript");

    account_default_ac = autocomplete_new();
    autocomplete_add(account_default_ac, "set");
    autocomplete_add(account_default_ac, "off");

    account_status_ac = autocomplete_new();
    autocomplete_add(account_status_ac, "online");
    autocomplete_add(account_status_ac, "chat");
    autocomplete_add(account_status_ac, "away");
    autocomplete_add(account_status_ac, "xa");
    autocomplete_add(account_status_ac, "dnd");
    autocomplete_add(account_status_ac, "last");

    close_ac = autocomplete_new();
    autocomplete_add(close_ac, "read");
    autocomplete_add(close_ac, "all");

    wins_ac = autocomplete_new();
    autocomplete_add(wins_ac, "prune");
    autocomplete_add(wins_ac, "tidy");
    autocomplete_add(wins_ac, "autotidy");
    autocomplete_add(wins_ac, "swap");

    roster_ac = autocomplete_new();
    autocomplete_add(roster_ac, "add");
    autocomplete_add(roster_ac, "online");
    autocomplete_add(roster_ac, "nick");
    autocomplete_add(roster_ac, "clearnick");
    autocomplete_add(roster_ac, "remove");
    autocomplete_add(roster_ac, "remove_all");
    autocomplete_add(roster_ac, "show");
    autocomplete_add(roster_ac, "hide");
    autocomplete_add(roster_ac, "by");
    autocomplete_add(roster_ac, "order");
    autocomplete_add(roster_ac, "size");
    autocomplete_add(roster_ac, "wrap");
    autocomplete_add(roster_ac, "header");
    autocomplete_add(roster_ac, "contact");
    autocomplete_add(roster_ac, "resource");
    autocomplete_add(roster_ac, "presence");

    roster_header_ac = autocomplete_new();
    autocomplete_add(roster_header_ac, "char");

    roster_contact_ac = autocomplete_new();
    autocomplete_add(roster_contact_ac, "char");
    autocomplete_add(roster_contact_ac, "indent");

    roster_resource_ac = autocomplete_new();
    autocomplete_add(roster_resource_ac, "char");
    autocomplete_add(roster_resource_ac, "indent");
    autocomplete_add(roster_resource_ac, "join");

    roster_presence_ac = autocomplete_new();
    autocomplete_add(roster_presence_ac, "indent");

    roster_char_ac = autocomplete_new();
    autocomplete_add(roster_char_ac, "none");

    roster_show_ac = autocomplete_new();
    autocomplete_add(roster_show_ac, "offline");
    autocomplete_add(roster_show_ac, "resource");
    autocomplete_add(roster_show_ac, "presence");
    autocomplete_add(roster_show_ac, "status");
    autocomplete_add(roster_show_ac, "empty");
    autocomplete_add(roster_show_ac, "count");
    autocomplete_add(roster_show_ac, "priority");

    roster_by_ac = autocomplete_new();
    autocomplete_add(roster_by_ac, "group");
    autocomplete_add(roster_by_ac, "presence");
    autocomplete_add(roster_by_ac, "none");

    roster_order_ac = autocomplete_new();
    autocomplete_add(roster_order_ac, "name");
    autocomplete_add(roster_order_ac, "presence");

    roster_remove_all_ac = autocomplete_new();
    autocomplete_add(roster_remove_all_ac, "contacts");

    group_ac = autocomplete_new();
    autocomplete_add(group_ac, "show");
    autocomplete_add(group_ac, "add");
    autocomplete_add(group_ac, "remove");

    theme_load_ac = NULL;

    who_roster_ac = autocomplete_new();
    autocomplete_add(who_roster_ac, "chat");
    autocomplete_add(who_roster_ac, "online");
    autocomplete_add(who_roster_ac, "away");
    autocomplete_add(who_roster_ac, "xa");
    autocomplete_add(who_roster_ac, "dnd");
    autocomplete_add(who_roster_ac, "offline");
    autocomplete_add(who_roster_ac, "available");
    autocomplete_add(who_roster_ac, "unavailable");
    autocomplete_add(who_roster_ac, "any");

    who_room_ac = autocomplete_new();
    autocomplete_add(who_room_ac, "chat");
    autocomplete_add(who_room_ac, "online");
    autocomplete_add(who_room_ac, "away");
    autocomplete_add(who_room_ac, "xa");
    autocomplete_add(who_room_ac, "dnd");
    autocomplete_add(who_room_ac, "available");
    autocomplete_add(who_room_ac, "unavailable");
    autocomplete_add(who_room_ac, "moderator");
    autocomplete_add(who_room_ac, "participant");
    autocomplete_add(who_room_ac, "visitor");
    autocomplete_add(who_room_ac, "owner");
    autocomplete_add(who_room_ac, "admin");
    autocomplete_add(who_room_ac, "member");

    bookmark_ac = autocomplete_new();
    autocomplete_add(bookmark_ac, "list");
    autocomplete_add(bookmark_ac, "add");
    autocomplete_add(bookmark_ac, "update");
    autocomplete_add(bookmark_ac, "remove");
    autocomplete_add(bookmark_ac, "join");

    bookmark_property_ac = autocomplete_new();
    autocomplete_add(bookmark_property_ac, "nick");
    autocomplete_add(bookmark_property_ac, "password");
    autocomplete_add(bookmark_property_ac, "autojoin");

    otr_ac = autocomplete_new();
    autocomplete_add(otr_ac, "gen");
    autocomplete_add(otr_ac, "start");
    autocomplete_add(otr_ac, "end");
    autocomplete_add(otr_ac, "myfp");
    autocomplete_add(otr_ac, "theirfp");
    autocomplete_add(otr_ac, "trust");
    autocomplete_add(otr_ac, "untrust");
    autocomplete_add(otr_ac, "secret");
    autocomplete_add(otr_ac, "log");
    autocomplete_add(otr_ac, "libver");
    autocomplete_add(otr_ac, "policy");
    autocomplete_add(otr_ac, "question");
    autocomplete_add(otr_ac, "answer");
    autocomplete_add(otr_ac, "char");

    otr_log_ac = autocomplete_new();
    autocomplete_add(otr_log_ac, "on");
    autocomplete_add(otr_log_ac, "off");
    autocomplete_add(otr_log_ac, "redact");

    otr_policy_ac = autocomplete_new();
    autocomplete_add(otr_policy_ac, "manual");
    autocomplete_add(otr_policy_ac, "opportunistic");
    autocomplete_add(otr_policy_ac, "always");

    connect_property_ac = autocomplete_new();
    autocomplete_add(connect_property_ac, "server");
    autocomplete_add(connect_property_ac, "port");
    autocomplete_add(connect_property_ac, "tls");

    tls_property_ac = autocomplete_new();
    autocomplete_add(tls_property_ac, "force");
    autocomplete_add(tls_property_ac, "allow");
    autocomplete_add(tls_property_ac, "disable");

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

    room_ac = autocomplete_new();
    autocomplete_add(room_ac, "accept");
    autocomplete_add(room_ac, "destroy");
    autocomplete_add(room_ac, "config");

    affiliation_ac = autocomplete_new();
    autocomplete_add(affiliation_ac, "owner");
    autocomplete_add(affiliation_ac, "admin");
    autocomplete_add(affiliation_ac, "member");
    autocomplete_add(affiliation_ac, "none");
    autocomplete_add(affiliation_ac, "outcast");

    role_ac = autocomplete_new();
    autocomplete_add(role_ac, "moderator");
    autocomplete_add(role_ac, "participant");
    autocomplete_add(role_ac, "visitor");
    autocomplete_add(role_ac, "none");

    privilege_cmd_ac = autocomplete_new();
    autocomplete_add(privilege_cmd_ac, "list");
    autocomplete_add(privilege_cmd_ac, "set");

    subject_ac = autocomplete_new();
    autocomplete_add(subject_ac, "set");
    autocomplete_add(subject_ac, "edit");
    autocomplete_add(subject_ac, "prepend");
    autocomplete_add(subject_ac, "append");
    autocomplete_add(subject_ac, "clear");

    form_ac = autocomplete_new();
    autocomplete_add(form_ac, "submit");
    autocomplete_add(form_ac, "cancel");
    autocomplete_add(form_ac, "show");
    autocomplete_add(form_ac, "help");

    form_field_multi_ac = autocomplete_new();
    autocomplete_add(form_field_multi_ac, "add");
    autocomplete_add(form_field_multi_ac, "remove");

    occupants_ac = autocomplete_new();
    autocomplete_add(occupants_ac, "show");
    autocomplete_add(occupants_ac, "hide");
    autocomplete_add(occupants_ac, "default");
    autocomplete_add(occupants_ac, "size");

    occupants_default_ac = autocomplete_new();
    autocomplete_add(occupants_default_ac, "show");
    autocomplete_add(occupants_default_ac, "hide");

    occupants_show_ac = autocomplete_new();
    autocomplete_add(occupants_show_ac, "jid");

    time_ac = autocomplete_new();
    autocomplete_add(time_ac, "console");
    autocomplete_add(time_ac, "chat");
    autocomplete_add(time_ac, "muc");
    autocomplete_add(time_ac, "mucconfig");
    autocomplete_add(time_ac, "private");
    autocomplete_add(time_ac, "xml");
    autocomplete_add(time_ac, "statusbar");
    autocomplete_add(time_ac, "lastactivity");

    time_format_ac = autocomplete_new();
    autocomplete_add(time_format_ac, "set");
    autocomplete_add(time_format_ac, "off");

    resource_ac = autocomplete_new();
    autocomplete_add(resource_ac, "set");
    autocomplete_add(resource_ac, "off");
    autocomplete_add(resource_ac, "title");
    autocomplete_add(resource_ac, "message");

    inpblock_ac = autocomplete_new();
    autocomplete_add(inpblock_ac, "timeout");
    autocomplete_add(inpblock_ac, "dynamic");

    receipts_ac = autocomplete_new();
    autocomplete_add(receipts_ac, "send");
    autocomplete_add(receipts_ac, "request");

    pgp_ac = autocomplete_new();
    autocomplete_add(pgp_ac, "keys");
    autocomplete_add(pgp_ac, "contacts");
    autocomplete_add(pgp_ac, "setkey");
    autocomplete_add(pgp_ac, "libver");
    autocomplete_add(pgp_ac, "start");
    autocomplete_add(pgp_ac, "end");
    autocomplete_add(pgp_ac, "log");
    autocomplete_add(pgp_ac, "char");

    pgp_log_ac = autocomplete_new();
    autocomplete_add(pgp_log_ac, "on");
    autocomplete_add(pgp_log_ac, "off");
    autocomplete_add(pgp_log_ac, "redact");

    tls_ac = autocomplete_new();
    autocomplete_add(tls_ac, "allow");
    autocomplete_add(tls_ac, "always");
    autocomplete_add(tls_ac, "deny");
    autocomplete_add(tls_ac, "cert");
    autocomplete_add(tls_ac, "trust");
    autocomplete_add(tls_ac, "trusted");
    autocomplete_add(tls_ac, "revoke");
    autocomplete_add(tls_ac, "certpath");
    autocomplete_add(tls_ac, "show");

    tls_certpath_ac = autocomplete_new();
    autocomplete_add(tls_certpath_ac, "set");
    autocomplete_add(tls_certpath_ac, "clear");

    script_ac = autocomplete_new();
    autocomplete_add(script_ac, "run");
    autocomplete_add(script_ac, "list");
    autocomplete_add(script_ac, "show");

    script_show_ac = NULL;
}

void
cmd_uninit(void)
{
    autocomplete_free(commands_ac);
    autocomplete_free(who_room_ac);
    autocomplete_free(who_roster_ac);
    autocomplete_free(help_ac);
    autocomplete_free(help_commands_ac);
    autocomplete_free(notify_ac);
    autocomplete_free(notify_message_ac);
    autocomplete_free(notify_room_ac);
    autocomplete_free(notify_typing_ac);
    autocomplete_free(notify_trigger_ac);
    autocomplete_free(sub_ac);
    autocomplete_free(titlebar_ac);
    autocomplete_free(log_ac);
    autocomplete_free(prefs_ac);
    autocomplete_free(autoaway_ac);
    autocomplete_free(autoaway_mode_ac);
    autocomplete_free(autoaway_presence_ac);
    autocomplete_free(autoconnect_ac);
    autocomplete_free(theme_ac);
    autocomplete_free(theme_load_ac);
    autocomplete_free(account_ac);
    autocomplete_free(account_set_ac);
    autocomplete_free(account_clear_ac);
    autocomplete_free(account_default_ac);
    autocomplete_free(account_status_ac);
    autocomplete_free(disco_ac);
    autocomplete_free(close_ac);
    autocomplete_free(wins_ac);
    autocomplete_free(roster_ac);
    autocomplete_free(roster_header_ac);
    autocomplete_free(roster_contact_ac);
    autocomplete_free(roster_resource_ac);
    autocomplete_free(roster_presence_ac);
    autocomplete_free(roster_char_ac);
    autocomplete_free(roster_show_ac);
    autocomplete_free(roster_by_ac);
    autocomplete_free(roster_order_ac);
    autocomplete_free(roster_remove_all_ac);
    autocomplete_free(group_ac);
    autocomplete_free(bookmark_ac);
    autocomplete_free(bookmark_property_ac);
    autocomplete_free(otr_ac);
    autocomplete_free(otr_log_ac);
    autocomplete_free(otr_policy_ac);
    autocomplete_free(connect_property_ac);
    autocomplete_free(tls_property_ac);
    autocomplete_free(statuses_ac);
    autocomplete_free(statuses_setting_ac);
    autocomplete_free(alias_ac);
    autocomplete_free(aliases_ac);
    autocomplete_free(join_property_ac);
    autocomplete_free(room_ac);
    autocomplete_free(affiliation_ac);
    autocomplete_free(role_ac);
    autocomplete_free(privilege_cmd_ac);
    autocomplete_free(subject_ac);
    autocomplete_free(form_ac);
    autocomplete_free(form_field_multi_ac);
    autocomplete_free(occupants_ac);
    autocomplete_free(occupants_default_ac);
    autocomplete_free(occupants_show_ac);
    autocomplete_free(time_ac);
    autocomplete_free(time_format_ac);
    autocomplete_free(resource_ac);
    autocomplete_free(inpblock_ac);
    autocomplete_free(receipts_ac);
    autocomplete_free(pgp_ac);
    autocomplete_free(pgp_log_ac);
    autocomplete_free(tls_ac);
    autocomplete_free(tls_certpath_ac);
    autocomplete_free(script_ac);
    autocomplete_free(script_show_ac);
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
    if (commands_ac) {
        autocomplete_add(commands_ac, value);
    }
}

void
cmd_autocomplete_add_form_fields(DataForm *form)
{
    if (form == NULL) {
        return;
    }

    GSList *fields = autocomplete_create_list(form->tag_ac);
    GSList *curr_field = fields;
    while (curr_field) {
        GString *field_str = g_string_new("/");
        g_string_append(field_str, curr_field->data);
        cmd_autocomplete_add(field_str->str);
        g_string_free(field_str, TRUE);
        curr_field = g_slist_next(curr_field);
    }
    g_slist_free_full(fields, free);
}

void
cmd_autocomplete_remove_form_fields(DataForm *form)
{
    if (form == NULL) {
        return;
    }

    GSList *fields = autocomplete_create_list(form->tag_ac);
    GSList *curr_field = fields;
    while (curr_field) {
        GString *field_str = g_string_new("/");
        g_string_append(field_str, curr_field->data);
        cmd_autocomplete_remove(field_str->str);
        g_string_free(field_str, TRUE);
        curr_field = g_slist_next(curr_field);
    }
    g_slist_free_full(fields, free);
}

void
cmd_autocomplete_remove(char *value)
{
    if (commands_ac) {
        autocomplete_remove(commands_ac, value);
    }
}

void
cmd_alias_add(char *value)
{
    if (aliases_ac) {
        autocomplete_add(aliases_ac, value);
    }
}

void
cmd_alias_remove(char *value)
{
    if (aliases_ac) {
        autocomplete_remove(aliases_ac, value);
    }
}

// Command autocompletion functions
char*
cmd_autocomplete(ProfWin *window, const char *const input)
{
    // autocomplete command
    if ((strncmp(input, "/", 1) == 0) && (!str_contains(input, strlen(input), ' '))) {
        char *found = NULL;
        found = autocomplete_complete(commands_ac, input, TRUE);
        if (found) {
            return found;
        }

    // autocomplete parameters
    } else {
        char *found = _cmd_complete_parameters(window, input);
        if (found) {
            return found;
        }
    }

    return NULL;
}

void
cmd_reset_autocomplete(ProfWin *window)
{
    roster_reset_search_attempts();
    muc_invites_reset_ac();
    accounts_reset_all_search();
    accounts_reset_enabled_search();
    tlscerts_reset_ac();
    prefs_reset_boolean_choice();
    presence_reset_sub_request_search();
#ifdef HAVE_LIBGPGME
    p_gpg_autocomplete_key_reset();
#endif
    autocomplete_reset(help_ac);
    autocomplete_reset(help_commands_ac);
    autocomplete_reset(notify_ac);
    autocomplete_reset(notify_message_ac);
    autocomplete_reset(notify_room_ac);
    autocomplete_reset(notify_typing_ac);
    autocomplete_reset(notify_trigger_ac);
    autocomplete_reset(sub_ac);

    autocomplete_reset(who_room_ac);
    autocomplete_reset(who_roster_ac);
    autocomplete_reset(prefs_ac);
    autocomplete_reset(log_ac);
    autocomplete_reset(commands_ac);
    autocomplete_reset(autoaway_ac);
    autocomplete_reset(autoaway_mode_ac);
    autocomplete_reset(autoaway_presence_ac);
    autocomplete_reset(autoconnect_ac);
    autocomplete_reset(theme_ac);
    if (theme_load_ac) {
        autocomplete_free(theme_load_ac);
        theme_load_ac = NULL;
    }
    autocomplete_reset(account_ac);
    autocomplete_reset(account_set_ac);
    autocomplete_reset(account_clear_ac);
    autocomplete_reset(account_default_ac);
    autocomplete_reset(account_status_ac);
    autocomplete_reset(disco_ac);
    autocomplete_reset(close_ac);
    autocomplete_reset(wins_ac);
    autocomplete_reset(roster_ac);
    autocomplete_reset(roster_header_ac);
    autocomplete_reset(roster_contact_ac);
    autocomplete_reset(roster_resource_ac);
    autocomplete_reset(roster_presence_ac);
    autocomplete_reset(roster_char_ac);
    autocomplete_reset(roster_show_ac);
    autocomplete_reset(roster_by_ac);
    autocomplete_reset(roster_order_ac);
    autocomplete_reset(roster_remove_all_ac);
    autocomplete_reset(group_ac);
    autocomplete_reset(titlebar_ac);
    autocomplete_reset(bookmark_ac);
    autocomplete_reset(bookmark_property_ac);
    autocomplete_reset(otr_ac);
    autocomplete_reset(otr_log_ac);
    autocomplete_reset(otr_policy_ac);
    autocomplete_reset(connect_property_ac);
    autocomplete_reset(tls_property_ac);
    autocomplete_reset(statuses_ac);
    autocomplete_reset(statuses_setting_ac);
    autocomplete_reset(alias_ac);
    autocomplete_reset(aliases_ac);
    autocomplete_reset(join_property_ac);
    autocomplete_reset(room_ac);
    autocomplete_reset(affiliation_ac);
    autocomplete_reset(role_ac);
    autocomplete_reset(privilege_cmd_ac);
    autocomplete_reset(subject_ac);
    autocomplete_reset(form_ac);
    autocomplete_reset(form_field_multi_ac);
    autocomplete_reset(occupants_ac);
    autocomplete_reset(occupants_default_ac);
    autocomplete_reset(occupants_show_ac);
    autocomplete_reset(time_ac);
    autocomplete_reset(time_format_ac);
    autocomplete_reset(resource_ac);
    autocomplete_reset(inpblock_ac);
    autocomplete_reset(receipts_ac);
    autocomplete_reset(pgp_ac);
    autocomplete_reset(pgp_log_ac);
    autocomplete_reset(tls_ac);
    autocomplete_reset(tls_certpath_ac);
    autocomplete_reset(script_ac);
    if (script_show_ac) {
        autocomplete_free(script_show_ac);
        script_show_ac = NULL;
    }

    if (window->type == WIN_CHAT) {
        ProfChatWin *chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        PContact contact = roster_get_contact(chatwin->barejid);
        if (contact) {
            p_contact_resource_ac_reset(contact);
        }
    }

    if (window->type == WIN_MUC) {
        ProfMucWin *mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        muc_autocomplete_reset(mucwin->roomjid);
        muc_jid_autocomplete_reset(mucwin->roomjid);
    }

    if (window->type == WIN_MUC_CONFIG) {
        ProfMucConfWin *confwin = (ProfMucConfWin*)window;
        assert(confwin->memcheck == PROFCONFWIN_MEMCHECK);
        if (confwin->form) {
            form_reset_autocompleters(confwin->form);
        }
    }

    bookmark_autocomplete_reset();
    prefs_reset_message_trigger_ac();
    prefs_reset_room_trigger_ac();
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
        (g_strcmp0(str, CMD_TAG_UI) == 0));
}

gboolean
cmd_has_tag(Command *pcmd, const char *const tag)
{
    int i = 0;
    for (i = 0; pcmd->help.tags[i] != NULL; i++) {
        if (g_strcmp0(tag, pcmd->help.tags[i]) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * Take a line of input and process it, return TRUE if profanity is to
 * continue, FALSE otherwise
 */
gboolean
cmd_process_input(ProfWin *window, char *inp)
{
    log_debug("Input received: %s", inp);
    gboolean result = FALSE;
    g_strchomp(inp);

    // just carry on if no input
    if (strlen(inp) == 0) {
        result = TRUE;

    // handle command if input starts with a '/'
    } else if (inp[0] == '/') {
        char *inp_cpy = strdup(inp);
        char *command = strtok(inp_cpy, " ");
        result = _cmd_execute(window, command, inp);
        free(inp_cpy);

    // call a default handler if input didn't start with '/'
    } else {
        result = cmd_execute_default(window, inp);
    }

    return result;
}

// Command execution

void
cmd_execute_connect(ProfWin *window, const char *const account)
{
    GString *command = g_string_new("/connect ");
    g_string_append(command, account);
    cmd_process_input(window, command->str);
    g_string_free(command, TRUE);
}

static gboolean
_cmd_execute(ProfWin *window, const char *const command, const char *const inp)
{
    if (g_str_has_prefix(command, "/field") && window->type == WIN_MUC_CONFIG) {
        gboolean result = FALSE;
        gchar **args = parse_args_with_freetext(inp, 1, 2, &result);
        if (!result) {
            ui_current_print_formatted_line('!', 0, "Invalid command, see /form help");
            result = TRUE;
        } else {
            gchar **tokens = g_strsplit(inp, " ", 2);
            char *field = tokens[0] + 1;
            result = cmd_form_field(window, field, args);
            g_strfreev(tokens);
        }

        g_strfreev(args);
        return result;
    }

    Command *cmd = g_hash_table_lookup(commands, command);
    gboolean result = FALSE;

    if (cmd) {
        gchar **args = cmd->parser(inp, cmd->min_args, cmd->max_args, &result);
        if (result == FALSE) {
            ui_invalid_command_usage(cmd->cmd, cmd->setting_func);
            return TRUE;
        } else {
            gboolean result = cmd->func(window, command, args);
            g_strfreev(args);
            return result;
        }
    } else {
        gboolean ran_alias = FALSE;
        gboolean alias_result = cmd_execute_alias(window, inp, &ran_alias);
        if (!ran_alias) {
            return cmd_execute_default(window, inp);
        } else {
            return alias_result;
        }
    }
}

static char*
_cmd_complete_parameters(ProfWin *window, const char *const input)
{
    int i;
    char *result = NULL;

    // autocomplete boolean settings
    gchar *boolean_choices[] = { "/beep", "/intype", "/states", "/outtype",
        "/flash", "/splash", "/chlog", "/grlog", "/history", "/vercheck",
        "/privileges", "/presence", "/wrap", "/winstidy", "/carbons", "/encwarn", "/lastactivity" };

    for (i = 0; i < ARRAY_SIZE(boolean_choices); i++) {
        result = autocomplete_param_with_func(input, boolean_choices[i], prefs_autocomplete_boolean_choice);
        if (result) {
            return result;
        }
    }

    // autocomplete nickname in chat rooms
    if (window->type == WIN_MUC) {
        ProfMucWin *mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        Autocomplete nick_ac = muc_roster_ac(mucwin->roomjid);
        if (nick_ac) {
            gchar *nick_choices[] = { "/msg", "/info", "/caps", "/status", "/software" } ;

            // Remove quote character before and after names when doing autocomplete
            char *unquoted = strip_arg_quotes(input);
            for (i = 0; i < ARRAY_SIZE(nick_choices); i++) {
                result = autocomplete_param_with_ac(unquoted, nick_choices[i], nick_ac, TRUE);
                if (result) {
                    free(unquoted);
                    return result;
                }
            }
            free(unquoted);
        }

    // otherwise autocomplete using roster
    } else {
        gchar *contact_choices[] = { "/msg", "/info", "/status" };
        // Remove quote character before and after names when doing autocomplete
        char *unquoted = strip_arg_quotes(input);
        for (i = 0; i < ARRAY_SIZE(contact_choices); i++) {
            result = autocomplete_param_with_func(unquoted, contact_choices[i], roster_contact_autocomplete);
            if (result) {
                free(unquoted);
                return result;
            }
        }
        free(unquoted);

        gchar *resource_choices[] = { "/caps", "/software", "/ping" };
        for (i = 0; i < ARRAY_SIZE(resource_choices); i++) {
            result = autocomplete_param_with_func(input, resource_choices[i], roster_fulljid_autocomplete);
            if (result) {
                return result;
            }
        }
    }

    result = autocomplete_param_with_func(input, "/invite", roster_contact_autocomplete);
    if (result) {
        return result;
    }

    gchar *invite_choices[] = { "/decline", "/join" };
    for (i = 0; i < ARRAY_SIZE(invite_choices); i++) {
        result = autocomplete_param_with_func(input, invite_choices[i], muc_invites_find);
        if (result) {
            return result;
        }
    }

    gchar *cmds[] = { "/prefs", "/disco", "/close", "/room" };
    Autocomplete completers[] = { prefs_ac, disco_ac, close_ac, room_ac };

    for (i = 0; i < ARRAY_SIZE(cmds); i++) {
        result = autocomplete_param_with_ac(input, cmds[i], completers[i], TRUE);
        if (result) {
            return result;
        }
    }

    GHashTable *ac_funcs = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(ac_funcs, "/help",          _help_autocomplete);
    g_hash_table_insert(ac_funcs, "/who",           _who_autocomplete);
    g_hash_table_insert(ac_funcs, "/sub",           _sub_autocomplete);
    g_hash_table_insert(ac_funcs, "/notify",        _notify_autocomplete);
    g_hash_table_insert(ac_funcs, "/autoaway",      _autoaway_autocomplete);
    g_hash_table_insert(ac_funcs, "/theme",         _theme_autocomplete);
    g_hash_table_insert(ac_funcs, "/log",           _log_autocomplete);
    g_hash_table_insert(ac_funcs, "/account",       _account_autocomplete);
    g_hash_table_insert(ac_funcs, "/roster",        _roster_autocomplete);
    g_hash_table_insert(ac_funcs, "/group",         _group_autocomplete);
    g_hash_table_insert(ac_funcs, "/bookmark",      _bookmark_autocomplete);
    g_hash_table_insert(ac_funcs, "/autoconnect",   _autoconnect_autocomplete);
    g_hash_table_insert(ac_funcs, "/otr",           _otr_autocomplete);
    g_hash_table_insert(ac_funcs, "/pgp",           _pgp_autocomplete);
    g_hash_table_insert(ac_funcs, "/connect",       _connect_autocomplete);
    g_hash_table_insert(ac_funcs, "/statuses",      _statuses_autocomplete);
    g_hash_table_insert(ac_funcs, "/alias",         _alias_autocomplete);
    g_hash_table_insert(ac_funcs, "/join",          _join_autocomplete);
    g_hash_table_insert(ac_funcs, "/form",          _form_autocomplete);
    g_hash_table_insert(ac_funcs, "/occupants",     _occupants_autocomplete);
    g_hash_table_insert(ac_funcs, "/kick",          _kick_autocomplete);
    g_hash_table_insert(ac_funcs, "/ban",           _ban_autocomplete);
    g_hash_table_insert(ac_funcs, "/affiliation",   _affiliation_autocomplete);
    g_hash_table_insert(ac_funcs, "/role",          _role_autocomplete);
    g_hash_table_insert(ac_funcs, "/resource",      _resource_autocomplete);
    g_hash_table_insert(ac_funcs, "/titlebar",      _titlebar_autocomplete);
    g_hash_table_insert(ac_funcs, "/inpblock",      _inpblock_autocomplete);
    g_hash_table_insert(ac_funcs, "/time",          _time_autocomplete);
    g_hash_table_insert(ac_funcs, "/receipts",      _receipts_autocomplete);
    g_hash_table_insert(ac_funcs, "/wins",          _wins_autocomplete);
    g_hash_table_insert(ac_funcs, "/tls",           _tls_autocomplete);
    g_hash_table_insert(ac_funcs, "/script",        _script_autocomplete);
    g_hash_table_insert(ac_funcs, "/subject",        _subject_autocomplete);

    int len = strlen(input);
    char parsed[len+1];
    i = 0;
    while (i < len) {
        if (input[i] == ' ') {
            break;
        } else {
            parsed[i] = input[i];
        }
        i++;
    }
    parsed[i] = '\0';

    char * (*ac_func)(ProfWin*, const char * const) = g_hash_table_lookup(ac_funcs, parsed);
    if (ac_func) {
        result = ac_func(window, input);
        if (result) {
            g_hash_table_destroy(ac_funcs);
            return result;
        }
    }
    g_hash_table_destroy(ac_funcs);

    if (g_str_has_prefix(input, "/field")) {
        result = _form_field_autocomplete(window, input);
        if (result) {
            return result;
        }
    }

    return NULL;
}

static char*
_sub_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;
    result = autocomplete_param_with_func(input, "/sub allow", presence_sub_request_find);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_func(input, "/sub deny", presence_sub_request_find);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/sub", sub_ac, TRUE);
    if (result) {
        return result;
    }

    return NULL;
}

static char*
_who_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;

    if (window->type == WIN_MUC) {
        result = autocomplete_param_with_ac(input, "/who", who_room_ac, TRUE);
        if (result) {
            return result;
        }
    } else {
        int i = 0;
        gchar *group_commands[] = { "/who any", "/who online", "/who offline",
            "/who chat", "/who away", "/who xa", "/who dnd", "/who available",
            "/who unavailable" };

        for (i = 0; i < ARRAY_SIZE(group_commands); i++) {
            result = autocomplete_param_with_func(input, group_commands[i], roster_group_autocomplete);
            if (result) {
                return result;
            }
        }

        result = autocomplete_param_with_ac(input, "/who", who_roster_ac, TRUE);
        if (result) {
            return result;
        }
    }

    return NULL;
}

static char*
_roster_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;
    result = autocomplete_param_with_ac(input, "/roster header char", roster_char_ac, TRUE);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/roster contact char", roster_char_ac, TRUE);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/roster resource char", roster_char_ac, TRUE);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_func(input, "/roster resource join", prefs_autocomplete_boolean_choice);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_func(input, "/roster nick", roster_barejid_autocomplete);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_func(input, "/roster clearnick", roster_barejid_autocomplete);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_func(input, "/roster remove", roster_barejid_autocomplete);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/roster remove_all", roster_remove_all_ac, TRUE);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/roster show", roster_show_ac, TRUE);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/roster hide", roster_show_ac, TRUE);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/roster by", roster_by_ac, TRUE);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/roster order", roster_order_ac, TRUE);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_func(input, "/roster wrap", prefs_autocomplete_boolean_choice);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/roster header", roster_header_ac, TRUE);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/roster contact", roster_contact_ac, TRUE);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/roster resource", roster_resource_ac, TRUE);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/roster presence", roster_presence_ac, TRUE);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/roster", roster_ac, TRUE);
    if (result) {
        return result;
    }

    return NULL;
}

static char*
_group_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;
    result = autocomplete_param_with_func(input, "/group show", roster_group_autocomplete);
    if (result) {
        return result;
    }

    result = autocomplete_param_no_with_func(input, "/group add", 4, roster_contact_autocomplete);
    if (result) {
        return result;
    }
    result = autocomplete_param_no_with_func(input, "/group remove", 4, roster_contact_autocomplete);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_func(input, "/group add", roster_group_autocomplete);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_func(input, "/group remove", roster_group_autocomplete);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/group", group_ac, TRUE);
    if (result) {
        return result;
    }

    return NULL;
}

static char*
_bookmark_autocomplete(ProfWin *window, const char *const input)
{
    char *found = NULL;

    gboolean result;
    gchar **args = parse_args(input, 3, 8, &result);
    gboolean handle_options = result && (g_strv_length(args) > 2);

    if (handle_options && ((strcmp(args[0], "add") == 0) || (strcmp(args[0], "update") == 0)) ) {
        GString *beginning = g_string_new("/bookmark");
        gboolean autojoin = FALSE;
        int num_args = g_strv_length(args);

        g_string_append(beginning, " ");
        g_string_append(beginning, args[0]);
        g_string_append(beginning, " ");
        g_string_append(beginning, args[1]);
        if (num_args == 4 && g_strcmp0(args[2], "autojoin") == 0) {
            g_string_append(beginning, " ");
            g_string_append(beginning, args[2]);
            autojoin = TRUE;
        }

        if (num_args > 4) {
            g_string_append(beginning, " ");
            g_string_append(beginning, args[2]);
            g_string_append(beginning, " ");
            g_string_append(beginning, args[3]);
            if (num_args == 6 && g_strcmp0(args[4], "autojoin") == 0) {
                g_string_append(beginning, " ");
                g_string_append(beginning, args[4]);
                autojoin = TRUE;
            }
        }

        if (num_args > 6) {
            g_string_append(beginning, " ");
            g_string_append(beginning, args[4]);
            g_string_append(beginning, " ");
            g_string_append(beginning, args[5]);
            if (num_args == 8 && g_strcmp0(args[6], "autojoin") == 0) {
                g_string_append(beginning, " ");
                g_string_append(beginning, args[6]);
                autojoin = TRUE;
            }
        }

        if (autojoin) {
            found = autocomplete_param_with_func(input, beginning->str, prefs_autocomplete_boolean_choice);
        } else {
            found = autocomplete_param_with_ac(input, beginning->str, bookmark_property_ac, TRUE);
        }
        g_string_free(beginning, TRUE);
        if (found) {
            g_strfreev(args);
            return found;
        }
    }

    g_strfreev(args);

    found = autocomplete_param_with_func(input, "/bookmark remove", bookmark_find);
    if (found) {
        return found;
    }
    found = autocomplete_param_with_func(input, "/bookmark join", bookmark_find);
    if (found) {
        return found;
    }
    found = autocomplete_param_with_func(input, "/bookmark update", bookmark_find);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/bookmark", bookmark_ac, TRUE);
    return found;
}

static char*
_notify_autocomplete(ProfWin *window, const char *const input)
{
    int i = 0;
    char *result = NULL;

    result = autocomplete_param_with_func(input, "/notify message trigger remove", prefs_autocomplete_message_trigger);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_func(input, "/notify room trigger remove", prefs_autocomplete_room_trigger);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_func(input, "/notify room current", prefs_autocomplete_boolean_choice);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/notify room trigger", notify_trigger_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_func(input, "/notify message current", prefs_autocomplete_boolean_choice);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/notify message trigger", notify_trigger_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_func(input, "/notify typing current", prefs_autocomplete_boolean_choice);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_func(input, "/notify room text", prefs_autocomplete_boolean_choice);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_func(input, "/notify room mention", prefs_autocomplete_boolean_choice);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_func(input, "/notify message text", prefs_autocomplete_boolean_choice);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/notify room", notify_room_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/notify message", notify_message_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/notify typing", notify_typing_ac, TRUE);
    if (result) {
        return result;
    }

    gchar *boolean_choices[] = { "/notify invite", "/notify sub" };
    for (i = 0; i < ARRAY_SIZE(boolean_choices); i++) {
        result = autocomplete_param_with_func(input, boolean_choices[i],
            prefs_autocomplete_boolean_choice);
        if (result) {
            return result;
        }
    }

    result = autocomplete_param_with_ac(input, "/notify", notify_ac, TRUE);
    if (result) {
        return result;
    }

    return NULL;
}

static char*
_autoaway_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;

    result = autocomplete_param_with_ac(input, "/autoaway mode", autoaway_mode_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/autoaway time", autoaway_presence_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/autoaway message", autoaway_presence_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_func(input, "/autoaway check", prefs_autocomplete_boolean_choice);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/autoaway", autoaway_ac, TRUE);
    if (result) {
        return result;
    }

    return NULL;
}

static char*
_log_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;

    result = autocomplete_param_with_func(input, "/log rotate",
        prefs_autocomplete_boolean_choice);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_func(input, "/log shared",
        prefs_autocomplete_boolean_choice);
    if (result) {
        return result;
    }
    result = autocomplete_param_with_ac(input, "/log", log_ac, TRUE);
    if (result) {
        return result;
    }

    return NULL;
}

static char*
_autoconnect_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;

    result = autocomplete_param_with_func(input, "/autoconnect set", accounts_find_enabled);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/autoconnect", autoconnect_ac, TRUE);
    if (result) {
        return result;
    }

    return NULL;
}

static char*
_otr_autocomplete(ProfWin *window, const char *const input)
{
    char *found = NULL;

    found = autocomplete_param_with_func(input, "/otr start", roster_contact_autocomplete);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/otr log", otr_log_ac, TRUE);
    if (found) {
        return found;
    }

    // /otr policy always user@server.com
    gboolean result;
    gchar **args = parse_args(input, 3, 3, &result);
    if (result && (strcmp(args[0], "policy") == 0)) {
        GString *beginning = g_string_new("/otr ");
        g_string_append(beginning, args[0]);
        g_string_append(beginning, " ");
        g_string_append(beginning, args[1]);

        found = autocomplete_param_with_func(input, beginning->str, roster_contact_autocomplete);
        g_string_free(beginning, TRUE);
        if (found) {
            g_strfreev(args);
            return found;
        }
    }

    g_strfreev(args);

    found = autocomplete_param_with_ac(input, "/otr policy", otr_policy_ac, TRUE);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/otr", otr_ac, TRUE);
    if (found) {
        return found;
    }

    return NULL;
}

static char*
_pgp_autocomplete(ProfWin *window, const char *const input)
{
    char *found = NULL;

    found = autocomplete_param_with_func(input, "/pgp start", roster_contact_autocomplete);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/pgp log", pgp_log_ac, TRUE);
    if (found) {
        return found;
    }

#ifdef HAVE_LIBGPGME
    gboolean result;
    gchar **args = parse_args(input, 2, 3, &result);
    if ((strncmp(input, "/pgp", 4) == 0) && (result == TRUE)) {
        GString *beginning = g_string_new("/pgp ");
        g_string_append(beginning, args[0]);
        if (args[1]) {
            g_string_append(beginning, " ");
            g_string_append(beginning, args[1]);
        }
        found = autocomplete_param_with_func(input, beginning->str, p_gpg_autocomplete_key);
        g_string_free(beginning, TRUE);
        if (found) {
            g_strfreev(args);
            return found;
        }
    }
    g_strfreev(args);
#endif

    found = autocomplete_param_with_func(input, "/pgp setkey", roster_barejid_autocomplete);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/pgp", pgp_ac, TRUE);
    if (found) {
        return found;
    }

    return NULL;
}

static char*
_theme_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;
    if ((strncmp(input, "/theme load ", 12) == 0) && (strlen(input) > 12)) {
        if (theme_load_ac == NULL) {
            theme_load_ac = autocomplete_new();
            GSList *themes = theme_list();
            GSList *curr = themes;
            while (curr) {
                autocomplete_add(theme_load_ac, curr->data);
                curr = g_slist_next(curr);
            }
            g_slist_free_full(themes, g_free);
            autocomplete_add(theme_load_ac, "default");
        }
        result = autocomplete_param_with_ac(input, "/theme load", theme_load_ac, TRUE);
        if (result) {
            return result;
        }
    }
    result = autocomplete_param_with_ac(input, "/theme", theme_ac, TRUE);
    if (result) {
        return result;
    }

    return NULL;
}

static char*
_script_autocomplete_func(const char *const prefix)
{
    if (script_show_ac == NULL) {
        script_show_ac = autocomplete_new();
        GSList *scripts = scripts_list();
        GSList *curr = scripts;
        while (curr) {
            autocomplete_add(script_show_ac, curr->data);
            curr = g_slist_next(curr);
        }
        g_slist_free_full(scripts, g_free);
    }

    return autocomplete_complete(script_show_ac, prefix, FALSE);
}


static char*
_script_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;
    if ((strncmp(input, "/script show ", 13) == 0) && (strlen(input) > 13)) {
        result = autocomplete_param_with_func(input, "/script show", _script_autocomplete_func);
        if (result) {
            return result;
        }
    }

    if ((strncmp(input, "/script run ", 12) == 0) && (strlen(input) > 12)) {
        result = autocomplete_param_with_func(input, "/script run", _script_autocomplete_func);
        if (result) {
            return result;
        }
    }

    result = autocomplete_param_with_ac(input, "/script", script_ac, TRUE);
    if (result) {
        return result;
    }

    return NULL;
}

static char*
_resource_autocomplete(ProfWin *window, const char *const input)
{
    char *found = NULL;

    if (window->type == WIN_CHAT) {
        ProfChatWin *chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        PContact contact = roster_get_contact(chatwin->barejid);
        if (contact) {
            Autocomplete ac = p_contact_resource_ac(contact);
            found = autocomplete_param_with_ac(input, "/resource set", ac, FALSE);
            if (found) {
                return found;
            }
        }
    }

    found = autocomplete_param_with_func(input, "/resource title", prefs_autocomplete_boolean_choice);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_func(input, "/resource message", prefs_autocomplete_boolean_choice);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/resource", resource_ac, FALSE);
    if (found) {
        return found;
    }

    return NULL;
}

static char*
_titlebar_autocomplete(ProfWin *window, const char *const input)
{
    char *found = NULL;

    found = autocomplete_param_with_func(input, "/titlebar show", prefs_autocomplete_boolean_choice);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_func(input, "/titlebar goodbye", prefs_autocomplete_boolean_choice);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/titlebar", titlebar_ac, FALSE);
    if (found) {
        return found;
    }

    return NULL;
}

static char*
_inpblock_autocomplete(ProfWin *window, const char *const input)
{
    char *found = NULL;

    found = autocomplete_param_with_func(input, "/inpblock dynamic", prefs_autocomplete_boolean_choice);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/inpblock", inpblock_ac, FALSE);
    if (found) {
        return found;
    }

    return NULL;
}

static char*
_form_autocomplete(ProfWin *window, const char *const input)
{
    if (window->type != WIN_MUC_CONFIG) {
        return NULL;
    }

    char *found = NULL;

    ProfMucConfWin *confwin = (ProfMucConfWin*)window;
    DataForm *form = confwin->form;
    if (form) {
        found = autocomplete_param_with_ac(input, "/form help", form->tag_ac, TRUE);
        if (found) {
            return found;
        }
    }

    found = autocomplete_param_with_ac(input, "/form", form_ac, TRUE);
    if (found) {
        return found;
    }

    return NULL;
}

static char*
_form_field_autocomplete(ProfWin *window, const char *const input)
{
    if (window->type != WIN_MUC_CONFIG) {
        return NULL;
    }

    char *found = NULL;

    ProfMucConfWin *confwin = (ProfMucConfWin*)window;
    DataForm *form = confwin->form;
    if (form == NULL) {
        return NULL;
    }

    gchar **split = g_strsplit(input, " ", 0);

    if (g_strv_length(split) == 3) {
        char *field_tag = split[0]+1;
        if (form_tag_exists(form, field_tag)) {
            form_field_type_t field_type = form_get_field_type(form, field_tag);
            Autocomplete value_ac = form_get_value_ac(form, field_tag);;
            GString *beginning = g_string_new(split[0]);
            g_string_append(beginning, " ");
            g_string_append(beginning, split[1]);

            if (((g_strcmp0(split[1], "add") == 0) || (g_strcmp0(split[1], "remove") == 0))
                    && field_type == FIELD_LIST_MULTI) {
                found = autocomplete_param_with_ac(input, beginning->str, value_ac, TRUE);

            } else if ((g_strcmp0(split[1], "remove") == 0) && field_type == FIELD_TEXT_MULTI) {
                found = autocomplete_param_with_ac(input, beginning->str, value_ac, TRUE);

            } else if ((g_strcmp0(split[1], "remove") == 0) && field_type == FIELD_JID_MULTI) {
                found = autocomplete_param_with_ac(input, beginning->str, value_ac, TRUE);
            }

            g_string_free(beginning, TRUE);
        }

    } else if (g_strv_length(split) == 2) {
        char *field_tag = split[0]+1;
        if (form_tag_exists(form, field_tag)) {
            form_field_type_t field_type = form_get_field_type(form, field_tag);
            Autocomplete value_ac = form_get_value_ac(form, field_tag);;

            switch (field_type)
            {
                case FIELD_BOOLEAN:
                    found = autocomplete_param_with_func(input, split[0], prefs_autocomplete_boolean_choice);
                    break;
                case FIELD_LIST_SINGLE:
                    found = autocomplete_param_with_ac(input, split[0], value_ac, TRUE);
                    break;
                case FIELD_LIST_MULTI:
                case FIELD_JID_MULTI:
                case FIELD_TEXT_MULTI:
                    found = autocomplete_param_with_ac(input, split[0], form_field_multi_ac, TRUE);
                    break;
                default:
                    break;
            }
        }
    }

    g_strfreev(split);

    return found;
}

static char*
_occupants_autocomplete(ProfWin *window, const char *const input)
{
    char *found = NULL;

    found = autocomplete_param_with_ac(input, "/occupants default show", occupants_show_ac, TRUE);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/occupants default hide", occupants_show_ac, TRUE);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/occupants default", occupants_default_ac, TRUE);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/occupants show", occupants_show_ac, TRUE);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/occupants hide", occupants_show_ac, TRUE);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/occupants", occupants_ac, TRUE);
    if (found) {
        return found;
    }

    return NULL;
}

static char*
_time_autocomplete(ProfWin *window, const char *const input)
{
    char *found = NULL;

    found = autocomplete_param_with_ac(input, "/time statusbar", time_format_ac, TRUE);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/time lastactivity", time_format_ac, TRUE);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/time console", time_format_ac, TRUE);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/time chat", time_format_ac, TRUE);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/time muc", time_format_ac, TRUE);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/time mucconfig", time_format_ac, TRUE);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/time private", time_format_ac, TRUE);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/time xml", time_format_ac, TRUE);
    if (found) {
        return found;
    }

    found = autocomplete_param_with_ac(input, "/time", time_ac, TRUE);
    if (found) {
        return found;
    }

    return NULL;
}

static char*
_kick_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;

    if (window->type == WIN_MUC) {
        ProfMucWin *mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        Autocomplete nick_ac = muc_roster_ac(mucwin->roomjid);

        if (nick_ac) {
            result = autocomplete_param_with_ac(input, "/kick", nick_ac, TRUE);
            if (result) {
                return result;
            }
        }
    }

    return result;
}

static char*
_ban_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;

    if (window->type == WIN_MUC) {
        ProfMucWin *mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        Autocomplete jid_ac = muc_roster_jid_ac(mucwin->roomjid);

        if (jid_ac) {
            result = autocomplete_param_with_ac(input, "/ban", jid_ac, TRUE);
            if (result) {
                return result;
            }
        }
    }

    return result;
}

static char*
_affiliation_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;

    if (window->type == WIN_MUC) {
        ProfMucWin *mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        gboolean parse_result;
        Autocomplete jid_ac = muc_roster_jid_ac(mucwin->roomjid);

        gchar **args = parse_args(input, 3, 3, &parse_result);

        if ((strncmp(input, "/affiliation", 12) == 0) && (parse_result == TRUE)) {
            GString *beginning = g_string_new("/affiliation ");
            g_string_append(beginning, args[0]);
            g_string_append(beginning, " ");
            g_string_append(beginning, args[1]);

            result = autocomplete_param_with_ac(input, beginning->str, jid_ac, TRUE);
            g_string_free(beginning, TRUE);
            if (result) {
                g_strfreev(args);
                return result;
            }
        }

        g_strfreev(args);
    }

    result = autocomplete_param_with_ac(input, "/affiliation set", affiliation_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/affiliation list", affiliation_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/affiliation", privilege_cmd_ac, TRUE);
    if (result) {
        return result;
    }

    return result;
}

static char*
_role_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;

    if (window->type == WIN_MUC) {
        ProfMucWin *mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        gboolean parse_result;
        Autocomplete nick_ac = muc_roster_ac(mucwin->roomjid);

        gchar **args = parse_args(input, 3, 3, &parse_result);

        if ((strncmp(input, "/role", 5) == 0) && (parse_result == TRUE)) {
            GString *beginning = g_string_new("/role ");
            g_string_append(beginning, args[0]);
            g_string_append(beginning, " ");
            g_string_append(beginning, args[1]);

            result = autocomplete_param_with_ac(input, beginning->str, nick_ac, TRUE);
            g_string_free(beginning, TRUE);
            if (result) {
                g_strfreev(args);
                return result;
            }
        }

        g_strfreev(args);
    }

    result = autocomplete_param_with_ac(input, "/role set", role_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/role list", role_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/role", privilege_cmd_ac, TRUE);
    if (result) {
        return result;
    }

    return result;
}

static char*
_statuses_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;

    result = autocomplete_param_with_ac(input, "/statuses console", statuses_setting_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/statuses chat", statuses_setting_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/statuses muc", statuses_setting_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/statuses", statuses_ac, TRUE);
    if (result) {
        return result;
    }

    return NULL;
}

static char*
_wins_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;

    result = autocomplete_param_with_func(input, "/wins autotidy", prefs_autocomplete_boolean_choice);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/wins", wins_ac, TRUE);
    if (result) {
        return result;
    }

    return NULL;
}

static char*
_tls_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;

    result = autocomplete_param_with_func(input, "/tls revoke", tlscerts_complete);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_func(input, "/tls cert", tlscerts_complete);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/tls certpath", tls_certpath_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_func(input, "/tls show", prefs_autocomplete_boolean_choice);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/tls", tls_ac, TRUE);
    if (result) {
        return result;
    }

    return result;
}

static char*
_receipts_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;

    result = autocomplete_param_with_func(input, "/receipts send", prefs_autocomplete_boolean_choice);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_func(input, "/receipts request", prefs_autocomplete_boolean_choice);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/receipts", receipts_ac, TRUE);
    if (result) {
        return result;
    }

    return NULL;
}

static char*
_alias_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;

    result = autocomplete_param_with_ac(input, "/alias remove", aliases_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/alias", alias_ac, TRUE);
    if (result) {
        return result;
    }

    return NULL;
}

static char*
_connect_autocomplete(ProfWin *window, const char *const input)
{
    char *found = NULL;
    gboolean result = FALSE;

    gchar **args = parse_args(input, 2, 6, &result);

    if ((strncmp(input, "/connect", 8) == 0) && (result == TRUE)) {
        GString *beginning = g_string_new("/connect ");
        g_string_append(beginning, args[0]);
        if (args[1] && args[2]) {
            g_string_append(beginning, " ");
            g_string_append(beginning, args[1]);
            g_string_append(beginning, " ");
            g_string_append(beginning, args[2]);
            if (args[3] && args[4]) {
                g_string_append(beginning, " ");
                g_string_append(beginning, args[3]);
                g_string_append(beginning, " ");
                g_string_append(beginning, args[4]);
            }
        }
        found = autocomplete_param_with_ac(input, beginning->str, connect_property_ac, TRUE);
        g_string_free(beginning, TRUE);
        if (found) {
            g_strfreev(args);
            return found;
        }
    }

    g_strfreev(args);

    result = FALSE;
    args = parse_args(input, 2, 7, &result);

    if ((strncmp(input, "/connect", 8) == 0) && (result == TRUE)) {
        GString *beginning = g_string_new("/connect ");
        g_string_append(beginning, args[0]);
        int curr = 0;
        if (args[1]) {
            g_string_append(beginning, " ");
            g_string_append(beginning, args[1]);
            curr = 1;
            if (args[2] && args[3]) {
                g_string_append(beginning, " ");
                g_string_append(beginning, args[2]);
                g_string_append(beginning, " ");
                g_string_append(beginning, args[3]);
                curr = 3;
                if (args[4] && args[5]) {
                    g_string_append(beginning, " ");
                    g_string_append(beginning, args[4]);
                    g_string_append(beginning, " ");
                    g_string_append(beginning, args[5]);
                    curr = 5;
                }
            }
        }
        if (curr != 0 && (g_strcmp0(args[curr], "tls") == 0)) {
            found = autocomplete_param_with_ac(input, beginning->str, tls_property_ac, TRUE);
            g_string_free(beginning, TRUE);
            if (found) {
                g_strfreev(args);
                return found;
            }
        } else {
            g_string_free(beginning, TRUE);
        }
    }

    g_strfreev(args);

    found = autocomplete_param_with_func(input, "/connect", accounts_find_enabled);
    if (found) {
        return found;
    }

    return NULL;
}

static char*
_help_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;

    result = autocomplete_param_with_ac(input, "/help commands", help_commands_ac, TRUE);
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/help", help_ac, TRUE);
    if (result) {
        return result;
    }

    return NULL;
}

static char*
_join_autocomplete(ProfWin *window, const char *const input)
{
    char *found = NULL;
    gboolean result = FALSE;

    found = autocomplete_param_with_func(input, "/join", bookmark_find);
    if (found) {
        return found;
    }

    gchar **args = parse_args(input, 2, 4, &result);

    if ((strncmp(input, "/join", 5) == 0) && (result == TRUE)) {
        GString *beginning = g_string_new("/join ");
        g_string_append(beginning, args[0]);
        if (args[1] && args[2]) {
            g_string_append(beginning, " ");
            g_string_append(beginning, args[1]);
            g_string_append(beginning, " ");
            g_string_append(beginning, args[2]);
        }
        found = autocomplete_param_with_ac(input, beginning->str, join_property_ac, TRUE);
        g_string_free(beginning, TRUE);
        if (found) {
            g_strfreev(args);
            return found;
        }
    }

    g_strfreev(args);

    return NULL;
}

static char*
_subject_autocomplete(ProfWin *window, const char *const input)
{
    char *result = NULL;

    if (window->type == WIN_MUC) {
        if ((g_strcmp0(input, "/subject e") == 0)
                || (g_strcmp0(input, "/subject ed") == 0)
                || (g_strcmp0(input, "/subject edi") == 0)
                || (g_strcmp0(input, "/subject edit") == 0)
                || (g_strcmp0(input, "/subject edit ") == 0)
                || (g_strcmp0(input, "/subject edit \"") == 0)) {
            ProfMucWin *mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

            char *subject = muc_subject(mucwin->roomjid);
            if (subject) {
                GString *result_str = g_string_new("/subject edit \"");
                g_string_append(result_str, subject);
                g_string_append(result_str, "\"");

                result = result_str->str;
                g_string_free(result_str, FALSE);
            }
        }
    }
    if (result) {
        return result;
    }

    result = autocomplete_param_with_ac(input, "/subject", subject_ac, TRUE);
    if (result) {
        return result;
    }

    return NULL;
}

static char*
_account_autocomplete(ProfWin *window, const char *const input)
{
    char *found = NULL;
    gboolean result = FALSE;

    gchar **args = parse_args(input, 3, 4, &result);

    if ((strncmp(input, "/account set", 12) == 0) && (result == TRUE)) {
        GString *beginning = g_string_new("/account set ");
        g_string_append(beginning, args[1]);
        if ((g_strv_length(args) > 3) && (g_strcmp0(args[2], "otr")) == 0) {
            g_string_append(beginning, " ");
            g_string_append(beginning, args[2]);
            found = autocomplete_param_with_ac(input, beginning->str, otr_policy_ac, TRUE);
            g_string_free(beginning, TRUE);
            if (found) {
                g_strfreev(args);
                return found;
            }
        } else if ((g_strv_length(args) > 3) && (g_strcmp0(args[2], "status")) == 0) {
            g_string_append(beginning, " ");
            g_string_append(beginning, args[2]);
            found = autocomplete_param_with_ac(input, beginning->str, account_status_ac, TRUE);
            g_string_free(beginning, TRUE);
            if (found) {
                g_strfreev(args);
                return found;
            }
        } else if ((g_strv_length(args) > 3) && (g_strcmp0(args[2], "tls")) == 0) {
            g_string_append(beginning, " ");
            g_string_append(beginning, args[2]);
            found = autocomplete_param_with_ac(input, beginning->str, tls_property_ac, TRUE);
            g_string_free(beginning, TRUE);
            if (found) {
                g_strfreev(args);
                return found;
            }
        } else if ((g_strv_length(args) > 3) && (g_strcmp0(args[2], "startscript")) == 0) {
            g_string_append(beginning, " ");
            g_string_append(beginning, args[2]);
            found = autocomplete_param_with_func(input, beginning->str, _script_autocomplete_func);
            g_string_free(beginning, TRUE);
            if (found) {
                g_strfreev(args);
                return found;
            }
#ifdef HAVE_LIBGPGME
        } else if ((g_strv_length(args) > 3) && (g_strcmp0(args[2], "pgpkeyid")) == 0) {
            g_string_append(beginning, " ");
            g_string_append(beginning, args[2]);
            found = autocomplete_param_with_func(input, beginning->str, p_gpg_autocomplete_key);
            g_string_free(beginning, TRUE);
            if (found) {
                g_strfreev(args);
                return found;
            }
#endif
        } else {
            found = autocomplete_param_with_ac(input, beginning->str, account_set_ac, TRUE);
            g_string_free(beginning, TRUE);
            if (found) {
                g_strfreev(args);
                return found;
            }
        }
    }

    if ((strncmp(input, "/account clear", 14) == 0) && (result == TRUE)) {
        GString *beginning = g_string_new("/account clear ");
        g_string_append(beginning, args[1]);
        found = autocomplete_param_with_ac(input, beginning->str, account_clear_ac, TRUE);
        g_string_free(beginning, TRUE);
        if (found) {
            g_strfreev(args);
            return found;
        }
    }

    g_strfreev(args);

    found = autocomplete_param_with_ac(input, "/account default", account_default_ac, TRUE);
    if(found){
        return found;
    }

    int i = 0;
    gchar *account_choice[] = { "/account set", "/account show", "/account enable",
        "/account disable", "/account rename", "/account clear", "/account remove",
        "/account default set" };

    for (i = 0; i < ARRAY_SIZE(account_choice); i++) {
        found = autocomplete_param_with_func(input, account_choice[i], accounts_find_all);
        if (found) {
            return found;
        }
    }

    found = autocomplete_param_with_ac(input, "/account", account_ac, TRUE);
    return found;
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
