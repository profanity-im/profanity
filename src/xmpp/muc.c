/*
 * muc.c
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <glib.h>

#include "common.h"
#include "tools/autocomplete.h"
#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/jid.h"
#include "xmpp/muc.h"
#include "xmpp/contact.h"

typedef struct _muc_room_t {
    char *room; // e.g. test@conference.server
    char *nick; // e.g. Some User
    muc_role_t role;
    muc_affiliation_t affiliation;
    char *password;
    char *subject;
    char *autocomplete_prefix;
    gboolean pending_config;
    GList *pending_broadcasts;
    gboolean autojoin;
    gboolean pending_nick_change;
    GHashTable *roster;
    Autocomplete nick_ac;
    Autocomplete jid_ac;
    GHashTable *nick_changes;
    gboolean roster_received;
    muc_member_type_t member_type;
} ChatRoom;

GHashTable *rooms = NULL;
GHashTable *invite_passwords = NULL;
Autocomplete invite_ac;

static void _free_room(ChatRoom *room);
static gint _compare_occupants(Occupant *a, Occupant *b);
static muc_role_t _role_from_string(const char *const role);
static muc_affiliation_t _affiliation_from_string(const char *const affiliation);
static char* _role_to_string(muc_role_t role);
static char* _affiliation_to_string(muc_affiliation_t affiliation);
static Occupant* _muc_occupant_new(const char *const nick, const char *const jid, muc_role_t role,
    muc_affiliation_t affiliation, resource_presence_t presence, const char *const status);
static void _occupant_free(Occupant *occupant);

void
muc_init(void)
{
    invite_ac = autocomplete_new();
    rooms = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)_free_room);
    invite_passwords = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

void
muc_close(void)
{
    autocomplete_free(invite_ac);
    g_hash_table_destroy(rooms);
    g_hash_table_destroy(invite_passwords);
    rooms = NULL;
    invite_passwords = NULL;
}

void
muc_invites_add(const char *const room, const char *const password)
{
    autocomplete_add(invite_ac, room);
    if (password) {
        g_hash_table_replace(invite_passwords, strdup(room), strdup(password));
    }
}

void
muc_invites_remove(const char *const room)
{
    autocomplete_remove(invite_ac, room);
    g_hash_table_remove(invite_passwords, room);
}

gint
muc_invites_count(void)
{
    return autocomplete_length(invite_ac);
}

GList*
muc_invites(void)
{
    return autocomplete_create_list(invite_ac);
}

char*
muc_invite_password(const char *const room)
{
    return g_hash_table_lookup(invite_passwords, room);
}

gboolean
muc_invites_contain(const char *const room)
{
    GList *invites = autocomplete_create_list(invite_ac);
    GList *curr = invites;
    while (curr) {
        if (strcmp(curr->data, room) == 0) {
            g_list_free_full(invites, g_free);
            return TRUE;
        } else {
            curr = g_list_next(curr);
        }
    }
    g_list_free_full(invites, g_free);

    return FALSE;
}

void
muc_invites_reset_ac(void)
{
    autocomplete_reset(invite_ac);
}

char*
muc_invites_find(const char *const search_str, gboolean previous)
{
    return autocomplete_complete(invite_ac, search_str, TRUE, previous);
}

void
muc_invites_clear(void)
{
    autocomplete_clear(invite_ac);
    if (invite_passwords) {
        g_hash_table_remove_all(invite_passwords);
    }
}

void
muc_join(const char *const room, const char *const nick, const char *const password, gboolean autojoin)
{
    ChatRoom *new_room = malloc(sizeof(ChatRoom));
    new_room->room = strdup(room);
    new_room->nick = strdup(nick);
    new_room->role = MUC_ROLE_NONE;
    new_room->affiliation = MUC_AFFILIATION_NONE;
    new_room->autocomplete_prefix = NULL;
    if (password) {
        new_room->password = strdup(password);
    } else {
        new_room->password = NULL;
    }
    new_room->subject = NULL;
    new_room->pending_broadcasts = NULL;
    new_room->pending_config = FALSE;
    new_room->roster = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)_occupant_free);
    new_room->nick_ac = autocomplete_new();
    new_room->jid_ac = autocomplete_new();
    new_room->nick_changes = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    new_room->roster_received = FALSE;
    new_room->pending_nick_change = FALSE;
    new_room->autojoin = autojoin;
    new_room->member_type = MUC_MEMBER_TYPE_UNKNOWN;

    g_hash_table_insert(rooms, strdup(room), new_room);
}

void
muc_leave(const char *const room)
{
    g_hash_table_remove(rooms, room);
}

gboolean
muc_requires_config(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        return chat_room->pending_config;
    } else {
        return FALSE;
    }

}

void
muc_set_requires_config(const char *const room, gboolean val)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        chat_room->pending_config = val;
    }
}

void
muc_set_features(const char *const room, GSList *features)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room && features) {
        if (g_slist_find_custom(features, "muc_membersonly", (GCompareFunc)g_strcmp0)) {
            chat_room->member_type = MUC_MEMBER_TYPE_MEMBERS_ONLY;
        } else {
            chat_room->member_type = MUC_MEMBER_TYPE_PUBLIC;
        }
    }
}

/*
 * Returns TRUE if the user is currently in the room
 */
