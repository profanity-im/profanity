/*
 * cmd_defs.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2025 Michael Vetter <jubalh@iodoru.org>
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

#define CMD_TAG_CHAT       "chat"
#define CMD_TAG_GROUPCHAT  "groupchat"
#define CMD_TAG_ROSTER     "roster"
#define CMD_TAG_PRESENCE   "presence"
#define CMD_TAG_CONNECTION "connection"
#define CMD_TAG_DISCOVERY  "discovery"
#define CMD_TAG_UI         "ui"
#define CMD_TAG_PLUGINS    "plugins"

#define CMD_PREAMBLE(c, p, min, max, set) .cmd = c, .parser = p, .min_args = min, .max_args = max, .setting_func = set,
#define CMD_MAINFUNC(f)                   .func = f,
#define CMD_SUBFUNCS(...)                 .sub_funcs = { __VA_ARGS__, { NULL, NULL } },
#define CMD_TAGS(...)                     .help.tags = { __VA_ARGS__, NULL },
#define CMD_SYN(...)                      .help.synopsis = { __VA_ARGS__, NULL },
#define CMD_DESC(d)                       .help.desc = d,
#define CMD_ARGS(...)                     .help.args = { __VA_ARGS__, { NULL, NULL } },
#define CMD_EXAMPLES(...)                 .help.examples = { __VA_ARGS__, NULL }

GHashTable* commands = NULL;

static gboolean _cmd_has_tag(Command* pcmd, const char* const tag);

/*
 * Command list
 */

// clang-format off
static const struct cmd_t command_defs[] = {
    { CMD_PREAMBLE("/help",
      parse_args_with_freetext, 0, 2, NULL)
      CMD_MAINFUNC(cmd_help)
      CMD_SYN(
              "/help [<area>|<command>|search_all|search_any] [<search_terms>]")
      CMD_DESC(
              "Help on using Profanity. Passing no arguments list help areas. "
              "For command help, optional arguments are shown using square brackets, "
              "arguments representing variables rather than a literal name are surrounded by angle brackets. "
              "Arguments that may be one of a number of values are separated by a pipe "
              "e.g. val1|val2|val3.")
      CMD_ARGS(
              { "<area>", "Summary help for commands in a certain area of functionality." },
              { "<command>", "Full help for a specific command, for example '/help connect'." },
              { "search_all <search_terms>", "Search commands for returning matches that contain all of the search terms." },
              { "search_any <search_terms>", "Search commands for returning matches that contain any of the search terms." })
      CMD_EXAMPLES(
              "/help search_all presence online",
              "/help commands",
              "/help presence",
              "/help who")
    },

    { CMD_PREAMBLE("/about",
                   parse_args, 0, 0, NULL)
      CMD_MAINFUNC(cmd_about)
      CMD_SYN(
              "/about")
      CMD_DESC(
              "Show version and license information.")
    },

    { CMD_PREAMBLE("/connect",
                   parse_args, 0, 7, NULL)
      CMD_MAINFUNC(cmd_connect)
      CMD_TAGS(
              CMD_TAG_CONNECTION)
      CMD_SYN(
              "/connect [<account>]",
              "/connect <account> [server <server>] [port <port>] [tls force|allow|trust|legacy|disable] [auth default|legacy]",
              "/connect <server>")
      CMD_DESC(
              "Login to a chat service. "
              "If no account is specified, the default is used if one is configured. "
              "A local account is created with the JID as it's name if it doesn't already exist. "
              "In case you want to connect to a server via SASL ANONYMOUS (c.f. XEP-0175) you can also do that.")
      CMD_ARGS(
              { "<account>", "The local account you wish to connect with, or a JID if connecting for the first time." },
              { "server <server>", "Supply a server if it is different to the domain part of your JID." },
              { "port <port>", "The port to use if different to the default (5222, or 5223 for SSL)." },
              { "<server>", "Connect to said server in an anonymous way. (Be aware: There aren't many servers that support this.)" },
              { "tls force", "Force TLS connection, and fail if one cannot be established, this is default behaviour." },
              { "tls allow", "Use TLS for the connection if it is available." },
              { "tls trust", "Force TLS connection and trust server's certificate." },
              { "tls legacy", "Use legacy TLS for the connection. It means server doesn't support STARTTLS and TLS is forced just after TCP connection is established." },
              { "tls disable", "Disable TLS for the connection." },
              { "auth default", "Default authentication process." },
              { "auth legacy", "Allow legacy authentication." })
      CMD_EXAMPLES(
              "/connect",
              "/connect odin@valhalla.edda",
              "/connect odin@valhalla.edda server talk.google.com",
              "/connect freyr@vanaheimr.edda port 5678",
              "/connect me@localhost.test.org server 127.0.0.1 tls disable",
              "/connect me@chatty server chatty.com port 5443",
              "/connect server.supporting.sasl.anonymous.example")
    },

    { CMD_PREAMBLE("/tls",
                   parse_args, 1, 3, NULL)
      CMD_SUBFUNCS(
              { "certpath", cmd_tls_certpath },
              { "trust", cmd_tls_trust },
              { "trusted", cmd_tls_trusted },
              { "revoke", cmd_tls_revoke },
              { "cert", cmd_tls_cert })
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
              "/tls certpath default")
      CMD_DESC(
              "Handle TLS certificates. ")
      CMD_ARGS(
              { "allow", "Allow connection to continue with TLS certificate." },
              { "always", "Always allow connections with TLS certificate." },
              { "deny", "Abort connection." },
              { "cert", "Show the current TLS certificate." },
              { "cert <fingerprint>", "Show details of trusted certificate." },
              { "trust", "Add the current TLS certificate to manually trusted certificates." },
              { "trusted", "List summary of manually trusted certificates (with '/tls always' or '/tls trust')." },
              { "revoke <fingerprint>", "Remove a manually trusted certificate." },
              { "certpath", "Show the trusted certificate path." },
              { "certpath set <path>", "Specify filesystem path containing trusted certificates." },
              { "certpath clear", "Clear the trusted certificate path." },
              { "certpath default", "Use default system certificate path, if it can be found." })
    },

    { CMD_PREAMBLE("/disconnect",
                   parse_args, 0, 0, NULL)
      CMD_MAINFUNC(cmd_disconnect)
      CMD_TAGS(
              CMD_TAG_CONNECTION)
      CMD_SYN(
              "/disconnect")
      CMD_DESC(
              "Disconnect from the current chat service.")
    },

    { CMD_PREAMBLE("/msg",
                   parse_args_with_freetext, 1, 2, NULL)
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
              { "<contact>", "Open chat window with contact, by JID or nickname." },
              { "<contact> [<message>]", "Send message to contact, by JID or nickname." },
              { "<nick>", "Open private chat window with chat room occupant." },
              { "<nick> [<message>]", "Send a private message to a chat room occupant." })
      CMD_EXAMPLES(
              "/msg thor@valhalla.edda Hey, here's a message!",
              "/msg heimdall@valhalla.edda",
              "/msg Thor Here is a private message",
              "/msg \"My Friend\" Hi, how are you?")
    },

    { CMD_PREAMBLE("/roster",
                   parse_args_with_freetext, 0, 4, NULL)
      CMD_SUBFUNCS(
              { "group", cmd_group })
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
              "/roster color on|off",
              "/roster order name|presence",
              "/roster unread before|after|off",
              "/roster room char <char>|none",
              "/roster room private char <char>|none",
              "/roster room position first|last",
              "/roster room by service|none",
              "/roster room order name|unread",
              "/roster room unread before|after|off",
              "/roster room title bookmark|jid|localpart|name",
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
              "/roster remove <contact>",
              "/roster remove_all contacts",
              "/roster nick <jid> <nick>",
              "/roster clearnick <jid>",
              "/roster group",
              "/roster group show <group>",
              "/roster group add <group> <contat>",
              "/roster group remove <group> <contact>")
      CMD_DESC(
              "Manage your roster, and roster display settings. "
              "Passing no arguments lists all contacts in your roster.")
      CMD_ARGS(
              { "online", "Show all online contacts in console." },
              { "show", "Show the roster panel." },
              { "show offline", "Show offline contacts in roster panel." },
              { "show resource", "Show contact's connected resources in roster panel." },
              { "show presence", "Show contact's presence in roster panel." },
              { "show status", "Show contact's status message in roster panel." },
              { "show empty", "Show empty groups in roster panel." },
              { "show priority", "Show resource priority in roster panel." },
              { "show contacts", "Show contacts in roster panel." },
              { "show rooms", "Show chat rooms in roster panel." },
              { "hide", "Hide the roster panel." },
              { "hide offline", "Hide offline contacts in roster panel." },
              { "hide resource", "Hide contact's connected resources in roster panel." },
              { "hide presence", "Hide contact's presence in roster panel." },
              { "hide status", "Hide contact's status message in roster panel." },
              { "hide empty", "Hide empty groups in roster panel." },
              { "hide priority", "Hide resource priority in roster panel." },
              { "hide contacts", "Hide contacts in roster panel." },
              { "hide rooms", "Hide chat rooms in roster panel." },
              { "by group", "Group contacts in roster panel by roster group." },
              { "by presence", "Group contacts in roster panel by presence." },
              { "by none", "No grouping in roster panel." },
              { "count unread", "Show unread message count with roster headers." },
              { "count items", "Show item count with roster headers." },
              { "count off", "Do not show any count with roster headers." },
              { "count zero on", "Show roster header count when 0." },
              { "count zero off", "Hide roster header count when 0." },
              { "color on", "Enable generated color names (XEP-0392)" },
              { "color off", "Disable generated color names (XEP-0392)" },
              { "order name", "Order roster contacts by name only." },
              { "order presence", "Order roster contacts by presence, and then by name." },
              { "unread before", "Show unread message count before contact." },
              { "unread after", "Show unread message count after contact." },
              { "unread off", "Do not show unread message count for contacts." },
              { "room char <char>", "Prefix rooms with specified character." },
              { "room char none", "Remove room character prefix." },
              { "room private char <char>", "Prefix private room chat with specified character when displayed with room." },
              { "room private char none", "Remove private room chat character prefix when displayed with room." },
              { "room position first", "Show rooms first in roster." },
              { "room position last", "Show rooms last in roster." },
              { "room by service", "Group rooms by chat service." },
              { "room by none", "Do not group rooms." },
              { "room order name", "Order rooms by name." },
              { "room order unread", "Order rooms by unread messages, and then by name." },
              { "room unread before", "Show unread message count before room." },
              { "room unread after", "Show unread message count after room." },
              { "room unread off", "Do not show unread message count for rooms." },
              { "room title bookmark|jid|localpart|name", "Display the bookmark name, JID, JID localpart, or room name as the roster title for MUCs." },
              { "private room", "Show room private chats with the room." },
              { "private group", "Show room private chats as a separate roster group." },
              { "private off", "Do not show room private chats." },
              { "private char <char>", "Prefix private room chats with specified character when displayed in separate group." },
              { "private char none", "Remove private room chat character prefix." },
              { "header char <char>", "Prefix roster headers with specified character." },
              { "header char none", "Remove roster header character prefix." },
              { "contact char <char>", "Prefix roster contacts with specified character." },
              { "contact char none", "Remove roster contact character prefix." },
              { "contact indent <indent>", "Indent contact line by <indent> spaces (0 to 10)." },
              { "resource char <char>", "Prefix roster resources with specified character." },
              { "resource char none", "Remove roster resource character prefix." },
              { "resource indent <indent>", "Indent resource line by <indent> spaces (0 to 10)." },
              { "resource join on|off", "Join resource with previous line when only one available resource." },
              { "presence indent <indent>", "Indent presence line by <indent> spaces (-1 to 10), a value of -1 will show presence on the previous line." },
              { "size <percent>", "Percentage of the screen taken up by the roster (1-99)." },
              { "wrap on|off", "Enable or disable line wrapping in roster panel." },
              { "add <jid> [<nick>]", "Add a new item to the roster." },
              { "remove <jid>", "Removes an item from the roster." },
              { "remove_all contacts", "Remove all items from roster." },
              { "nick <jid> <nick>", "Change a contacts nickname." },
              { "clearnick <jid>", "Removes the current nickname." },
              { "group show <group>", "List all roster items in a group." },
              { "group add <group> <contact>", "Add a contact to a group." },
              { "group remove <group> <contact>", "Remove a contact from a group." })
      CMD_EXAMPLES(
              "/roster",
              "/roster add odin@valhalla.edda",
              "/roster add odin@valhalla.edda Allfather",
              "/roster remove loki@ownserver.org",
              "/roster nick odin@valhalla.edda \"All Father\"",
              "/roster clearnick thor@valhalla.edda",
              "/roster size 15",
              "/roster group",
              "/roster group show friends",
              "/roster group add friends fenris@ownserver.org",
              "/roster group add family Brother",
              "/roster group remove colleagues boss@work.com")
    },

    { CMD_PREAMBLE("/blocked",
                   parse_args_with_freetext, 0, 3, NULL)
      CMD_MAINFUNC(cmd_blocked)
      CMD_TAGS(
              CMD_TAG_ROSTER,
              CMD_TAG_CHAT)
      CMD_SYN(
              "/blocked",
              "/blocked add [<jid>]",
              "/blocked report-abuse [<jid>] [<message>]",
              "/blocked report-spam [<jid>] [<message>]",
              "/blocked remove <jid>")
      CMD_DESC(
              "Manage blocked users (XEP-0191), calling with no arguments shows the current list of blocked users. "
              "To blog a certain user in a MUC use the following as jid: room@conference.example.org/spammy-user"
              "It is also possible to block and report (XEP-0377) a user with the report-abuse and report-spam commands.")
      CMD_ARGS(
              { "add [<jid>]", "Block the specified Jabber ID. If in a chat window and no jid is specified, the current recipient will be blocked." },
              { "remove <jid>", "Remove the specified Jabber ID from the blocked list." },
              { "report-abuse <jid> [<message>]", "Report the jid as abuse with an optional message to the service operator." },
              { "report-spam <jid> [<message>]", "Report the jid as spam with an optional message to the service operator." })
      CMD_EXAMPLES(
              "/blocked add hel@helheim.edda",
              "/blocked report-spam hel@helheim.edda Very annoying guy",
              "/blocked add profanity@rooms.dismail.de/spammy-user")
    },

    { CMD_PREAMBLE("/info",
                   parse_args, 0, 1, NULL)
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
              { "<nick>", "When in a chat room, the occupant you wish to view information about." })
      CMD_EXAMPLES(
              "/info thor@asgard.server.org",
              "/info heimdall")
    },

    { CMD_PREAMBLE("/caps",
                   parse_args, 0, 1, NULL)
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
              { "<nick>", "If in a chat room, nickname for which you wish to see capabilities." })
      CMD_EXAMPLES(
              "/caps ran@cold.sea.org/laptop",
              "/caps ran@cold.sea.org/phone",
              "/caps aegir")
    },

    { CMD_PREAMBLE("/software",
                   parse_args, 0, 1, NULL)
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
              { "<nick>", "If in a chat room, nickname for which you wish to see software information." })
      CMD_EXAMPLES(
              "/software odin@valhalla.edda/laptop",
              "/software odin@valhalla.edda/phone",
              "/software thor")
    },

    { CMD_PREAMBLE("/status",
                   parse_args, 2, 3, NULL)
      CMD_SUBFUNCS(
              { "get", cmd_status_get },
              { "set", cmd_status_set })
      CMD_TAGS(
              CMD_TAG_CHAT,
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/status set <state> [\"<message>\"]",
              "/status get <contact>|<nick>")
      CMD_DESC(
              "/status get: Find out a contact, or room members presence information. "
              "/status set: set own status.")
      CMD_ARGS(
              { "<state>", "Own status. Possible values: chat, online, away, dnd, xa" },
              { "<message>", "Optional message to use with the status. Needs quotation marks if it's more than one word." },
              { "<contact>", "The contact who's presence you which to see." },
              { "<nick>", "If in a chat room, the occupant who's presence you wish to see." })
      CMD_EXAMPLES(
              "/status get odin@valhalla.edda",
              "/status get jon",
              "/status set online")
    },

    { CMD_PREAMBLE("/resource",
                   parse_args, 1, 2, &cons_resource_setting)
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
              { "off", "Let the server choose which resource to route messages to." },
              { "title on|off", "Show or hide the current resource in the titlebar." },
              { "message on|off", "Show or hide the resource when showing an incoming message." })
    },

    { CMD_PREAMBLE("/join",
                   parse_args, 0, 5, NULL)
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
              "If the room doesn't exist, and the server allows it, a new one will be created. "
              "If you join to a room often, you might also want to add a bookmark (see `/help bookmark`), which also allows to set a default nickname. "
              "In this case, you should use `/bookmark join`.")
      CMD_ARGS(
              { "<room>", "The chat room to join." },
              { "nick <nick>", "Nickname to use in the room." },
              { "password <password>", "Password if the room requires one." })
      CMD_EXAMPLES(
              "/join",
              "/join profanity@rooms.dismail.de",
              "/join profanity@rooms.dismail.de nick mynick",
              "/join private@conference.jabber.org nick mynick password mypassword",
              "/join mychannel")
    },

    { CMD_PREAMBLE("/invite",
                   parse_args_with_freetext, 1, 3, NULL)
      CMD_MAINFUNC(cmd_invite)
      CMD_TAGS(
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/invite send <contact> [<message>]",
              "/invite list",
              "/invite decline")
      CMD_DESC(
              "Manage room invites. "
              "Send an invite to a contact for the current chat room. "
              "List received invites. "
              "Decline them using /invite decline and accept them using /join.")
      CMD_ARGS(
              { "send <contact> [<message>]", "The contact you wish to invite. And an optional message." },
              { "list", "Show all rooms that you have been invited to, and not accepted or declined." },
              { "decline <room>", "Decline a chat room invitation." })
      CMD_EXAMPLES(
              "/invite send gustavo@pollos.tx",
              "/invite decline profanity@rooms.dismail.de",
              "/invite list")
    },

    { CMD_PREAMBLE("/room",
                   parse_args, 1, 1, NULL)
      CMD_MAINFUNC(cmd_room)
      CMD_TAGS(
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/room accept|destroy|config")
      CMD_DESC(
              "Chat room configuration.")
      CMD_ARGS(
              { "accept", "Accept default room configuration." },
              { "destroy", "Reject default room configuration, and destroy the room." },
              { "config", "Edit room configuration." })
    },

    { CMD_PREAMBLE("/kick",
                   parse_args_with_freetext, 1, 2, NULL)
      CMD_MAINFUNC(cmd_kick)
      CMD_TAGS(
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/kick <nick> [<reason>]")
      CMD_DESC(
              "Kick occupant from chat room.")
      CMD_ARGS(
              { "<nick>", "Nickname of the occupant to kick from the room." },
              { "<reason>", "Optional reason for kicking the occupant." })
    },

    { CMD_PREAMBLE("/ban",
                   parse_args_with_freetext, 1, 2, NULL)
      CMD_MAINFUNC(cmd_ban)
      CMD_TAGS(
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/ban <jid> [<reason>]")
      CMD_DESC(
              "Ban user from chat room.")
      CMD_ARGS(
              { "<jid>", "Bare JID of the user to ban from the room." },
              { "<reason>", "Optional reason for banning the user." })
    },

    { CMD_PREAMBLE("/subject",
                   parse_args_with_freetext, 0, 2, NULL)
      CMD_MAINFUNC(cmd_subject)
      CMD_TAGS(
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/subject set <subject>",
              "/subject edit <subject>",
              "/subject editor",
              "/subject prepend <text>",
              "/subject append <text>",
              "/subject clear")
      CMD_DESC(
              "Set, modify, or clear room subject.")
      CMD_ARGS(
              { "set <subject>", "Set the room subject." },
              { "edit <subject>", "Edit the current room subject, tab autocompletion will display the subject to edit." },
              { "editor", "Edit the current room subject in external editor." },
              { "prepend <text>", "Prepend text to the current room subject, use double quotes if a trailing space is needed." },
              { "append <text>", "Append text to the current room subject, use double quotes if a preceding space is needed." },
              { "clear", "Clear the room subject." })
    },

    { CMD_PREAMBLE("/affiliation",
                   parse_args_with_freetext, 1, 4, NULL)
      CMD_MAINFUNC(cmd_affiliation)
      CMD_TAGS(
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/affiliation set <affiliation> <jid> [<reason>]",
              "/affiliation list [<affiliation>]",
              "/affiliation request",
              "/affiliation register")
      CMD_DESC(
              "Manage room affiliations. "
              "Affiliation may be one of owner, admin, member, outcast or none.")
      CMD_ARGS(
              { "set <affiliation> <jid> [<reason>]", "Set the affiliation of user with jid, with an optional reason." },
              { "list [<affiliation>]", "List all users with the specified affiliation, or all if none specified." },
              { "request", "Request voice."},
              { "register", "Register your nickname with the MUC."})
    },

    { CMD_PREAMBLE("/role",
                   parse_args_with_freetext, 1, 4, NULL)
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
              { "list [<role>]", "List all occupants with the specified role, or all if none specified." })
    },

    { CMD_PREAMBLE("/occupants",
                   parse_args, 1, 3, cons_occupants_setting)
      CMD_MAINFUNC(cmd_occupants)
      CMD_TAGS(
              CMD_TAG_GROUPCHAT,
              CMD_TAG_UI)
      CMD_SYN(
              "/occupants show|hide [jid|offline]",
              "/occupants char <char>|none",
              "/occupants color on|off",
              "/occupants default show|hide [jid|offline]",
              "/occupants size [<percent>]",
              "/occupants indent <indent>",
              "/occupants header char <char>|none",
              "/occupants wrap on|off")
      CMD_DESC(
              "Show or hide room occupants, and occupants panel display settings.")
      CMD_ARGS(
              { "show", "Show the occupants panel in current room." },
              { "char <char>", "Prefix occupants with specified character." },
              { "char none", "Remove occupants character prefix." },
              { "color on", "Enable generated color names (XEP-0392) for occupants" },
              { "color off", "Disable generated color names (XEP-0392) for occupants" },
              { "hide", "Hide the occupants panel in current room." },
              { "show jid", "Show jid in the occupants panel in current room." },
              { "hide jid", "Hide jid in the occupants panel in current room." },
              { "show offline", "Show offline occupants panel in current room." },
              { "hide offline", "Hide offline occupants panel in current room." },
              { "default show|hide", "Whether occupants are shown by default in new rooms." },
              { "default show|hide jid", "Whether occupants jids are shown by default in new rooms." },
              { "default show|hide offline", "Whether offline occupants are shown by default in new rooms." },
              { "size <percent>", "Percentage of the screen taken by the occupants list in rooms (1-99)." },
              { "indent <indent>", "Indent contact line by <indent> spaces (0 to 10)." },
              { "header char <char>", "Prefix occupants headers with specified character." },
              { "header char none", "Remove occupants header character prefix." },
              { "wrap on|off", "Enable or disable line wrapping in occupants panel." })
    },

    { CMD_PREAMBLE("/form",
                   parse_args, 1, 2, NULL)
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
              { "show", "Show the current form." },
              { "submit", "Submit the current form." },
              { "cancel", "Cancel changes to the current form." },
              { "help [<tag>]", "Display help for form, or a specific field." })
    },

    { CMD_PREAMBLE("/rooms",
                   parse_args, 0, 4, NULL)
      CMD_MAINFUNC(cmd_rooms)
      CMD_TAGS(
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/rooms",
              "/rooms filter <text>",
              "/rooms service <service>",
              "/rooms service <service> filter <text>",
              "/rooms cache on|off|clear")
      CMD_DESC(
              "List the chat rooms available at the specified conference service. "
              "If no argument is supplied, the account preference 'muc.service' is used, 'conference.<domain-part>' by default. "
              "The filter argument only shows rooms that contain the provided text, case insensitive.")
      CMD_ARGS(
              { "service <service>", "The conference service to query." },
              { "filter <text>", "The text to filter results by." },
              { "cache on|off", "Enable or disable caching of rooms list response, enabled by default." },
              { "cache clear", "Clear the rooms response cache if enabled." })
      CMD_EXAMPLES(
              "/rooms",
              "/rooms filter development",
              "/rooms service conference.jabber.org",
              "/rooms service conference.jabber.org filter \"News Room\"")
    },

    { CMD_PREAMBLE("/bookmark",
                   parse_args, 0, 8, NULL)
      CMD_SUBFUNCS(
              { "ignore", cmd_bookmark_ignore })
      CMD_MAINFUNC(cmd_bookmark)
      CMD_TAGS(
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/bookmark",
              "/bookmark list [<jid>]",
              "/bookmark add [<room>] [nick <nick>] [password <password>] [name <roomname>] [autojoin on|off]",
              "/bookmark update <room> [nick <nick>] [password <password>] [name <roomname>] [autojoin on|off]",
              "/bookmark remove [<room>]",
              "/bookmark join <room>",
              "/bookmark invites on|off",
              "/bookmark ignore",
              "/bookmark ignore add <jid>",
              "/bookmark ignore remove <jid>")
      CMD_DESC(
              "Manage bookmarks and join bookmarked rooms. "
              "If you are in a chat room and no arguments are supplied to `/bookmark add`, autojoin is set to \"on\". "
              "There is also an autojoin ignore list in case you want to autojoin in many clients but not on Profanity. ")
      CMD_ARGS(
              { "list [<jid>]", "List all bookmarks. Or the details of one." },
              { "add [<room>]", "Add a bookmark, passing no room will bookmark the current room, setting autojoin to \"on\"." },
              { "remove [<room>]", "Remove a bookmark, passing no room will remove the bookmark for the current room, if one exists." },
              { "update <room>", "Update the properties associated with a bookmark." },
              { "nick <nick>", "Nickname used when joining the chat room." },
              { "password <password>", "Password if required, may be stored in plaintext on your server." },
              { "name <roomname>", "Optional name for the bookmark. By default localpart of the JID will be used." },
              { "autojoin on|off", "Whether to join the room automatically on login." },
              { "join <room>", "Join room using the properties associated with the bookmark." },
              { "invites on|off", "Whether or not to bookmark accepted room invites, defaults to 'on'." },
              { "ignore add <barejid>", "Add a bookmark to the autojoin ignore list." },
              { "ignore remove <barejid>", "Remove a bookmark from the autojoin ignore list." })
      CMD_EXAMPLES(
              "/bookmark add room@example.com nick YOURNICK",
              "/bookmark join room@example.com",
              "/bookmark update room@example.com nick NEWNICK autojoin on",
              "/bookmark ignore room@example.com",
              "/bookmark list",
              "/bookmark list room@example.com",
              "/bookmark remove room@example.com")
    },

    { CMD_PREAMBLE("/disco",
                   parse_args, 1, 2, NULL)
      CMD_MAINFUNC(cmd_disco)
      CMD_TAGS(
              CMD_TAG_DISCOVERY)
      CMD_SYN(
              "/disco info [<jid>]",
              "/disco items [<jid>]")
      CMD_DESC(
              "Find out information about an entities supported services. "
              "Calling with no arguments will query the server you are currently connected to. "
              "This includes discovering contact addresses for XMPP services (XEP-0157).")
      CMD_ARGS(
              { "info [<jid>]", "List protocols and features supported by an entity." },
              { "items [<jid>]", "List items associated with an entity." })
      CMD_EXAMPLES(
              "/disco info",
              "/disco items myserver.org",
              "/disco items conference.jabber.org",
              "/disco info odin@valhalla.edda/laptop")
    },

    { CMD_PREAMBLE("/sendfile",
                   parse_args_with_freetext, 1, 1, NULL)
      CMD_MAINFUNC(cmd_sendfile)
      CMD_TAGS(
              CMD_TAG_CHAT,
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/sendfile <file>")
      CMD_DESC(
              "Send a file using XEP-0363 HTTP file transfer. "
              "If you are in an OMEMO session then the file will be encrypted (XEP-0454) as well.")
      CMD_ARGS(
              { "<file>", "Path to the file." })
      CMD_EXAMPLES(
              "/sendfile /etc/hosts",
              "/sendfile ~/images/sweet_cat.jpg")
    },

    { CMD_PREAMBLE("/lastactivity",
                   parse_args, 1, 2, NULL)
      CMD_MAINFUNC(cmd_lastactivity)
      CMD_TAGS(
              CMD_TAG_PRESENCE)
      CMD_SYN(
              "/lastactivity set on|off",
              "/lastactivity get [<jid>]")
      CMD_DESC(
              "Enable/disable sending last activity, and send last activity requests.")
      CMD_ARGS(
              { "on|off", "Enable or disable sending of last activity." },
              { "<jid>", "The JID of the entity to query. Omitting the JID will query your server for its uptime." })
      CMD_EXAMPLES(
              "/lastactivity get",
              "/lastactivity set off",
              "/lastactivity get freyja@asgaard.edda",
              "/lastactivity get freyja@asgaard.edda/laptop",
              "/lastactivity get someserver.com")
    },

    { CMD_PREAMBLE("/nick",
                   parse_args_with_freetext, 1, 1, NULL)
      CMD_MAINFUNC(cmd_nick)
      CMD_TAGS(
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/nick <nickname>")
      CMD_DESC(
              "Change your nickname in the current chat room.")
      CMD_ARGS(
              { "<nickname>", "Your new nickname." })
    },

    { CMD_PREAMBLE("/win",
                   parse_args, 1, 1, NULL)
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
              { "console", "Focus the Console window." },
              { "<num>", "Focus specified window number." },
              { "<barejid>", "Focus chat window with contact by JID if open." },
              { "<nick>", "Focus chat window with contact by nickname if open." },
              { "<roomjid>", "Focus chat room window with roomjid if open." },
              { "<roomoccupantjid>", "Focus private chat roomoccupantjid if open." },
              { "xmlconsole", "Focus the XML Console window if open." },
              { "<plugin>", "Focus the plugin window." })
      CMD_EXAMPLES(
              "/win console",
              "/win 4",
              "/win odin@valhalla.edda",
              "/win Eddie",
              "/win bigroom@conference.chat.org",
              "/win bigroom@conference.chat.org/thor",
              "/win wikipedia")
    },

    { CMD_PREAMBLE("/wins",
                   parse_args, 0, 3, NULL)
      CMD_SUBFUNCS(
              { "unread", cmd_wins_unread },
              { "attention", cmd_wins_attention },
              { "prune", cmd_wins_prune },
              { "swap", cmd_wins_swap })
      CMD_MAINFUNC(cmd_wins)
      CMD_TAGS(
              CMD_TAG_UI)
      CMD_SYN(
              "/wins",
              "/wins unread",
              "/wins attention",
              "/wins prune",
              "/wins swap <source> <target>")
      CMD_DESC(
              "Manage windows. "
              "Passing no argument will list all currently active windows and information about their usage.")
      CMD_ARGS(
              { "unread", "List windows with unread messages." },
              { "attention", "List windows that have been marked with the attention flag (alt+v). You can toggle between marked windows with alt+m." },
              { "prune", "Close all windows with no unread messages." },
              { "swap <source> <target>", "Swap windows, target may be an empty position." })
    },

    { CMD_PREAMBLE("/sub",
                   parse_args, 1, 2, NULL)
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
              { "allow [<jid>]", "Approve a contact's subscription request." },
              { "deny [<jid>]", "Remove subscription for a contact, or deny a request." },
              { "show [<jid>]", "Show subscription status for a contact." },
              { "sent", "Show all sent subscription requests pending a response." },
              { "received", "Show all received subscription requests awaiting your response." })
      CMD_EXAMPLES(
              "/sub request odin@valhalla.edda",
              "/sub allow odin@valhalla.edda",
              "/sub request",
              "/sub sent")
    },

    { CMD_PREAMBLE("/who",
                   parse_args, 0, 2, NULL)
      CMD_MAINFUNC(cmd_who)
      CMD_TAGS(
              CMD_TAG_CHAT,
              CMD_TAG_GROUPCHAT,
              CMD_TAG_ROSTER)
      CMD_SYN(
              "/who",
              "/who online|offline|away|dnd|xa|chat|available|unavailable|any [<group>]",
              "/who moderator|participant|visitor",
              "/who owner|admin|member|none")
      CMD_DESC(
              "Show contacts or room occupants with chosen status, role or affiliation.")
      CMD_ARGS(
              { "offline|away|dnd|xa|chat", "Show contacts or room occupants with specified presence." },
              { "online", "Contacts that are online, chat, away, xa, dnd." },
              { "available", "Contacts that are available for chat - online, chat." },
              { "unavailable", "Contacts that are not available for chat - offline, away, xa, dnd." },
              { "any", "Contacts with any status (same as calling with no argument)." },
              { "<group>", "Filter the results by the specified roster group, not applicable in chat rooms." },
              { "moderator|participant|visitor", "Room occupants with the specified role." },
              { "owner|admin|member|none", "Room occupants with the specified affiliation." })
      CMD_EXAMPLES(
              "/who",
              "/who xa",
              "/who online friends",
              "/who any family",
              "/who participant",
              "/who admin")
    },

    { CMD_PREAMBLE("/close",
                   parse_args, 0, 1, NULL)
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
              { "<num>", "Close specified window number." },
              { "<barejid>", "Close chat window with contact by JID if open." },
              { "<nick>", "Close chat window with contact by nickname if open." },
              { "<roomjid>", "Close chat room window with roomjid if open." },
              { "<roomoccupantjid>", "Close private chat roomoccupantjid if open." },
              { "xmlconsole", "Close the XML Console window if open." },
              { "all", "Close all windows." },
              { "read", "Close all windows that have no unread messages." })
    },

    { CMD_PREAMBLE("/clear",
                   parse_args, 0, 2, NULL)
      CMD_MAINFUNC(cmd_clear)
      CMD_TAGS(
              CMD_TAG_UI)
      CMD_SYN(
              "/clear",
              "/clear persist_history <on|off>")
      CMD_DESC(
              "Clear the current window. "
              "If you set persist_history you can still access the history by pressing PAGE UP.")
      CMD_ARGS(
              { "persist_history on|off", "Whether or not to clear the screen persistently." })
      CMD_EXAMPLES(
              "/clear",
              "/clear persist_history",
              "/clear persist_history on")
    },

    { CMD_PREAMBLE("/quit",
                   parse_args, 0, 0, NULL)
       CMD_MAINFUNC(cmd_quit)
        CMD_SYN(
               "/quit")
       CMD_DESC(
               "Logout of any current session, and quit Profanity.")
    },

    { CMD_PREAMBLE("/privileges",
                   parse_args, 1, 1, &cons_privileges_setting)
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
    },

    { CMD_PREAMBLE("/charset",
                   parse_args, 0, 0, NULL)
      CMD_MAINFUNC(cmd_charset)
      CMD_TAGS(
              CMD_TAG_UI)
      CMD_SYN(
              "/charset")
      CMD_DESC(
              "Display information about the current character set supported by the terminal. ")
    },

    { CMD_PREAMBLE("/beep",
                   parse_args, 1, 1, &cons_beep_setting)
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
    },

    { CMD_PREAMBLE("/console",
                   parse_args, 2, 2, &cons_console_setting)
      CMD_MAINFUNC(cmd_console)
      CMD_TAGS(
              CMD_TAG_UI,
              CMD_TAG_CHAT,
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/console chat all|first|none",
              "/console muc all|first|mention|none",
              "/console private all|first|none")
      CMD_DESC(
              "Configure what is displayed in the console window when messages are received. "
              "The default is set to 'all' for all types of messages.")
      CMD_ARGS(
              { "chat all", "Indicate all new chat messages in the console." },
              { "chat first", "Indicate only the first new message per chat in the console." },
              { "chat none", "Do not show any new chat messages in the console window." },
              { "muc all", "Indicate all new chat room messages in the console." },
              { "muc first", "Indicate only the first new message in each room in the console." },
              { "muc mention", "Indicate only messages in which you have been mentioned in the console." },
              { "muc none", "Do not show any new chat room messages in the console window." },
              { "private all", "Indicate all new private room messages in the console." },
              { "private first", "Indicate only the first private room message in the console." },
              { "private none", "Do not show any new private room messages in the console window." })
    },

    { CMD_PREAMBLE("/presence",
                   parse_args, 2, 2, &cons_presence_setting)
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
              { "console all", "Show all presence changes in the console window." },
              { "console online", "Show only online/offline presence changes in the console window." },
              { "console none", "Don't show any presence changes in the console window." },
              { "chat all", "Show all presence changes in the chat windows." },
              { "chat online", "Show only online/offline presence changes in chat windows." },
              { "chat none", "Don't show any presence changes in chat windows." },
              { "room all", "Show all presence changes in chat room windows." },
              { "room online", "Show only online/offline presence changes in chat room windows." },
              { "room none", "Don't show any presence changes in chat room windows." })
      CMD_EXAMPLES(
              "/presence titlebar off",
              "/presence console none",
              "/presence chat online",
              "/presence room all")
    },

    { CMD_PREAMBLE("/wrap",
                   parse_args, 1, 1, &cons_wrap_setting)
      CMD_MAINFUNC(cmd_wrap)
      CMD_TAGS(
              CMD_TAG_UI)
      CMD_SYN(
              "/wrap on|off")
      CMD_DESC(
              "Word wrapping.")
      CMD_ARGS(
              { "on|off", "Enable or disable word wrapping in the main window." })
    },

    { CMD_PREAMBLE("/time",
                   parse_args, 1, 3, &cons_time_setting)
      CMD_MAINFUNC(cmd_time)
      CMD_TAGS(
              CMD_TAG_UI)
      CMD_SYN(
              "/time all|console|chat|muc|config|private|xml set <format>",
              "/time all|console|chat|muc|config|private|xml off",
              "/time statusbar set <format>",
              "/time statusbar off",
              "/time lastactivity set <format>",
              "/time vcard set <format>")
      CMD_DESC(
              "Configure time display preferences. "
              "Time formats are strings supported by g_date_time_format. "
              "See https://developer.gnome.org/glib/stable/glib-GDateTime.html#g-date-time-format for more details. "
              "Setting the format to an unsupported string, will display the string. "
              "If the format contains spaces, it must be surrounded with double quotes. "
              "It is possible to pass format as 'iso8601' in order to set the time format according to ISO-8601 (only local time, without Time zone designator).")
      CMD_ARGS(
              { "console set <format>", "Set time format for console window." },
              { "console off", "Do not show time in console window." },
              { "chat set <format>", "Set time format for chat windows." },
              { "chat off", "Do not show time in chat windows." },
              { "muc set <format>", "Set time format for chat room windows." },
              { "muc off", "Do not show time in chat room windows." },
              { "config set <format>", "Set time format for config windows." },
              { "config off", "Do not show time in config windows." },
              { "private set <format>", "Set time format for private chat windows." },
              { "private off", "Do not show time in private chat windows." },
              { "xml set <format>", "Set time format for XML console window." },
              { "xml off", "Do not show time in XML console window." },
              { "statusbar set <format>", "Change time format in statusbar." },
              { "statusbar off", "Do not show time in status bar." },
              { "lastactivity set <format>", "Change time format for last activity." },
              { "vcard set <format>", "Change the time format used to display time/dates in vCard (such as birthdays)" },
              { "all set <format>", "Set time for: console, chat, muc, config, private, and xml windows." },
              { "all off", "Do not show time for: console, chat, muc, config, private and xml windows." })
      CMD_EXAMPLES(
              "/time console set %H:%M:%S",
              "/time chat set \"%d-%m-%y %H:%M:%S\"",
              "/time xml off",
              "/time statusbar set %H:%M",
              "/time lastactivity set \"%d-%m-%y %H:%M:%S\"",
              "/time all set \"%d-%m-%y %H:%M:%S\"",
              "/time all set iso8601")
    },

    { CMD_PREAMBLE("/inpblock",
                   parse_args, 2, 2, &cons_inpblock_setting)
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
    },


    { CMD_PREAMBLE("/titlebar",
                   parse_args, 1, 3, &cons_titlebar_setting)
      CMD_SUBFUNCS(
              { "show", cmd_titlebar_show_hide },
              { "hide", cmd_titlebar_show_hide })
      CMD_MAINFUNC(cmd_titlebar)
      CMD_TAGS(
              CMD_TAG_UI)
      CMD_SYN(
              "/titlebar up",
              "/titlebar down",
              "/titlebar show|hide encwarn|resource|tls",
              "/titlebar room title bookmark|jid|localpart|name")
      CMD_DESC(
              "Titlebar settings.")
      CMD_ARGS(
              { "up", "Move the title bar up the screen." },
              { "down", "Move the title bar down the screen." },
              { "show tls", "Show or hide TLS indicator in the titlebar." },
              { "show encwarn", "Enable or disable the unencrypted warning message in the titlebar." },
              { "show resource", "Show or hide the current resource in the titlebar." },
              { "room title bookmark|jid|localpart|name", "Display the bookmark name, JID, JID localpart, or room name as the MUC window title." })
      CMD_EXAMPLES(
              "/titlebar up",
              "/titlebar show tls",
              "/titlebar hide encwarn",
              "/titlebar room title localpart")
    },

    { CMD_PREAMBLE("/mainwin",
                   parse_args, 1, 1, &cons_winpos_setting)
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
    },

    { CMD_PREAMBLE("/statusbar",
                   parse_args, 1, 3, &cons_statusbar_setting)
      CMD_MAINFUNC(cmd_statusbar)
      CMD_TAGS(
              CMD_TAG_UI)
      CMD_SYN(
              "/statusbar show name|number|read",
              "/statusbar hide name|number|read",
              "/statusbar maxtabs <value>",
              "/statusbar tablen <value>",
              "/statusbar tabmode default|dynamic|actlist",
              "/statusbar self user|barejid|fulljid|off",
              "/statusbar chat user|jid",
              "/statusbar room title bookmark|jid|localpart|name",
              "/statusbar up",
              "/statusbar down")
      CMD_DESC(
              "Manage statusbar display preferences.")
      CMD_ARGS(
              { "maxtabs <value>", "Set the maximum number of tabs to display, <value> must be between 0 and 10." },
              { "tablen <value>", "Set the maximum number of characters to show as the tab name, 0 sets to unlimited." },
              { "tabmode default|dynamic|actlist", "Set the mode tabs are shown. `dynamic` is a mode that displays tabs conveniently around current tab, thus providing proper pagination. `actlist` setting shows only active tabs. `default` setting always shows tabs in 1 to max_tabs range." },
              { "show|hide name", "Show or hide names in tabs." },
              { "show|hide number", "Show or hide numbers in tabs." },
              { "show|hide read", "Show or hide inactive tabs." },
              { "self user|barejid|fulljid", "Show account user name, barejid, fulljid as status bar title." },
              { "self off", "Disable showing self as status bar title." },
              { "chat user|jid", "Show users name, or fulljid. Change needs a redraw/restart to take effect." },
              { "room title bookmark|jid|localpart|name", "Display the bookmark name, JID, JID localpart, or room name as the title for MUC tabs." },
              { "up", "Move the status bar up the screen." },
              { "down", "Move the status bar down the screen." })
      CMD_EXAMPLES(
              "/statusbar maxtabs 8",
              "/statusbar tablen 5",
              "/statusbar tabmode actlist",
              "/statusbar self user",
              "/statusbar chat jid",
              "/statusbar hide read",
              "/statusbar hide name")
    },

    { CMD_PREAMBLE("/inputwin",
                   parse_args, 1, 1, &cons_winpos_setting)
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
    },

    { CMD_PREAMBLE("/notify",
                   parse_args_with_freetext, 0, 4, NULL)
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
              "/notify room offline on|off",
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
              { "chat on|off", "Notifications for regular chat messages." },
              { "chat current on|off", "Whether to show regular chat message notifications when the window is focused." },
              { "chat text on|off", "Show message text in regular message notifications." },
              { "room on|off", "Notifications for all chat room messages." },
              { "room mention on|off", "Notifications for chat room messages when your nick is mentioned." },
              { "room mention case_sensitive", "Set room mention notifications as case sensitive." },
              { "room mention case_insensitive", "Set room mention notifications as case insensitive." },
              { "room mention word_whole", "Set room mention notifications only on whole word match, i.e. when nickname is not part of a larger word." },
              { "room mention word_part", "Set room mention notifications on partial word match, i.e. nickname may be part of a larger word." },
              { "room offline on|off", "Notifications for chat room messages that were sent while you were offline." },
              { "room current on|off", "Whether to show all chat room messages notifications when the window is focused." },
              { "room text on|off", "Show message text in chat room message notifications." },
              { "room trigger add <text>", "Notify when specified text included in all chat room messages." },
              { "room trigger remove <text>", "Remove chat room notification trigger." },
              { "room trigger list", "List all chat room highlight triggers." },
              { "room trigger on|off", "Enable or disable all chat room notification triggers." },
              { "on|off", "Override the global message setting for the current chat room." },
              { "mention on|off", "Override the global 'mention' setting for the current chat room." },
              { "trigger on|off", "Override the global 'trigger' setting for the current chat room." },
              { "reset", "Reset to global notification settings for the current chat room." },
              { "remind <seconds>", "Notification reminder period for unread messages, use 0 to disable." },
              { "typing on|off", "Notifications when contacts are typing." },
              { "typing current on|off", "Whether typing notifications are triggered for the current window." },
              { "invite on|off", "Notifications for chat room invites." },
              { "sub on|off", "Notifications for subscription requests." })
      CMD_EXAMPLES(
              "/notify chat on",
              "/notify chat text on",
              "/notify room mention on",
              "/notify room offline on",
              "/notify room trigger add beer",
              "/notify room trigger on",
              "/notify room current off",
              "/notify room text off",
              "/notify remind 60",
              "/notify typing on",
              "/notify invite on")
    },

    { CMD_PREAMBLE("/flash",
                   parse_args, 1, 1, &cons_flash_setting)
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
    },

    { CMD_PREAMBLE("/tray",
                   parse_args, 1, 2, &cons_tray_setting)
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
              { "on|off", "Show tray icon." },
              { "read on|off", "Show tray icon when no unread messages." },
              { "timer <seconds>", "Set tray icon timer, seconds must be between 1-10." })
    },

    { CMD_PREAMBLE("/intype",
                   parse_args, 2, 2, &cons_intype_setting)
      CMD_MAINFUNC(cmd_intype)
      CMD_TAGS(
              CMD_TAG_UI,
              CMD_TAG_CHAT)
      CMD_SYN(
              "/intype console|titlebar on|off")
      CMD_DESC(
              "Show when a contact is typing in the console, and in active message window.")
      CMD_ARGS(
              { "titlebar on|off", "Enable or disable contact typing messages notification in titlebar." },
              { "console on|off", "Enable or disable contact typing messages notification in console window." })
    },

    { CMD_PREAMBLE("/splash",
                   parse_args, 1, 1, &cons_splash_setting)
      CMD_MAINFUNC(cmd_splash)
      CMD_TAGS(
              CMD_TAG_UI)
      CMD_SYN(
              "/splash on|off")
      CMD_DESC(
              "Switch on or off the ascii logo on start up and when the /about command is called.")
      CMD_ARGS(
              { "on|off", "Enable or disable splash logo." })
    },

    { CMD_PREAMBLE("/autoconnect",
                   parse_args, 1, 2, &cons_autoconnect_setting)
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
              { "off", "Disable autoconnect." })
      CMD_EXAMPLES(
              "/autoconnect set ulfhednar@valhalla.edda",
              "/autoconnect off")
    },

    { CMD_PREAMBLE("/vcard",
                   parse_args, 0, 7, NULL)
      CMD_SUBFUNCS(
              {"add", cmd_vcard_add},
              {"remove", cmd_vcard_remove},
              {"get", cmd_vcard_get},
              {"set", cmd_vcard_set},
              {"photo", cmd_vcard_photo},
              {"refresh", cmd_vcard_refresh},
              {"save", cmd_vcard_save})
      CMD_MAINFUNC(cmd_vcard)
      CMD_TAGS(
              CMD_TAG_CHAT,
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/vcard get [<nick|contact>]",
              "/vcard photo open <nick|contact> [<index>]",
              "/vcard photo save <nick|contact> [output <filepath>] [index <index>]",
              "/vcard set fullname <fullname>",
              "/vcard set name family <family>",
              "/vcard set name given <given>",
              "/vcard set name middle <middle>",
              "/vcard set name prefix <prefix>",
              "/vcard set name suffix <suffix>",
              "/vcard set <index> [<value>]",
              "/vcard set <index> pobox <value>",
              "/vcard set <index> extaddr <value>",
              "/vcard set <index> street <value>",
              "/vcard set <index> locality <value>",
              "/vcard set <index> region <value>",
              "/vcard set <index> pocode <value>",
              "/vcard set <index> country <value>",
              "/vcard set <index> type domestic|international",
              "/vcard set <index> home on|off",
              "/vcard set <index> work on|off",
              "/vcard set <index> voice on|off",
              "/vcard set <index> fax on|off",
              "/vcard set <index> pager on|off",
              "/vcard set <index> msg on|off",
              "/vcard set <index> cell on|off",
              "/vcard set <index> video on|off",
              "/vcard set <index> bbs on|off",
              "/vcard set <index> modem on|off",
              "/vcard set <index> isdn on|off",
              "/vcard set <index> pcs on|off",
              "/vcard set <index> preferred on|off",
              "/vcard set <index> parcel on|off",
              "/vcard set <index> postal on|off",
              "/vcard set <index> internet on|off",
              "/vcard set <index> x400 on|off",
              "/vcard add nickname <nickname>",
              "/vcard add birthday <date>",
              "/vcard add address",
              "/vcard add tel <number>",
              "/vcard add email <userid>",
              "/vcard add jid <jid>",
              "/vcard add title <title>",
              "/vcard add role <role>",
              "/vcard add note <note>",
              "/vcard add url <url>",
              "/vcard remove <index>",
              "/vcard refresh",
              "/vcard save")
      CMD_DESC(
              "Read your vCard or a user's vCard, get a user's avatar via their vCard, or modify your vCard. If no arguments are given, your vCard will be displayed in a new window, or an existing vCard window.")
      CMD_ARGS(
              { "get [<nick|contact>]", "Get your vCard, if a nickname/contact is provided, get that user's vCard" },
              { "photo open <nick|contact> [<index>]", "Download a user's photo from their vCard to a file, and open it. If index is not specified, download the first photo (usually avatar) from their vCard" },
              { "photo save <nick|contact>", "Download a user's photo from their vCard to a file. If index is not specified, download the first photo (usually avatar) from their vCard. If output is not specified, download the photo to profanity's photos directory." },
              { "photo open-self [<index>]", "Download a photo from your vCard to a file, and open it. If index is not specified, download the first photo (usually avatar) from your vCard" },
              { "photo save-self", "Download a photo from your vCard to a file. If index is not specified, download the first photo (usually avatar) from your vCard. If output is not specified, download the photo to profanity's photos directory. Same arguments as `photo open`" },
              { "set fullname <fullname>", "Set your vCard's fullname to the specified value" },
              { "set name family <family>", "Set your vCard's family name to the specified value" },
              { "set name given <given>", "Set your vCard's given name to the specified value" },
              { "set name middle <middle>", "Set your vCard's middle name to the specified value" },
              { "set name prefix <prefix>", "Set your vCard's prefix name to the specified value" },
              { "set name suffix <suffix>", "Set your vCard's suffix name to the specified value" },
              { "set <index> [<value>]", "Set the main field in a element in your vCard to the specified value, or if no value was specified, modify the field in an editor, This only works in elements that have one field." },
              { "set <index> pobox <value>", "Set the P.O. box in an address element in your vCard to the specified value." },
              { "set <index> extaddr <value>", "Set the extended address in an address element in your vCard to the specified value." },
              { "set <index> street <value>", "Set the street in an address element in your vCard to the specified value." },
              { "set <index> locality <value>", "Set the locality in an address element in your vCard to the specified value." },
              { "set <index> region <value>", "Set the region in an address element in your vCard to the specified value." },
              { "set <index> pocode <value>", "Set the P.O. code in an address element in your vCard to the specified value." },
              { "set <index> type domestic|international", "Set the type in an address element in your vCard to either domestic or international." },
              { "set <index> home on|off", "Set the home option in an element in your vCard. (address, telephone, e-mail only)" },
              { "set <index> work on|off", "Set the work option in an element in your vCard. (address, telephone, e-mail only)" },
              { "set <index> voice on|off", "Set the voice option in a telephone element in your vCard." },
              { "set <index> fax on|off", "Set the fax option in a telephone element in your vCard." },
              { "set <index> pager on|off", "Set the pager option in a telephone element in your vCard." },
              { "set <index> msg on|off", "Set the message option in a telephone element in your vCard." },
              { "set <index> cell on|off", "Set the cellphone option in a telephone element in your vCard." },
              { "set <index> video on|off", "Set the video option in a telephone element in your vCard." },
              { "set <index> bbs on|off", "Set the BBS option in a telephone element in your vCard." },
              { "set <index> modem on|off", "Set the modem option in a telephone element in your vCard." },
              { "set <index> isdn on|off", "Set the ISDN option in a telephone element in your vCard." },
              { "set <index> pcs on|off", "Set the PCS option in a telephone element in your vCard." },
              { "set <index> preferred on|off", "Set the preferred option in an element in your vCard. (address, telephone, e-mail only)" },
              { "set <index> parcel on|off", "Set the parcel option in an address element in your vCard." },
              { "set <index> postal on|off", "Set the postal option in an address element in your vCard." },
              { "set <index> internet on|off", "Set the internet option in an e-mail address in your vCard." },
              { "set <index> x400 on|off", "Set the X400 option in an e-mail address in your vCard." },
              { "add nickname <nickname>", "Add a nickname to your vCard" },
              { "add birthday <date>", "Add a birthday date to your vCard" },
              { "add address", "Add an address to your vCard" },
              { "add tel <number>", "Add a telephone number to your vCard" },
              { "add email <userid>", "Add an e-mail address to your vCard" },
              { "add jid <jid>", "Add a Jabber ID to your vCard" },
              { "add title <title>", "Add a title to your vCard" },
              { "add role <role>", "Add a role to your vCard" },
              { "add note <note>", "Add a note to your vCard" },
              { "add url <url>", "Add a URL to your vCard" },
              { "remove <index>", "Remove a element in your vCard by index" },
              { "refresh", "Refreshes the local copy of the current account's vCard (undoes all your unpublished modifications)" },
              { "save", "Save changes to the server" })
    },

    { CMD_PREAMBLE("/vercheck",
                   parse_args, 0, 1, NULL)
      CMD_MAINFUNC(cmd_vercheck)
      CMD_TAGS(
              CMD_TAG_UI)
      CMD_SYN(
              "/vercheck on|off")
      CMD_DESC(
              "Check for new versions when Profanity starts, and when the /about command is run.")
      CMD_ARGS(
              { "on|off", "Enable or disable the version check." })
    },

    { CMD_PREAMBLE("/wintitle",
                   parse_args, 2, 2, &cons_wintitle_setting)
      CMD_MAINFUNC(cmd_wintitle)
      CMD_TAGS(
              CMD_TAG_UI)
      CMD_SYN(
              "/wintitle show on|off",
              "/wintitle goodbye on|off")
      CMD_DESC(
              "Allow Profanity to modify the window title bar.")
      CMD_ARGS(
              { "show on|off", "Show current logged in user, and unread messages as the window title." },
              { "goodbye on|off", "Show a message in the title when exiting profanity." })
    },

    { CMD_PREAMBLE("/alias",
                   parse_args_with_freetext, 1, 3, NULL)
      CMD_MAINFUNC(cmd_alias)
      CMD_SYN(
              "/alias list",
              "/alias add <name> <value>",
              "/alias remove <name>")
      CMD_DESC(
              "Add, remove or list command aliases.")
      CMD_ARGS(
              { "list", "List all aliases." },
              { "add <name> <value>", "Add a new command alias. The alias name must not contain any space characters." },
              { "remove <name>", "Remove a command alias." })
      CMD_EXAMPLES(
              "/alias add friends /who online friends",
              "/alias add /q /quit",
              "/alias add urg /msg odin@valhalla.edda [URGENT]",
              "/alias add afk /status set away \"Away From Keyboard\"",
              "/alias remove q",
              "/alias list")
    },

    { CMD_PREAMBLE("/logging",
                   parse_args, 2, 3, &cons_logging_setting)
      CMD_MAINFUNC(cmd_logging)
      CMD_TAGS(
              CMD_TAG_CHAT)
      CMD_SYN(
              "/logging chat|group on|off")
      CMD_DESC(
              "Configure chat logging. "
              "Switch logging on or off. "
              "Chat logging will be enabled if /history is set to on. "
              "When disabling this option, /history will also be disabled. ")
      CMD_ARGS(
              { "chat on|off", "Enable/Disable regular chat logging." },
              { "group on|off", "Enable/Disable groupchat (room) logging." })
      CMD_EXAMPLES(
              "/logging chat on",
              "/logging group off")
    },

    { CMD_PREAMBLE("/states",
                   parse_args, 1, 1, &cons_states_setting)
      CMD_MAINFUNC(cmd_states)
      CMD_TAGS(
              CMD_TAG_CHAT)
      CMD_SYN(
              "/states on|off")
      CMD_DESC(
              "Send chat state notifications to recipient during chat sessions, such as typing, paused, active, gone.")
      CMD_ARGS(
              { "on|off", "Enable or disable sending of chat state notifications." })
    },

    { CMD_PREAMBLE("/pgp",
                   parse_args, 1, 3, NULL)
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
              "/pgp char <char>",
              "/pgp sendfile on|off",
              "/pgp sendpub [<contact>]")
      CMD_DESC(
              "Open PGP commands to manage keys, and perform PGP encryption during chat sessions. "
              "See the /account command to set your own PGP key.")
      CMD_ARGS(
              { "libver", "Show which version of the libgpgme library is being used." },
              { "keys", "List all keys known to the system." },
              { "contacts", "Show contacts with assigned public keys." },
              { "setkey <contact> <keyid>", "Manually associate a contact with a public key." },
              { "start [<contact>]", "Start PGP encrypted chat, current contact will be used if not specified." },
              { "end", "End PGP encrypted chat with the current recipient." },
              { "log on|off", "Enable or disable plaintext logging of PGP encrypted messages." },
              { "log redact", "Log PGP encrypted messages, but replace the contents with [redacted]. This is the default." },
              { "char <char>", "Set the character to be displayed next to PGP encrypted messages." },
              { "sendfile on|off", "Allow /sendfile to send unencrypted files while otherwise using PGP." },
              { "autoimport on|off", "Autoimport PGP keys from messages." },
              { "sendpub [<contact>]", "Sends a message to the current recipient with your PGP public key, current contact will be used if not specified." })
      CMD_EXAMPLES(
              "/pgp log off",
              "/pgp setkey odin@valhalla.edda BA19CACE5A9592C5",
              "/pgp start odin@valhalla.edda",
              "/pgp end",
              "/pgp char P")
    },