gboolean
muc_active(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    return (chat_room != NULL);
}

gboolean
muc_autojoin(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        return chat_room->autojoin;
    } else {
        return FALSE;
    }
}

void
muc_set_subject(const char *const room, const char *const subject)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        free(chat_room->subject);
        if (subject) {
            chat_room->subject = strdup(subject);
        } else {
            chat_room->subject = NULL;
        }
    }
}

char*
muc_subject(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        return chat_room->subject;
    } else {
        return NULL;
    }
}

void
muc_pending_broadcasts_add(const char *const room, const char *const message)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        chat_room->pending_broadcasts = g_list_append(chat_room->pending_broadcasts, strdup(message));
    }
}

GList*
muc_pending_broadcasts(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        return chat_room->pending_broadcasts;
    } else {
        return NULL;
    }
}

char*
muc_old_nick(const char *const room, const char *const new_nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room && chat_room->pending_nick_change) {
        return g_hash_table_lookup(chat_room->nick_changes, new_nick);
    } else {
        return NULL;
    }
}

/*
 * Flag that the user has sent a nick change to the service
 * and is awaiting the response
 */
void
muc_nick_change_start(const char *const room, const char *const new_nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        chat_room->pending_nick_change = TRUE;
        g_hash_table_insert(chat_room->nick_changes, strdup(new_nick), strdup(chat_room->nick));
    }
}

/*
 * Returns TRUE if the room is awaiting the result of a
 * nick change
 */
gboolean
muc_nick_change_pending(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        return chat_room->pending_nick_change;
    } else {
        return FALSE;
    }
}

/*
 * Change the current nick name for the room, call once
 * the service has responded
 */
void
muc_nick_change_complete(const char *const room, const char *const nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        g_hash_table_remove(chat_room->roster, chat_room->nick);
        autocomplete_remove(chat_room->nick_ac, chat_room->nick);
        free(chat_room->nick);
        chat_room->nick = strdup(nick);
        chat_room->pending_nick_change = FALSE;
        g_hash_table_remove(chat_room->nick_changes, nick);
    }
}

/*
 * Return a list of room names
 * The contents of the list are owned by the chat room and should not be
 * modified or freed.
 */
GList*
muc_rooms(void)
{
    return g_hash_table_get_keys(rooms);
}

/*
 * Return current users nickname for the specified room
 * The nickname is owned by the chat room and should not be modified or freed
 */
char*
muc_nick(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        return chat_room->nick;
    } else {
        return NULL;
    }
}

/*
 * Return password for the specified room
 * The password is owned by the chat room and should not be modified or freed
 */
char*
muc_password(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        return chat_room->password;
    } else {
        return NULL;
    }
}

/*
 * Returns TRUE if the specified nick exists in the room's roster
 */
gboolean
muc_roster_contains_nick(const char *const room, const char *const nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        Occupant *occupant = g_hash_table_lookup(chat_room->roster, nick);
        return (occupant != NULL);
    } else {
        return FALSE;
    }
}

/*
 * Add a new chat room member to the room's roster
 */