// XEP-0373: OpenPGP for XMPP
#ifdef HAVE_LIBGPGME
    { CMD_PREAMBLE("/ox",
                   parse_args, 1, 3, NULL)
      CMD_SUBFUNCS(
              { "log", cmd_ox_log })
      CMD_MAINFUNC(cmd_ox)
      CMD_TAGS(
      CMD_TAG_CHAT,
      CMD_TAG_UI)
      CMD_SYN(
              "/ox keys",
              "/ox contacts",
              "/ox start [<contact>]",
              "/ox end",
              "/ox log on|off|redact",
              "/ox char <char>",
              "/ox announce <file>",
              "/ox discover <jid>",
              "/ox request <jid> <keyid>")
      CMD_DESC(
             "OpenPGP (OX) commands to manage keys, and perform OpenPGP encryption during chat sessions. "
             "Your OpenPGP key needs a user-id with your JID URI (xmpp:local@domain.tld). "
             "A key can be generated with \"gpg --quick-gen-key xmpp:local@domain.tld future-default default 3y\". "
             "See man profanity-ox-setup for details on how to set up OX the first time.")
      CMD_ARGS(
              { "keys", "List all keys known to the system." },
              { "contacts", "Show contacts with assigned public keys." },
              { "start [<contact>]", "Start PGP encrypted chat, current contact will be used if not specified." },
              { "end", "End PGP encrypted chat with the current recipient." },
              { "log on|off", "Enable or disable plaintext logging of PGP encrypted messages." },
              { "log redact", "Log PGP encrypted messages, but replace the contents with [redacted]." },
              { "char <char>", "Set the character to be displayed next to PGP encrypted messages." },
              { "announce <file>", "Announce a public key by pushing it on the XMPP Server" },
              { "discover <jid>", "Discover public keys of a jid. The OpenPGP Key IDs will be displayed" },
              { "request <jid> <keyid>", "Request public key. See /ox discover to to get available key IDs." })
      CMD_EXAMPLES(
              "/ox log off",
              "/ox start odin@valhalla.edda",
              "/ox end",
              "/ox char X")
    },