gboolean
muc_roster_add(const char *const room, const char *const nick, const char *const jid, const char *const role,
    const char *const affiliation, const char *const show, const char *const status)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    gboolean updated = FALSE;
    resource_presence_t new_presence = resource_presence_from_string(show);

    if (chat_room) {
        Occupant *old = g_hash_table_lookup(chat_room->roster, nick);

        if (!old) {
            updated = TRUE;
            autocomplete_add(chat_room->nick_ac, nick);
        } else if (old->presence != new_presence ||
                    (g_strcmp0(old->status, status) != 0)) {
            updated = TRUE;
        }

        resource_presence_t presence = resource_presence_from_string(show);
        muc_role_t role_t = _role_from_string(role);
        muc_affiliation_t affiliation_t = _affiliation_from_string(affiliation);
        Occupant *occupant = _muc_occupant_new(nick, jid, role_t, affiliation_t, presence, status);
        g_hash_table_replace(chat_room->roster, strdup(nick), occupant);

        if (jid) {
            Jid *jidp = jid_create(jid);
            if (jidp->barejid) {
                autocomplete_add(chat_room->jid_ac, jidp->barejid);
            }
            jid_destroy(jidp);
        }
    }

    return updated;
}

/*
 * Remove a room member from the room's roster
 */
void
muc_roster_remove(const char *const room, const char *const nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        g_hash_table_remove(chat_room->roster, nick);
        autocomplete_remove(chat_room->nick_ac, nick);
    }
}

Occupant*
muc_roster_item(const char *const room, const char *const nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        Occupant *occupant = g_hash_table_lookup(chat_room->roster, nick);
        return occupant;
    } else {
        return NULL;
    }
}

/*
 * Return a list of PContacts representing the room members in the room's roster
 */
GList*
muc_roster(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        GList *result = NULL;
        GList *occupants = g_hash_table_get_values(chat_room->roster);

        GList *curr = occupants;
        while (curr) {
            result = g_list_insert_sorted(result, curr->data, (GCompareFunc)_compare_occupants);
            curr = g_list_next(curr);
        }

        g_list_free(occupants);

        return result;
    } else {
        return NULL;
    }
}

/*
 * Return a Autocomplete representing the room member's in the roster
 */
Autocomplete
muc_roster_ac(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        return chat_room->nick_ac;
    } else {
        return NULL;
    }
}

Autocomplete
muc_roster_jid_ac(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        return chat_room->jid_ac;
    } else {
        return NULL;
    }
}

/*
 * Set to TRUE when the rooms roster has been fully received
 */
void
muc_roster_set_complete(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        chat_room->roster_received = TRUE;
    }
}

/*
 * Returns TRUE id the rooms roster has been fully received
 */
gboolean
muc_roster_complete(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        return chat_room->roster_received;
    } else {
        return FALSE;
    }
}

gboolean
muc_occupant_available(Occupant *occupant)
{
    return (occupant->presence == RESOURCE_ONLINE || occupant->presence == RESOURCE_CHAT);
}

const char*
muc_occupant_affiliation_str(Occupant *occupant)
{
    return _affiliation_to_string(occupant->affiliation);
}

const char*
muc_occupant_role_str(Occupant *occupant)
{
    return _role_to_string(occupant->role);
}

GSList*
muc_occupants_by_role(const char *const room, muc_role_t role)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        GSList *result = NULL;
        GHashTableIter iter;
        gpointer key;
        gpointer value;

        g_hash_table_iter_init(&iter, chat_room->roster);
        while (g_hash_table_iter_next(&iter, &key, &value)) {
            Occupant *occupant = (Occupant *)value;
            if (occupant->role == role) {
                result = g_slist_insert_sorted(result, value, (GCompareFunc)_compare_occupants);
            }
        }
        return result;
    } else {
        return NULL;
    }
}

GSList*
muc_occupants_by_affiliation(const char *const room, muc_affiliation_t affiliation)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        GSList *result = NULL;
        GHashTableIter iter;
        gpointer key;
        gpointer value;

        g_hash_table_iter_init(&iter, chat_room->roster);
        while (g_hash_table_iter_next(&iter, &key, &value)) {
            Occupant *occupant = (Occupant *)value;
            if (occupant->affiliation == affiliation) {
                result = g_slist_insert_sorted(result, value, (GCompareFunc)_compare_occupants);
            }
        }
        return result;
    } else {
        return NULL;
    }
}

/*
 * Remove the old_nick from the roster, and flag that a pending nickname change
 * is in progress
 */
void
muc_occupant_nick_change_start(const char *const room, const char *const new_nick, const char *const old_nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        g_hash_table_insert(chat_room->nick_changes, strdup(new_nick), strdup(old_nick));
        muc_roster_remove(room, old_nick);
    }
}

/*
 * Complete the pending nick name change for a contact in the room's roster
 * The new nick name will be added to the roster
 * The old nick name will be returned in a new string which must be freed by
 * the caller
 */
char*
muc_roster_nick_change_complete(const char *const room, const char *const nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        char *old_nick = g_hash_table_lookup(chat_room->nick_changes, nick);
        if (old_nick) {
            char *old_nick_cpy = strdup(old_nick);
            g_hash_table_remove(chat_room->nick_changes, nick);

            return old_nick_cpy;
        }
    }

    return NULL;
}

char*
muc_autocomplete(ProfWin *window, const char *const input, gboolean previous)
{
    if (window->type == WIN_MUC) {
        ProfMucWin *mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        ChatRoom *chat_room = g_hash_table_lookup(rooms, mucwin->roomjid);

        if (chat_room && chat_room->nick_ac) {
            const char * search_str = NULL;

            gchar *last_space = g_strrstr(input, " ");
            if (!last_space) {
                search_str = input;
                if (!chat_room->autocomplete_prefix) {
                    chat_room->autocomplete_prefix = strdup("");
                }
            } else {
                search_str = last_space+1;
                if (!chat_room->autocomplete_prefix) {
                    chat_room->autocomplete_prefix = g_strndup(input, search_str - input);
                }
            }

            char *result = autocomplete_complete(chat_room->nick_ac, search_str, FALSE, previous);
            if (result) {
                GString *replace_with = g_string_new(chat_room->autocomplete_prefix);
                g_string_append(replace_with, result);
                if (!last_space || (*(last_space+1) == '\0')) {
                    g_string_append(replace_with, ": ");
                }
                g_free(result);
                result = replace_with->str;
                g_string_free(replace_with, FALSE);
                return result;
            }
        }
    }

    return NULL;
}

void
muc_jid_autocomplete_reset(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        if (chat_room->jid_ac) {
            autocomplete_reset(chat_room->jid_ac);
        }
    }
}

void
muc_jid_autocomplete_add_all(const char *const room, GSList *jids)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        if (chat_room->jid_ac) {
            GSList *curr_jid = jids;
            while (curr_jid) {
                const char *jid = curr_jid->data;
                Jid *jidp = jid_create(jid);
                if (jidp) {
                    if (jidp->barejid) {
                        autocomplete_add(chat_room->jid_ac, jidp->barejid);
                    }
                }
                jid_destroy(jidp);
                curr_jid = g_slist_next(curr_jid);
            }
        }
    }
}

void
muc_autocomplete_reset(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        if (chat_room->nick_ac) {
            autocomplete_reset(chat_room->nick_ac);
        }

        if (chat_room->autocomplete_prefix) {
            free(chat_room->autocomplete_prefix);
            chat_room->autocomplete_prefix = NULL;
        }
    }
}

char*
muc_role_str(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        return _role_to_string(chat_room->role);
    } else {
        return "none";
    }
}

void
muc_set_role(const char *const room, const char *const role)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        chat_room->role = _role_from_string(role);
    }
}

char*
muc_affiliation_str(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        return _affiliation_to_string(chat_room->affiliation);
    } else {
        return "none";
    }
}

void
muc_set_affiliation(const char *const room, const char *const affiliation)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        chat_room->affiliation = _affiliation_from_string(affiliation);
    }
}

muc_member_type_t
muc_member_type(const char *const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    if (chat_room) {
        return chat_room->member_type;
    } else {
        return MUC_MEMBER_TYPE_UNKNOWN;
    }
}