#endif // HAVE_LIBGPGME

    { CMD_PREAMBLE("/otr",
                   parse_args, 1, 3, NULL)
      CMD_SUBFUNCS(
              { "char", cmd_otr_char },
              { "log", cmd_otr_log },
              { "libver", cmd_otr_libver },
              { "policy", cmd_otr_policy },
              { "gen", cmd_otr_gen },
              { "myfp", cmd_otr_myfp },
              { "theirfp", cmd_otr_theirfp },
              { "start", cmd_otr_start },
              { "end", cmd_otr_end },
              { "trust", cmd_otr_trust },
              { "untrust", cmd_otr_untrust },
              { "secret", cmd_otr_secret },
              { "question", cmd_otr_question },
              { "answer", cmd_otr_answer },
              { "sendfile", cmd_otr_sendfile })
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
              "/otr char <char>",
              "/otr sendfile on|off")
      CMD_DESC(
              "Off The Record (OTR) commands to manage keys, and perform OTR encryption during chat sessions.")
      CMD_ARGS(
              { "libver", "Show which version of the libotr library is being used." },
              { "gen", "Generate your private key." },
              { "myfp", "Show your fingerprint." },
              { "theirfp", "Show contacts fingerprint." },
              { "start [<contact>]", "Start an OTR session with contact, or current recipient if omitted." },
              { "end", "End the current OTR session." },
              { "trust|untrust", "Indicate whether or not you trust the contact's fingerprint." },
              { "secret <secret>", "Verify a contact's identity using a shared secret." },
              { "question <question> <answer>", "Verify a contact's identity using a question and expected answer." },
              { "answer <answer>", "Respond to a question answer verification request with your answer." },
              { "policy manual", "Set the global OTR policy to manual, OTR sessions must be started manually." },
              { "policy manual <contact>", "Set the OTR policy to manual for a specific contact." },
              { "policy opportunistic", "Set the global OTR policy to opportunistic, an OTR session will be attempted upon starting a conversation." },
              { "policy opportunistic <contact>", "Set the OTR policy to opportunistic for a specific contact." },
              { "policy always", "Set the global OTR policy to always, an error will be displayed if an OTR session cannot be initiated upon starting a conversation." },
              { "policy always <contact>", "Set the OTR policy to always for a specific contact." },
              { "log on|off", "Enable or disable plaintext logging of OTR encrypted messages." },
              { "log redact", "Log OTR encrypted messages, but replace the contents with [redacted]." },
              { "char <char>", "Set the character to be displayed next to OTR encrypted messages." },
              { "sendfile on|off", "Allow /sendfile to send unencrypted files while in an OTR session." })
      CMD_EXAMPLES(
              "/otr log off",
              "/otr policy manual",
              "/otr policy opportunistic odin@valhalla.edda",
              "/otr gen",
              "/otr start odin@valhalla.edda",
              "/otr myfp",
              "/otr theirfp",
              "/otr question \"What is the name of my rabbit?\" fiffi",
              "/otr end",
              "/otr char *")
    },

    { CMD_PREAMBLE("/outtype",
                   parse_args, 1, 1, &cons_outtype_setting)
      CMD_MAINFUNC(cmd_outtype)
      CMD_TAGS(
              CMD_TAG_CHAT)
      CMD_SYN(
              "/outtype on|off")
      CMD_DESC(
              "Send typing notifications, chat states (/states) will be enabled if this setting is enabled.")
      CMD_ARGS(
              { "on|off", "Enable or disable sending typing notifications." })
    },

    { CMD_PREAMBLE("/gone",
                   parse_args, 1, 1, &cons_gone_setting)
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
    },

    { CMD_PREAMBLE("/history",
                   parse_args, 1, 1, &cons_history_setting)
      CMD_MAINFUNC(cmd_history)
      CMD_TAGS(
              CMD_TAG_UI,
              CMD_TAG_CHAT)
      CMD_SYN(
              "/history on|off")
      CMD_DESC(
              "Switch chat history on or off, /logging chat will automatically be enabled when this setting is on. "
              "When history is enabled, previous messages are shown in chat windows.")
      CMD_ARGS(
              { "on|off", "Enable or disable showing chat history." })
    },

    { CMD_PREAMBLE("/log",
                   parse_args, 1, 2, &cons_log_setting)
      CMD_MAINFUNC(cmd_log)
      CMD_SYN(
              "/log where",
              "/log rotate on|off",
              "/log maxsize <bytes>",
              "/log shared on|off",
              "/log level INFO|DEBUG|WARN|ERROR")
      CMD_DESC(
              "Manage profanity log settings.")
      CMD_ARGS(
              { "where", "Show the current log file location." },
              { "rotate on|off", "Rotate log, default on. Does not take effect if you specified a filename yourself when starting Profanity." },
              { "maxsize <bytes>", "With rotate enabled, specifies the max log size, defaults to 10485760 (10MB)." },
              { "shared on|off", "Share logs between all instances, default: on. When off, the process id will be included in the log filename. Does not take effect if you specified a filename yourself when starting Profanity." },
              {"level INFO|DEBUG|WARN|ERROR", "Set the log level. Default is INFO. Only works with default log file, not with user provided log file during startup via -f." })
    },

    { CMD_PREAMBLE("/carbons",
                   parse_args, 1, 1, &cons_carbons_setting)
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
    },

    { CMD_PREAMBLE("/receipts",
                   parse_args, 2, 2, &cons_receipts_setting)
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
              { "send on|off", "Whether or not to send a receipt if one has been requested with a received message." })
    },

    { CMD_PREAMBLE("/reconnect",
                   parse_args, 1, 1, &cons_reconnect_setting)
      CMD_MAINFUNC(cmd_reconnect)
      CMD_TAGS(
              CMD_TAG_CONNECTION)
      CMD_SYN(
              "/reconnect <seconds>",
              "/reconnect now")
      CMD_DESC(
              "Set the reconnect attempt interval for when the connection is lost or immediately trigger a reconnect.")
      CMD_ARGS(
              { "<seconds>", "Number of seconds before attempting to reconnect, a value of 0 disables reconnect." },
              { "now", "Immediately trigger a reconnect." })
    },

    { CMD_PREAMBLE("/autoping",
                   parse_args, 2, 2, &cons_autoping_setting)
      CMD_MAINFUNC(cmd_autoping)
      CMD_TAGS(
              CMD_TAG_CONNECTION)
      CMD_SYN(
              "/autoping set <seconds>",
              "/autoping timeout <seconds>")
      CMD_DESC(
              "Set the interval between sending ping requests to the server to ensure the connection is kept alive.")
      CMD_ARGS(
              { "set <seconds>", "Number of seconds between sending pings, a value of 0 disables autoping." },
              { "timeout <seconds>", "Seconds to wait for autoping responses, after which the connection is considered broken." })
    },

    { CMD_PREAMBLE("/ping",
                   parse_args, 0, 1, NULL)
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
    },

    { CMD_PREAMBLE("/autoaway",
                   parse_args_with_freetext, 2, 3, &cons_autoaway_setting)
      CMD_MAINFUNC(cmd_autoaway)
      CMD_TAGS(
              CMD_TAG_PRESENCE)
      CMD_SYN(
              "/autoaway mode idle|away|off",
              "/autoaway time away|xa <minutes>",
              "/autoaway message away|xa <message>|off",
              "/autoaway check on|off")
      CMD_DESC(
              "Manage autoaway settings for idle time.")
      CMD_ARGS(
              { "mode idle", "Sends idle time, status remains online." },
              { "mode away", "Sends away and xa presence as well as idle time." },
              { "mode off", "Disabled (default)." },
              { "time away <minutes>", "Number of minutes before the away presence is sent, default: 15." },
              { "time xa <minutes>", "Number of minutes before the xa presence is sent, default: 0 (disabled)." },
              { "message away <message>", "Optional message to send with the away presence, default: off (disabled)." },
              { "message xa <message>", "Optional message to send with the xa presence, default: off (disabled)." },
              { "message away off", "Send no message with away presence." },
              { "message xa off", "Send no message with xa presence." },
              { "check on|off", "When enabled, checks for activity and sends online presence, default: on." })
      CMD_EXAMPLES(
              "/autoaway mode away",
              "/autoaway time away 30",
              "/autoaway message away Away from computer for a while",
              "/autoaway time xa 120",
              "/autoaway message xa Away from computer for a very long time",
              "/autoaway check off")
    },

    { CMD_PREAMBLE("/priority",
                   parse_args, 1, 1, NULL)
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
    },

    { CMD_PREAMBLE("/account",
                   parse_args, 0, 4, NULL)
      CMD_SUBFUNCS(
              { "list", cmd_account_list },
              { "show", cmd_account_show },
              { "add", cmd_account_add },
              { "remove", cmd_account_remove },
              { "enable", cmd_account_enable },
              { "disable", cmd_account_disable },
              { "rename", cmd_account_rename },
              { "default", cmd_account_default },
              { "set", cmd_account_set },
              { "clear", cmd_account_clear })
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
              "/account set <account> clientid \"<name> <version>\"",
              "/account set <account> tls force|allow|trust|legacy|disable",
              "/account set <account> auth default|legacy",
              "/account set <account> theme <theme>",
              "/account set <account> session_alarm <max_sessions>",
              "/account clear <account> password",
              "/account clear <account> eval_password",
              "/account clear <account> server",
              "/account clear <account> port",
              "/account clear <account> otr",
              "/account clear <account> pgpkeyid",
              "/account clear <account> startscript",
              "/account clear <account> clientid",
              "/account clear <account> muc",
              "/account clear <account> resource",
              "/account clear <account> session_alarm")
      CMD_DESC(
              "Commands for creating and managing accounts. "
              "Calling with no arguments will display information for the current account.")
      CMD_ARGS(
              { "list", "List all accounts." },
              { "enable <account>", "Enable the account, it will be used for autocompletion." },
              { "show <account>", "Show details for the specified account." },
              { "disable <account>", "Disable the account." },
              { "default set <account>", "Set the default account, used when no argument passed to the /connect command." },
              { "default off", "Clear the default account setting." },
              { "add <account>", "Create a new account." },
              { "remove <account>", "Remove an account." },
              { "rename <account> <newaccount>", "Rename 'account' to 'newaccount'." },
              { "set <account> jid <jid>", "Set the Jabber ID for the account, account name will be used if not set." },
              { "set <account> server <server>", "The chat server, if different to the domainpart of the JID." },
              { "set <account> port <port>", "The port used for connecting if not the default (5222, or 5223 for SSL)." },
              { "set <account> status <presence>", "The presence status to use on login." },
              { "set <account> status last", "Use your last status before logging out, when logging in." },
              { "set <account> <presence> <priority>", "Set the priority (-128..127) to use for the specified presence." },
              { "set <account> resource <resource>", "The resource to be used for this account, defaults to 'profanity'." },
              { "set <account> password <password>", "Password for the account, note this is currently stored in plaintext if set." },
              { "set <account> eval_password <command>", "Shell command evaluated to retrieve password for the account. Can be used to retrieve password from keyring." },
              { "set <account> muc <service>", "The default MUC chat service to use, defaults to the servers disco info response." },
              { "set <account> nick <nick>", "The default nickname to use when joining chat rooms." },
              { "set <account> otr <policy>", "Override global OTR policy for this account, see /otr." },
              { "set <account> pgpkeyid <pgpkeyid>", "Set the ID of the PGP key for this account, see /pgp." },
              { "set <account> startscript <script>", "Set the script to execute after connecting." },
              { "set <account> clientid \"<name> <version>\"", "Set XMPP client name for discovery according to XEP-0092. For privacy geeks. Recommendation to leave as is." },
              { "set <account> tls force", "Force TLS connection, and fail if one cannot be established, this is default behaviour." },
              { "set <account> tls allow", "Use TLS for the connection if it is available." },
              { "set <account> tls trust", "Force TLS connection and trust server's certificate." },
              { "set <account> tls legacy", "Use legacy TLS for the connection. It means server doesn't support STARTTLS and TLS is forced just after TCP connection is established." },
              { "set <account> tls disable", "Disable TLS for the connection." },
              { "set <account> auth default", "Use default authentication process." },
              { "set <account> auth legacy", "Allow legacy authentication." },
              { "set <account> theme <theme>", "Set the UI theme for the account." },
              { "set <account> session_alarm <max_sessions>", "Alarm about suspicious activity if sessions count exceeds max_sessions." },
              { "clear <account> server", "Remove the server setting for this account." },
              { "clear <account> port", "Remove the port setting for this account." },
              { "clear <account> password", "Remove the password setting for this account." },
              { "clear <account> eval_password", "Remove the eval_password setting for this account." },
              { "clear <account> otr", "Remove the OTR policy setting for this account." },
              { "clear <account> pgpkeyid", "Remove pgpkeyid associated with this account." },
              { "clear <account> startscript", "Remove startscript associated with this account." },
              { "clear <account> clientid", "Reset client's name to default." },
              { "clear <account> theme", "Clear the theme setting for the account, the global theme will be used." },
              { "clear <account> resource", "Remove the resource setting for this account." },
              { "clear <account> muc", "Remove the default MUC service setting." },
              { "clear <account> session_alarm", "Disable the session alarm." })
      CMD_EXAMPLES(
              "/account add me",
              "/account set me jid ulfhednar@valhalla.edda",
              "/account set me server talk.chat.com",
              "/account set me port 5111",
              "/account set me muc chatservice.mycompany.com",
              "/account set me nick dennis",
              "/account set me status dnd",
              "/account set me dnd -1",
              "/account set me clientid \"Profanity 0.42 (Dev)\"",
              "/account rename me chattyme",
              "/account clear me pgpkeyid")
    },

    { CMD_PREAMBLE("/plugins",
                   parse_args, 0, 3, NULL)
      CMD_SUBFUNCS(
              { "install", cmd_plugins_install },
              { "uninstall", cmd_plugins_uninstall },
              { "update", cmd_plugins_update },
              { "load", cmd_plugins_load },
              { "unload", cmd_plugins_unload },
              { "reload", cmd_plugins_reload },
              { "python_version", cmd_plugins_python_version })
      CMD_MAINFUNC(cmd_plugins)
      CMD_SYN(
              "/plugins",
              "/plugins install [<path or URL>]",
              "/plugins update [<path or URL>]",
              "/plugins uninstall [<plugin>]",
              "/plugins unload [<plugin>]",
              "/plugins load [<plugin>]",
              "/plugins reload [<plugin>]",
              "/plugins python_version")
      CMD_DESC(
              "Manage plugins. Passing no arguments lists installed plugins and global plugins which are available for local installation. Global directory for Python plugins is " GLOBAL_PYTHON_PLUGINS_PATH " and for C Plugins is " GLOBAL_C_PLUGINS_PATH ".")
      CMD_ARGS(
              { "install [<path or URL>]", "Install a plugin, or all plugins found in a directory (recursive), or download and install plugin (plugin name is based on basename). And loads it/them." },
              { "update [<path or URL>]", "Uninstall and then install the plugin. Plugin name to update is basename." },
              { "uninstall [<plugin>]", "Uninstall a plugin." },
              { "load [<plugin>]", "Load a plugin that already exists in the plugin directory, passing no argument loads all found plugins. It will be loaded upon next start too unless unloaded." },
              { "unload [<plugin>]", "Unload a loaded plugin, passing no argument will unload all plugins." },
              { "reload [<plugin>]", "Reload a plugin, passing no argument will reload all plugins." },
              { "python_version", "Show the Python interpreter version." })
      CMD_EXAMPLES(
              "/plugins install /home/steveharris/Downloads/metal.py",
              "/plugins install https://raw.githubusercontent.com/profanity-im/profanity-plugins/master/stable/sounds.py",
              "/plugins update /home/steveharris/Downloads/metal.py",
              "/plugins update https://raw.githubusercontent.com/profanity-im/profanity-plugins/master/stable/sounds.py",
              "/plugins uninstall browser.py",
              "/plugins load browser.py",
              "/plugins unload say.py",
              "/plugins reload wikipedia.py")
    },

    { CMD_PREAMBLE("/prefs",
                   parse_args, 0, 1, NULL)
      CMD_MAINFUNC(cmd_prefs)
      CMD_SYN(
              "/prefs [ui|desktop|chat|log|conn|presence|otr|pgp|omemo]")
      CMD_DESC(
              "Show preferences for different areas of functionality. "
              "Passing no arguments shows all preferences.")
      CMD_ARGS(
              { "ui", "User interface preferences." },
              { "desktop", "Desktop notification preferences." },
              { "chat", "Chat state preferences." },
              { "log", "Logging preferences." },
              { "conn", "Connection handling preferences." },
              { "presence", "Chat presence preferences." },
              { "otr", "Off The Record preferences." },
              { "pgp", "OpenPGP preferences." },
              { "omemo", "OMEMO preferences." })
    },

    { CMD_PREAMBLE("/theme",
                   parse_args, 1, 2, &cons_theme_setting)
      CMD_MAINFUNC(cmd_theme)
      CMD_TAGS(
              CMD_TAG_UI)
      CMD_SYN(
              "/theme list",
              "/theme load <theme>",
              "/theme full-load <theme>",
              "/theme colours",
              "/theme properties")
      CMD_DESC(
              "Load a theme, includes colours and UI options.")
      CMD_ARGS(
              { "list", "List all available themes." },
              { "load <theme>", "Load colours from specified theme. 'default' will reset to the default theme." },
              { "full-load <theme>", "Same as 'load' but will also load preferences set in the theme, not just colours." },
              { "colours", "Show colour values as rendered by the terminal." },
              { "properties", "Show colour settings for current theme." })
      CMD_EXAMPLES(
              "/theme list",
              "/theme load forest")
    },

    { CMD_PREAMBLE("/xmlconsole",
                   parse_args, 0, 0, NULL)
      CMD_MAINFUNC(cmd_xmlconsole)
      CMD_TAGS(
              CMD_TAG_UI)
      CMD_SYN(
              "/xmlconsole")
      CMD_DESC(
              "Open the XML console to view incoming and outgoing XMPP traffic.")
    },

    { CMD_PREAMBLE("/script",
                   parse_args, 1, 2, NULL)
      CMD_MAINFUNC(cmd_script)
      CMD_SYN(
              "/script run <script>",
              "/script list",
              "/script show <script>")
      CMD_DESC(
              "Run command scripts. "
              "Scripts are stored in $XDG_DATA_HOME/profanity/scripts/ which is usually $HOME/.local/share/profanity/scripts/.")
      CMD_ARGS(
              { "script run <script>", "Execute a script." },
              { "script list", "List all scripts TODO." },
              { "script show <script>", "Show the commands in script TODO." })
      CMD_EXAMPLES(
              "/script list",
              "/script run myscript",
              "/script show somescript")
    },

    { CMD_PREAMBLE("/export",
                   parse_args, 1, 1, NULL)
      CMD_MAINFUNC(cmd_export)
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

    { CMD_PREAMBLE("/cmd",
                   parse_args, 1, 3, NULL)
      CMD_SUBFUNCS(
              { "list", cmd_command_list },
              { "exec", cmd_command_exec })
      CMD_SYN(
              "/cmd list [<jid>]",
              "/cmd exec <command> [<jid>]")
      CMD_DESC(
              "Execute ad hoc commands.")
      CMD_ARGS(
              { "list", "List supported ad hoc commands." },
              { "exec <command>", "Execute a command." })
      CMD_EXAMPLES(
              "/cmd list",
              "/cmd exec ping")
    },

    { CMD_PREAMBLE("/omemo",
                   parse_args, 1, 3, NULL)
        CMD_SUBFUNCS(
            { "gen", cmd_omemo_gen },
            { "log", cmd_omemo_log },
            { "start", cmd_omemo_start },
            { "end", cmd_omemo_end },
            { "trustmode", cmd_omemo_trust_mode },
            { "trust", cmd_omemo_trust },
            { "untrust", cmd_omemo_untrust },
            { "fingerprint", cmd_omemo_fingerprint },
            { "char", cmd_omemo_char },
            { "policy", cmd_omemo_policy },
            { "clear_device_list", cmd_omemo_clear_device_list },
            { "qrcode", cmd_omemo_qrcode },
            { "colors", cmd_omemo_colors })
        CMD_TAGS(
            CMD_TAG_CHAT,
            CMD_TAG_UI)
        CMD_SYN(
            "/omemo gen",
            "/omemo log on|off|redact",
            "/omemo start [<contact>]",
            "/omemo trust [<contact>] <fingerprint>",
            "/omemo end",
            "/omemo fingerprint [<contact>]",
            "/omemo char <char>",
            "/omemo trustmode manual|firstusage|blind",
            "/omemo policy manual|automatic|always",
            "/omemo clear_device_list",
            "/omemo qrcode",
            "/omemo colors on|off")
        CMD_DESC(
            "OMEMO commands to manage keys, and perform encryption during chat sessions.")
        CMD_ARGS(
            { "gen",                     "Generate OMEMO cryptographic materials for current account." },
            { "start [<contact>]",       "Start an OMEMO session with contact, or current recipient if omitted." },
            { "end",                     "End the current OMEMO session." },
            { "log on|off",              "Enable or disable plaintext logging of OMEMO encrypted messages." },
            { "log redact",              "Log OMEMO encrypted messages, but replace the contents with [redacted]." },
            { "fingerprint [<contact>]", "Show contact's fingerprints, or current recipient's if omitted." },
            { "char <char>",             "Set the character to be displayed next to OMEMO encrypted messages." },
            { "trustmode manual",        "Set the global OMEMO trust mode to manual, OMEMO keys has to be trusted manually." },
            { "trustmode firstusage",    "Set the global OMEMO trust mode to ToFu, first OMEMO keys trusted automatically." },
            { "trustmode blind",         "Set the global OMEMO trust mode to blind, ALL OMEMO keys trusted automatically." },
            { "policy manual",           "Set the global OMEMO policy to manual, OMEMO sessions must be started manually." },
            { "policy automatic",        "Set the global OMEMO policy to opportunistic, an OMEMO session will be attempted upon starting a conversation." },
            { "policy always",           "Set the global OMEMO policy to always, an error will be displayed if an OMEMO session cannot be initiated upon starting a conversation." },
            { "clear_device_list",       "Clear your own device list on server side. Each client will reannounce itself when connected back."},
            { "qrcode",                  "Display QR code of your OMEMO fingerprint"},
            { "colors on|off",            "Enable or disable coloring of OMEMO messages. Default: off." })
        CMD_EXAMPLES(
            "/omemo gen",
            "/omemo start odin@valhalla.edda",
            "/omemo trust c4f9c875-144d7a3b-0c4a05b6-ca3be51a-a037f329-0bd3ae62-07f99719-55559d2a",
            "/omemo untrust loki@valhalla.edda c4f9c875-144d7a3b-0c4a05b6-ca3be51a-a037f329-0bd3ae62-07f99719-55559d2a",
            "/omemo char *")
    },

    { CMD_PREAMBLE("/save",
                   parse_args, 0, 0, NULL)
      CMD_MAINFUNC(cmd_save)
      CMD_SYN(
              "/save")
      CMD_DESC(
              "Save preferences to configuration file.")
    },

    { CMD_PREAMBLE("/reload",
                   parse_args, 0, 0, NULL)
      CMD_MAINFUNC(cmd_reload)
      CMD_SYN(
              "/reload")
      CMD_DESC(
              "Reload preferences from configuration file.")
    },

    { CMD_PREAMBLE("/paste",
                   parse_args, 0, 0, NULL)
      CMD_MAINFUNC(cmd_paste)
      CMD_SYN(
              "/paste")
      CMD_DESC(
              "Paste clipboard.")
    },

    { CMD_PREAMBLE("/color",
                   parse_args, 1, 2, &cons_color_setting)
      CMD_MAINFUNC(cmd_color)
      CMD_TAGS(
              CMD_TAG_UI)
      CMD_SYN(
              "/color on|off|redgreen|blue",
              "/color own on|off")
      CMD_DESC(
              "Settings for consistent color generation for nicks (XEP-0392). Including corrections for Color Vision Deficiencies. "
              "Your terminal needs to support 256 colors.")
      CMD_ARGS(
              { "on|off|redgreen|blue", "Enable or disable nick colorization for MUC nicks. 'redgreen' is for people with red/green blindness and 'blue' for people with blue blindness." },
              { "own on|off", "Enable color generation for own nick. If disabled the color from the color from the theme ('me') will get used." })
      CMD_EXAMPLES(
              "/color off",
              "/color on",
              "/color blue",
              "/color own off")
    },

    { CMD_PREAMBLE("/stamp",
                   parse_args, 0, 2, NULL)
      CMD_MAINFUNC(cmd_stamp)
      CMD_TAGS(
              CMD_TAG_UI)
      CMD_SYN("/stamp outgoing <string>",
              "/stamp incoming <string>",
              "/stamp unset outgoing|incoming")
      CMD_DESC("Set chat window stamp. "
               "The format of line in the chat window is: \"<timestamp> <encryption sign> <stamp> <message>\" "
               "where <stamp> is \"me:\" for incoming messages or \"username@server/resource\" for outgoing messages. "
               "This command allows to change <stamp> value.")
      CMD_ARGS({ "outgoing", "Set outgoing stamp" },
               { "incoming", "Set incoming stamp"},
               { "unset outgoing|incoming", "Use the defaults"})
      CMD_EXAMPLES(
              "/stamp outgoing -->",
              "/stamp incoming <--",
              "/stamp unset incoming")
    },

    { CMD_PREAMBLE("/avatar",
                   parse_args, 1, 2, NULL)
      CMD_MAINFUNC(cmd_avatar)
      CMD_TAGS(
              CMD_TAG_CHAT)
      CMD_SYN(
              "/avatar set <path>",
              "/avatar disable",
              "/avatar get <barejid>",
              "/avatar open <barejid>")
      CMD_DESC(
              "Upload an avatar for yourself, "
              "disable your avatar, "
              "or download a contact's avatar (XEP-0084). "
              "If nothing happens after using this command, the user either doesn't have an avatar set "
              "or doesn't use XEP-0084 to publish it.")
      CMD_ARGS(
              { "set <path>", "Set avatar to the image at <path>." },
              { "disable", "Disable avatar publishing; your avatar will not display to others." },
              { "get <barejid>", "Download the avatar. barejid is the JID to download avatar from." },
              { "open <barejid>", "Download avatar and open it with command. See /executable." })
      CMD_EXAMPLES(
              "/avatar set ~/images/avatar.png",
              "/avatar disable",
              "/avatar get thor@valhalla.edda",
              "/avatar open freyja@vanaheimr.edda") },

    { CMD_PREAMBLE("/correction",
                   parse_args, 1, 2, &cons_correction_setting)
      CMD_MAINFUNC(cmd_correction)
      CMD_TAGS(
              CMD_TAG_UI,
              CMD_TAG_CHAT,
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/correction <on>|<off>",
              "/correction char <char>")
      CMD_DESC(
              "Settings regarding Last Message Correction (XEP-0308). "
              "Corrections will only work in MUC and regular chat windows. MUC PMs won't be allowed. "
              "For more information on how to correct messages, see: /help correct.")
      CMD_ARGS(
              { "on|off", "Enable/Disable support for last message correction." },
              { "char", "Set character that will prefix corrected messages. Default: '+'." })
    },

    { CMD_PREAMBLE("/correct",
                   parse_args_as_one, 1, 1, NULL)
      CMD_MAINFUNC(cmd_correct)
      CMD_TAGS(
              CMD_TAG_CHAT,
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/correct <message>")
      CMD_DESC(
              "Correct and resend the last message (XEP-0308). "
              "Use tab completion to get the last sent message. "
              "For more information on how to configure corrections, see: /help correction.")
      CMD_ARGS(
              { "message", "The corrected message." })
    },

    { CMD_PREAMBLE("/slashguard",
                   parse_args, 1, 1, &cons_slashguard_setting)
      CMD_MAINFUNC(cmd_slashguard)
      CMD_TAGS(
              CMD_TAG_UI,
              CMD_TAG_CHAT)
      CMD_SYN(
              "/slashguard on|off")
      CMD_DESC(
              "Slashguard won't accept a slash in the first 4 characters of your input field. "
              "It tries to protect you from typing ' /quit' and similar things in chats.")
      CMD_ARGS(
              { "on|off", "Enable or disable slashguard." })
    },

    { CMD_PREAMBLE("/serversoftware",
                   parse_args, 1, 1, NULL)
      CMD_MAINFUNC(cmd_serversoftware)
      CMD_TAGS(
              CMD_TAG_DISCOVERY)
      CMD_SYN(
              "/serversoftware <domain>")
      CMD_DESC(
              "Find server or component software version information.")
      CMD_ARGS(
              { "<domain>", "The jid of your server or component." })
      CMD_EXAMPLES(
              "/serversoftware valhalla.edda",
              "/serversoftware xmpp.vanaheimr.edda")
    },

    { CMD_PREAMBLE("/executable",
                   parse_args, 2, 4, &cons_executable_setting)
      CMD_SUBFUNCS(
              { "avatar",  cmd_executable_avatar },
              { "urlopen", cmd_executable_urlopen },
              { "urlsave", cmd_executable_urlsave },
              { "editor", cmd_executable_editor },
              { "vcard_photo", cmd_executable_vcard_photo })
      CMD_TAGS(
              CMD_TAG_DISCOVERY)
      CMD_SYN(
              "/executable avatar set <cmdtemplate>",
              "/executable avatar default",
              "/executable urlopen set <cmdtemplate>",
              "/executable urlopen default",
              "/executable urlsave set <cmdtemplate>",
              "/executable urlsave default",
              "/executable editor set <cmdtemplate>",
              "/executable editor default",
              "/executable vcard_photo set <cmdtemplate>",
              "/executable vcard_photo default")
      CMD_DESC(
              "Configure executable that should be called upon a certain command.")
      CMD_ARGS(
              { "avatar set", "Set executable that is run by /avatar open. Use your favorite image viewer." },
              { "avatar default", "Restore to default settings." },
              { "urlopen set", "Set executable that is run by /url open. Takes a command template that replaces %u and %p with the URL and path respectively." },
              { "urlopen default", "Restore to default settings." },
              { "urlsave set", "Set executable that is run by /url save. Takes a command template that replaces %u and %p with the URL and path respectively." },
              { "urlsave default", "Use the built-in download method for saving." },
              { "editor set", "Set editor to be used with /editor. Needs a terminal editor or a script to run a graphical editor." },
              { "editor default", "Restore to default settings." },
              { "vcard_photo set", "Set executable that is run by /vcard photo open. Takes a command template that replaces %p with the path" },
              { "vcard_photo default", "Restore to default settings." })
      CMD_EXAMPLES(
              "/executable avatar xdg-open",
              "/executable urlopen set \"xdg-open %u\"",
              "/executable urlopen set \"firefox %u\"",
              "/executable urlopen default",
              "/executable urlsave set \"wget %u -O %p\"",
              "/executable urlsave set \"curl %u -o %p\"",
              "/executable urlsave default",
              "/executable vcard_photo set \"feh %p\"",
              "/executable editor set \"emacsclient -t\"")
    },

    { CMD_PREAMBLE("/url",
                   parse_args, 2, 3, NULL)
      CMD_SUBFUNCS(
              { "open", cmd_url_open },
              { "save", cmd_url_save })
      CMD_TAGS(
              CMD_TAG_CHAT,
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/url open <url>",
              "/url save <url> [<path>]")
      CMD_DESC(
              "Open or save URLs. This works with OMEMO encrypted files as well.")
      CMD_ARGS(
              { "open", "Open URL with predefined executable." },
              { "save", "Save URL to optional path. The location is displayed after successful download." })
      CMD_EXAMPLES(
              "/url open https://profanity-im.github.io",
              "/url save https://profanity-im.github.io/guide/latest/userguide.html /home/user/Download/")
    },

    { CMD_PREAMBLE("/mam",
                   parse_args, 1, 1, &cons_mam_setting)
      CMD_MAINFUNC(cmd_mam)
      CMD_TAGS(
              CMD_TAG_CHAT)
      CMD_SYN(
              "/mam <on>|<off>")
      CMD_DESC(
              "Enable/Disable Message Archive Management (XEP-0313) "
              "Currently MAM in groupchats (MUCs) is not supported. "
              "Use the PG UP key to load more history.")
      CMD_ARGS(
              { "on|off", "Enable or disable MAM" })
    },

    { CMD_PREAMBLE("/changepassword",
                   parse_args, 0, 0, NULL)
      CMD_MAINFUNC(cmd_change_password)
      CMD_SYN(
              "/changepassword")
      CMD_DESC(
              "Change password of logged in account")
    },

    { CMD_PREAMBLE("/editor",
                   parse_args, 0, 0, NULL)
      CMD_MAINFUNC(cmd_editor)
      CMD_TAGS(
              CMD_TAG_CHAT,
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/editor")
      CMD_DESC(
              "Spawn external editor to edit message. "
              "After editing the inputline may appear empty. Press enter to send the text anyways. "
              "Use /executable to set your favourite editor." )
    },

    { CMD_PREAMBLE("/correct-editor",
                   parse_args, 0, 0, NULL)
      CMD_MAINFUNC(cmd_correct_editor)
      CMD_TAGS(
              CMD_TAG_CHAT,
              CMD_TAG_GROUPCHAT)
      CMD_SYN(
              "/correct-editor")
      CMD_DESC(
              "Spawn external editor to correct and resend the last message (XEP-0308). "
              "For more information on how to configure corrections, see: /help correction. "
              "Use /executable to set your favourite editor.")
    },

    { CMD_PREAMBLE("/silence",
                   parse_args, 1, 1, &cons_silence_setting)
      CMD_MAINFUNC(cmd_silence)
      CMD_TAGS(
              CMD_TAG_CHAT)
      CMD_SYN(
              "/silence on|off")
      CMD_DESC(
              "Let's you silence all message attempts from people who are not in your roster.")
    },

    { CMD_PREAMBLE("/register",
                   parse_args, 2, 6, NULL)
      CMD_MAINFUNC(cmd_register)
      CMD_TAGS(
              CMD_TAG_CONNECTION)
      CMD_SYN(
              "/register <username> <server> [port <port>] [tls force|allow|trust|legacy|disable]")
      CMD_DESC(
              "Register an account on a server.")
      CMD_ARGS(
              { "<username>", "Username to register with." },
              { "<server>", "Server to register account on." },
              { "port <port>", "The port to use if different to the default (5222, or 5223 for SSL)." },
              { "tls force", "Force TLS connection, and fail if one cannot be established. This is the default behavior." },
              { "tls allow", "Use TLS for the connection if it is available." },
              { "tls trust", "Force TLS connection and trust the server's certificate." },
              { "tls legacy", "Use legacy TLS for the connection. This forces TLS just after the TCP connection is established. Use when a server doesn't support STARTTLS." },
              { "tls disable", "Disable TLS for the connection." })
      CMD_EXAMPLES(
              "/register odin valhalla.edda ",
              "/register freyr vanaheimr.edda port 5678",
              "/register me 127.0.0.1 tls disable",
              "/register someuser my.xmppserv.er port 5443 tls force")
    },

    { CMD_PREAMBLE("/mood",
                   parse_args, 1, 3, &cons_mood_setting)
      CMD_MAINFUNC(cmd_mood)
      CMD_TAGS(
              CMD_TAG_CHAT)
      CMD_SYN(
              "/mood on|off",
              "/mood set <mood> [text]",
              "/mood clear")
      CMD_DESC(
              "Set your mood (XEP-0107).")
      CMD_ARGS(
              { "on|off", "Enable or disable displaying the mood of other users. On by default."},
              { "set <mood> [text]", "Set user mood to <mood> with an optional [text]. Use /mood set <tab> to toggle through predefined moods." },
              { "clear", "Clear your user mood." })
      CMD_EXAMPLES(
              "/mood set happy \"So happy to use Profanity!\"",
              "/mood set amazed",
              "/mood clear")
    },

    { CMD_PREAMBLE("/strophe",
                   parse_args, 2, 2, &cons_strophe_setting)
      CMD_MAINFUNC(cmd_strophe)
      CMD_TAGS(
              CMD_TAG_CONNECTION)
      CMD_SYN(
              "/strophe verbosity 0-3",
              "/strophe sm on|no-resend|off")
      CMD_DESC(
              "Modify libstrophe settings.")
      CMD_ARGS(
              { "verbosity 0-3", "Set libstrophe verbosity level when log level is 'DEBUG'." },
              { "sm on|no-resend|off", "Enable or disable Stream-Management (SM) as of XEP-0198. The 'no-resend' option enables SM, but won't re-send un-ACK'ed messages on re-connect." })
      CMD_EXAMPLES(
              "/strophe verbosity 3",
              "/strophe sm no-resend")
    },

    { CMD_PREAMBLE("/privacy",
                   parse_args, 2, 3, &cons_privacy_setting)
      CMD_SUBFUNCS(
              { "os", cmd_os })
      CMD_MAINFUNC(cmd_privacy)
      CMD_TAGS(
              CMD_TAG_CHAT,
              CMD_TAG_DISCOVERY)
      CMD_SYN(
              "/privacy logging on|redact|off",
              "/privacy os on|off")
      CMD_DESC(
              "Configure privacy settings. "
              "Also check the the following settings in /account: "
              "clientid to set the client identification name "
              "session_alarm to configure an alarm when more clients log in.")
      CMD_ARGS(
              { "logging on|redact|off", "Switch chat logging. This will also disable logging in the internally used SQL database. Your messages will not be saved anywhere locally. This might have unintended consequences, such as not being able to decrypt OMEMO encrypted messages received later via MAM, and should be used with caution." },
              { "os on|off", "Choose whether to include the OS name if a user asks for software information (XEP-0092)." }
              )
      CMD_EXAMPLES(
              "/privacy logging off",
              "/privacy os off")
    },

    { CMD_PREAMBLE("/redraw",
                   parse_args, 0, 0, NULL)
      CMD_MAINFUNC(cmd_redraw)
      CMD_TAGS(
              CMD_TAG_UI)
      CMD_SYN(
              "/redraw")
      CMD_DESC(
              "Redraw user interface. Can be used when some other program interrupted profanity or wrote to the same terminal and the interface looks \"broken\"." )
    },

    // NEXT-COMMAND (search helper)
};

// clang-format on

static GHashTable* search_index;

static char*
_cmd_index(const Command* cmd)
{
    GString* index_source = g_string_new("");
    index_source = g_string_append(index_source, cmd->cmd);
    index_source = g_string_append(index_source, " ");
    index_source = g_string_append(index_source, cmd->help.desc);
    index_source = g_string_append(index_source, " ");

    int len = g_strv_length((gchar**)cmd->help.tags);
    for (int i = 0; i < len; i++) {
        index_source = g_string_append(index_source, cmd->help.tags[i]);
        index_source = g_string_append(index_source, " ");
    }
    len = g_strv_length((gchar**)cmd->help.synopsis);
    for (int i = 0; i < len; i++) {
        index_source = g_string_append(index_source, cmd->help.synopsis[i]);
        index_source = g_string_append(index_source, " ");
    }
    for (int i = 0; cmd->help.args[i][0] != NULL; i++) {
        index_source = g_string_append(index_source, cmd->help.args[i][0]);
        index_source = g_string_append(index_source, " ");
        index_source = g_string_append(index_source, cmd->help.args[i][1]);
        index_source = g_string_append(index_source, " ");
    }

    auto_gcharv gchar** tokens = g_str_tokenize_and_fold(index_source->str, NULL, NULL);
    g_string_free(index_source, TRUE);

    GString* index = g_string_new("");
    for (int i = 0; i < g_strv_length(tokens); i++) {
        index = g_string_append(index, tokens[i]);
        index = g_string_append(index, " ");
    }

    return g_string_free(index, FALSE);
}

GList*
cmd_search_index_any(char* term)
{
    GList* results = NULL;

    auto_gcharv gchar** processed_terms = g_str_tokenize_and_fold(term, NULL, NULL);
    int terms_len = g_strv_length(processed_terms);

    for (int i = 0; i < terms_len; i++) {
        GList* index_keys = g_hash_table_get_keys(search_index);
        GList* curr = index_keys;
        while (curr) {
            char* index_entry = g_hash_table_lookup(search_index, curr->data);
            if (g_str_match_string(processed_terms[i], index_entry, FALSE)) {
                results = g_list_append(results, curr->data);
            }
            curr = g_list_next(curr);
        }
        g_list_free(index_keys);
    }

    return results;
}

GList*
cmd_search_index_all(char* term)
{
    GList* results = NULL;

    auto_gcharv gchar** terms = g_str_tokenize_and_fold(term, NULL, NULL);
    int terms_len = g_strv_length(terms);

    GList* commands = g_hash_table_get_keys(search_index);
    GList* curr = commands;
    while (curr) {
        char* command = curr->data;
        int matches = 0;
        for (int i = 0; i < terms_len; i++) {
            char* command_index = g_hash_table_lookup(search_index, command);
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

    return results;
}

static void
_cmd_uninit(void)
{
    cmd_ac_uninit();
    g_hash_table_destroy(commands);
    commands = NULL;
    g_hash_table_destroy(search_index);
    search_index = NULL;
}

/*
 * Initialise command autocompleter and history
 */
void
cmd_init(void)
{
    log_info("Initialising commands");

    prof_add_shutdown_routine(_cmd_uninit);

    cmd_ac_init();

    search_index = g_hash_table_new_full(g_str_hash, g_str_equal, free, g_free);

    // load command defs into hash table
    commands = g_hash_table_new(g_str_hash, g_str_equal);
    for (unsigned int i = 0; i < ARRAY_SIZE(command_defs); i++) {
        const Command* pcmd = command_defs + i;

        // add to hash
        g_hash_table_insert(commands, pcmd->cmd, (gpointer)pcmd);

        // add to search index
        g_hash_table_insert(search_index, strdup(pcmd->cmd), _cmd_index(pcmd));

        // add to commands and help autocompleters
        cmd_ac_add_cmd(pcmd);
    }

    // load aliases
    GList* aliases = prefs_get_aliases();
    GList* curr = aliases;
    while (curr) {
        ProfAlias* alias = curr->data;
        cmd_ac_add_alias(alias);
        curr = g_list_next(curr);
    }
    prefs_free_aliases(aliases);
}

gboolean
cmd_valid_tag(const char* const str)
{
    return ((g_strcmp0(str, CMD_TAG_CHAT) == 0) || (g_strcmp0(str, CMD_TAG_GROUPCHAT) == 0) || (g_strcmp0(str, CMD_TAG_PRESENCE) == 0) || (g_strcmp0(str, CMD_TAG_ROSTER) == 0) || (g_strcmp0(str, CMD_TAG_DISCOVERY) == 0) || (g_strcmp0(str, CMD_TAG_CONNECTION) == 0) || (g_strcmp0(str, CMD_TAG_UI) == 0) || (g_strcmp0(str, CMD_TAG_PLUGINS) == 0));
}

Command*
cmd_get(const char* const command)
{
    if (commands) {
        return g_hash_table_lookup(commands, command);
    } else {
        return NULL;
    }
}

GList*
cmd_get_ordered(const char* const tag)
{
    GList* ordered_commands = NULL;

    GHashTableIter iter;
    gpointer key;
    gpointer value;

    g_hash_table_iter_init(&iter, commands);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        Command* pcmd = (Command*)value;
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
_cmd_has_tag(Command* pcmd, const char* const tag)
{
    for (int i = 0; pcmd->help.tags[i] != NULL; i++) {
        if (g_strcmp0(tag, pcmd->help.tags[i]) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

static int
_cmp_command(Command* cmd1, Command* cmd2)
{
    return g_strcmp0(cmd1->cmd, cmd2->cmd);
}

void
command_docgen(void)
{
    GList* cmds = NULL;

    for (unsigned int i = 0; i < ARRAY_SIZE(command_defs); i++) {
        const Command* pcmd = command_defs + i;
        cmds = g_list_insert_sorted(cmds, (gpointer)pcmd, (GCompareFunc)_cmp_command);
    }

    FILE* toc_fragment = fopen("toc_fragment.html", "w");
    FILE* main_fragment = fopen("main_fragment.html", "w");

    fputs("<ul><li><ul><li>\n", toc_fragment);
    fputs("<hr>\n", main_fragment);

    GList* curr = cmds;
    while (curr) {
        Command* pcmd = curr->data;

        fprintf(toc_fragment, "<a href=\"#%s\">%s</a>,\n", &pcmd->cmd[1], pcmd->cmd);
        fprintf(main_fragment, "<a name=\"%s\"></a>\n", &pcmd->cmd[1]);
        fprintf(main_fragment, "<h4>%s</h4>\n", pcmd->cmd);

        fputs("<p><b>Synopsis</b></p>\n", main_fragment);
        fputs("<p><pre><code>", main_fragment);
        int i = 0;
        while (pcmd->help.synopsis[i]) {
            char* str1 = str_replace(pcmd->help.synopsis[i], "<", "&lt;");
            char* str2 = str_replace(str1, ">", "&gt;");
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
                char* str1 = str_replace(pcmd->help.args[i][0], "<", "&lt;");
                char* str2 = str_replace(str1, ">", "&gt;");
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

void
command_mangen(void)
{
    GList* cmds = NULL;

    for (unsigned int i = 0; i < ARRAY_SIZE(command_defs); i++) {
        const Command* pcmd = command_defs + i;
        cmds = g_list_insert_sorted(cmds, (gpointer)pcmd, (GCompareFunc)_cmp_command);
    }

    create_dir("docs");

    GDateTime* now = g_date_time_new_now_local();
    auto_gchar gchar* date = g_date_time_format(now, "%F");
    auto_gchar gchar* header = g_strdup_printf(".TH man 1 \"%s\" \"" PACKAGE_VERSION "\" \"Profanity XMPP client\"\n", date);
    if (!header) {
        log_error("command_mangen(): could not allocate memory");
        return;
    }
    g_date_time_unref(now);

    GList* curr = cmds;
    while (curr) {
        Command* pcmd = curr->data;

        auto_gchar gchar* filename = g_strdup_printf("docs/profanity-%s.1", &pcmd->cmd[1]);
        if (!filename) {
            log_error("command_mangen(): could not allocate memory");
            return;
        }
        FILE* manpage = fopen(filename, "w");

        fprintf(manpage, "%s\n", header);
        fputs(".SH NAME\n", manpage);
        fprintf(manpage, "%s\n", pcmd->cmd);

        fputs("\n.SH DESCRIPTION\n", manpage);
        fprintf(manpage, "%s\n", pcmd->help.desc);

        fputs("\n.SH SYNOPSIS\n", manpage);
        int i = 0;
        while (pcmd->help.synopsis[i]) {
            fprintf(manpage, "%s\n", pcmd->help.synopsis[i]);
            fputs("\n.LP\n", manpage);
            i++;
        }

        if (pcmd->help.args[0][0] != NULL) {
            fputs("\n.SH ARGUMENTS\n", manpage);
            for (i = 0; pcmd->help.args[i][0] != NULL; i++) {
                fprintf(manpage, ".PP\n\\fB%s\\fR\n", pcmd->help.args[i][0]);
                fprintf(manpage, ".RS 4\n%s\n.RE\n", pcmd->help.args[i][1]);
            }
        }

        if (pcmd->help.examples[0] != NULL) {
            fputs("\n.SH EXAMPLES\n", manpage);
            int i = 0;
            while (pcmd->help.examples[i]) {
                fprintf(manpage, "%s\n", pcmd->help.examples[i]);
                fputs("\n.LP\n", manpage);
                i++;
            }
        }

        fclose(manpage);
        curr = g_list_next(curr);
    }

    printf("\nProcessed %d commands.\n\n", g_list_length(cmds));

    g_list_free(cmds);
}