static void
_free_room(ChatRoom *room)
{
    if (room) {
        free(room->room);
        free(room->nick);
        free(room->subject);
        free(room->password);
        free(room->autocomplete_prefix);
        if (room->roster) {
            g_hash_table_destroy(room->roster);
        }
        autocomplete_free(room->nick_ac);
        autocomplete_free(room->jid_ac);
        if (room->nick_changes) {
            g_hash_table_destroy(room->nick_changes);
        }
        if (room->pending_broadcasts) {
            g_list_free_full(room->pending_broadcasts, free);
        }
        free(room);
    }
}

static gint
_compare_occupants(Occupant *a, Occupant *b)
{
    const char * utf8_str_a = a->nick_collate_key;
    const char * utf8_str_b = b->nick_collate_key;

    gint result = g_strcmp0(utf8_str_a, utf8_str_b);

    return result;
}

static muc_role_t
_role_from_string(const char *const role)
{
    if (role) {
        if (g_strcmp0(role, "visitor") == 0) {
            return MUC_ROLE_VISITOR;
        } else if (g_strcmp0(role, "participant") == 0) {
            return MUC_ROLE_PARTICIPANT;
        } else if (g_strcmp0(role, "moderator") == 0) {
            return MUC_ROLE_MODERATOR;
        } else {
            return MUC_ROLE_NONE;
        }
    } else {
        return MUC_ROLE_NONE;
    }
}

static char*
_role_to_string(muc_role_t role)
{
    char *result = NULL;

    switch (role) {
    case MUC_ROLE_NONE:
        result = "none";
        break;
    case MUC_ROLE_VISITOR:
        result = "visitor";
        break;
    case MUC_ROLE_PARTICIPANT:
        result = "participant";
        break;
    case MUC_ROLE_MODERATOR:
        result = "moderator";
        break;
    default:
        result = "none";
        break;
    }

    return result;
}

static muc_affiliation_t
_affiliation_from_string(const char *const affiliation)
{
    if (affiliation) {
        if (g_strcmp0(affiliation, "outcast") == 0) {
            return MUC_AFFILIATION_OUTCAST;
        } else if (g_strcmp0(affiliation, "member") == 0) {
            return MUC_AFFILIATION_MEMBER;
        } else if (g_strcmp0(affiliation, "admin") == 0) {
            return MUC_AFFILIATION_ADMIN;
        } else if (g_strcmp0(affiliation, "owner") == 0) {
            return MUC_AFFILIATION_OWNER;
        } else {
            return MUC_AFFILIATION_NONE;
        }
    } else {
        return MUC_AFFILIATION_NONE;
    }
}

static char*
_affiliation_to_string(muc_affiliation_t affiliation)
{
    char *result = NULL;

    switch (affiliation) {
    case MUC_AFFILIATION_NONE:
        result = "none";
        break;
    case MUC_AFFILIATION_OUTCAST:
        result = "outcast";
        break;
    case MUC_AFFILIATION_MEMBER:
        result = "member";
        break;
    case MUC_AFFILIATION_ADMIN:
        result = "admin";
        break;
    case MUC_AFFILIATION_OWNER:
        result = "owner";
        break;
    default:
        result = "none";
        break;
    }

    return result;
}

static Occupant*
_muc_occupant_new(const char *const nick, const char *const jid, muc_role_t role, muc_affiliation_t affiliation,
    resource_presence_t presence, const char *const status)
{
    Occupant *occupant = malloc(sizeof(Occupant));

    if (nick) {
        occupant->nick = strdup(nick);
        occupant->nick_collate_key = g_utf8_collate_key(occupant->nick, -1);
    } else {
        occupant->nick = NULL;
        occupant->nick_collate_key = NULL;
    }

    if (jid) {
        occupant->jid = strdup(jid);
    } else {
        occupant->jid = NULL;
    }

    occupant->presence = presence;

    if (status) {
        occupant->status = strdup(status);
    } else {
        occupant->status = NULL;
    }

    occupant->role = role;
    occupant->affiliation = affiliation;

    return occupant;
}

static void
_occupant_free(Occupant *occupant)
{
    if (occupant) {
        free(occupant->nick);
        free(occupant->nick_collate_key);
        free(occupant->jid);
        free(occupant->status);
        free(occupant);
    }
}
