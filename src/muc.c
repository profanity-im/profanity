/*
 * muc.c
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "contact.h"
#include "common.h"
#include "jid.h"
#include "tools/autocomplete.h"
#include "ui/ui.h"

typedef struct _muc_room_t {
    char *room; // e.g. test@conference.server
    char *nick; // e.g. Some User
    char *password;
    char *subject;
    char *autocomplete_prefix;
    GList *pending_broadcasts;
    gboolean autojoin;
    gboolean pending_nick_change;
    GHashTable *roster;
    Autocomplete nick_ac;
    GHashTable *nick_changes;
    gboolean roster_received;
} ChatRoom;

GHashTable *rooms = NULL;
Autocomplete invite_ac;

static void _free_room(ChatRoom *room);
static gint _compare_participants(PContact a, PContact b);

void
muc_init(void)
{
    invite_ac = autocomplete_new();
}

void
muc_close(void)
{
    autocomplete_free(invite_ac);
    if (rooms != NULL) {
        g_hash_table_destroy(rooms);
        rooms = NULL;
    }
}

void
muc_add_invite(char *room)
{
    autocomplete_add(invite_ac, room);
}

void
muc_remove_invite(char *room)
{
    autocomplete_remove(invite_ac, room);
}

gint
muc_invite_count(void)
{
    return autocomplete_length(invite_ac);
}

GSList *
muc_get_invites(void)
{
    return autocomplete_get_list(invite_ac);
}

gboolean
muc_invites_include(const char * const room)
{
    GSList *invites = autocomplete_get_list(invite_ac);
    GSList *curr = invites;
    while (curr != NULL) {
        if (strcmp(curr->data, room) == 0) {
            g_slist_free_full(invites, g_free);
            return TRUE;
        } else {
            curr = g_slist_next(curr);
        }
    }
    g_slist_free_full(invites, g_free);
    return FALSE;
}

void
muc_reset_invites_ac(void)
{
    autocomplete_reset(invite_ac);
}

char *
muc_find_invite(char *search_str)
{
    return autocomplete_complete(invite_ac, search_str, TRUE);
}

void
muc_clear_invites(void)
{
    autocomplete_clear(invite_ac);
}

/*
 * Join the chat room with the specified nickname
 */
void
muc_join_room(const char * const room, const char * const nick,
    const char * const password, gboolean autojoin)
{
    if (rooms == NULL) {
        rooms = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
            (GDestroyNotify)_free_room);
    }

    ChatRoom *new_room = malloc(sizeof(ChatRoom));
    new_room->room = strdup(room);
    new_room->nick = strdup(nick);
    new_room->autocomplete_prefix = NULL;
    if (password != NULL) {
        new_room->password = strdup(password);
    } else {
        new_room->password = NULL;
    }
    new_room->subject = NULL;
    new_room->pending_broadcasts = NULL;
    new_room->roster = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
        (GDestroyNotify)p_contact_free);
    new_room->nick_ac = autocomplete_new();
    new_room->nick_changes = g_hash_table_new_full(g_str_hash, g_str_equal,
        g_free, g_free);
    new_room->roster_received = FALSE;
    new_room->pending_nick_change = FALSE;
    new_room->autojoin = autojoin;

    g_hash_table_insert(rooms, strdup(room), new_room);
}

/*
 * Leave the room
 */
void
muc_leave_room(const char * const room)
{
    if (rooms != NULL) {
        g_hash_table_remove(rooms, room);
    }
}

/*
 * Returns TRUE if the user is currently in the room
 */
gboolean
muc_room_is_active(const char * const room)
{
    if (rooms != NULL) {
        ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

        if (chat_room != NULL) {
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

gboolean
muc_room_is_autojoin(const char * const room)
{
    if (rooms != NULL) {
        ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

        if (chat_room != NULL) {
            return chat_room->autojoin;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

void
muc_set_subject(const char * const room, const char * const subject)
{
    if (rooms != NULL) {
        ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

        if (chat_room != NULL) {
            if (chat_room->subject != NULL) {
                free(chat_room->subject);
            }
            chat_room->subject = strdup(subject);
        }
    }
}

char *
muc_get_subject(const char * const room)
{
    if (rooms != NULL) {
        ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

        if (chat_room != NULL) {
            return chat_room->subject;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

void
muc_add_pending_broadcast(const char * const room, const char * const message)
{
    if (rooms != NULL) {
        ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

        if (chat_room != NULL) {
            chat_room->pending_broadcasts = g_list_append(chat_room->pending_broadcasts, strdup(message));
        }
    }
}

GList *
muc_get_pending_broadcasts(const char * const room)
{
    if (rooms != NULL) {
        ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

        if (chat_room != NULL) {
            return chat_room->pending_broadcasts;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

char *
muc_get_old_nick(const char * const room, const char * const new_nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

    if ((chat_room != NULL) && (chat_room->pending_nick_change)) {
        return g_hash_table_lookup(chat_room->nick_changes, new_nick);
    }

    return NULL;
}

/*
 * Flag that the user has sent a nick change to the service
 * and is awaiting the response
 */
void
muc_set_room_pending_nick_change(const char * const room, const char * const new_nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        chat_room->pending_nick_change = TRUE;
        g_hash_table_insert(chat_room->nick_changes, strdup(new_nick), strdup(chat_room->nick));
    }
}

/*
 * Returns TRUE if the room is awaiting the result of a
 * nick change
 */
gboolean
muc_is_room_pending_nick_change(const char * const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        return chat_room->pending_nick_change;
    } else {
        return FALSE;
    }
}

/*
 * Change the current nuck name for the room, call once
 * the service has responded
 */
void
muc_complete_room_nick_change(const char * const room, const char * const nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
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
GList *
muc_get_active_room_list(void)
{
    if (rooms != NULL) {
        return g_hash_table_get_keys(rooms);
    } else {
        return NULL;
    }
}

/*
 * Return current users nickname for the specified room
 * The nickname is owned by the chat room and should not be modified or freed
 */
char *
muc_get_room_nick(const char * const room)
{
    if (rooms != NULL) {
        ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

        if (chat_room != NULL) {
            return chat_room->nick;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

/*
 * Return password for the specified room
 * The password is owned by the chat room and should not be modified or freed
 */
char *
muc_get_room_password(const char * const room)
{
    if (rooms != NULL) {
        ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

        if (chat_room != NULL) {
            return chat_room->password;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

/*
 * Returns TRUE if the specified nick exists in the room's roster
 */
gboolean
muc_nick_in_roster(const char * const room, const char * const nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        PContact contact = g_hash_table_lookup(chat_room->roster, nick);
        if (contact != NULL) {
            return TRUE;
        } else {
            return FALSE;
        }
    }

    return FALSE;
}

/*
 * Add a new chat room member to the room's roster
 */
gboolean
muc_add_to_roster(const char * const room, const char * const nick,
    const char * const show, const char * const status,
    const char * const caps_str)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);
    gboolean updated = FALSE;

    if (chat_room != NULL) {
        PContact old = g_hash_table_lookup(chat_room->roster, nick);

        if (old == NULL) {
            updated = TRUE;
            autocomplete_add(chat_room->nick_ac, nick);
        } else if ((g_strcmp0(p_contact_presence(old), show) != 0) ||
                    (g_strcmp0(p_contact_status(old), status) != 0)) {
            updated = TRUE;
        }
        PContact contact = p_contact_new(nick, NULL, NULL, NULL, NULL, FALSE);
        resource_presence_t resource_presence = resource_presence_from_string(show);
        Resource *resource = resource_new(nick, resource_presence, status, 0, caps_str);
        p_contact_set_presence(contact, resource);
        g_hash_table_replace(chat_room->roster, strdup(nick), contact);
    }

    return updated;
}

/*
 * Remove a room member from the room's roster
 */
void
muc_remove_from_roster(const char * const room, const char * const nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        g_hash_table_remove(chat_room->roster, nick);
        autocomplete_remove(chat_room->nick_ac, nick);
    }
}

PContact
muc_get_participant(const char * const room, const char * const nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        PContact participant = g_hash_table_lookup(chat_room->roster, nick);
        return participant;
    }

    return NULL;
}

/*
 * Return a list of PContacts representing the room members in the room's roster
 * The list is owned by the room and must not be mofified or freed
 */
GList *
muc_get_roster(const char * const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        GList *result = NULL;
        GHashTableIter iter;
        gpointer key;
        gpointer value;

        g_hash_table_iter_init(&iter, chat_room->roster);
        while (g_hash_table_iter_next(&iter, &key, &value)) {
            result = g_list_insert_sorted(result, value, (GCompareFunc)_compare_participants);
        }

        return result;
    } else {
        return NULL;
    }
}

/*
 * Return a Autocomplete representing the room member's in the roster
 */
Autocomplete
muc_get_roster_ac(const char * const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        return chat_room->nick_ac;
    } else {
        return NULL;
    }
}

/*
 * Set to TRUE when the rooms roster has been fully recieved
 */
void
muc_set_roster_received(const char * const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        chat_room->roster_received = TRUE;
    }
}

/*
 * Returns TRUE id the rooms roster has been fully recieved
 */
gboolean
muc_get_roster_received(const char * const room)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        return chat_room->roster_received;
    } else {
        return FALSE;
    }
}

/*
 * Remove the old_nick from the roster, and flag that a pending nickname change
 * is in progress
 */
void
muc_set_roster_pending_nick_change(const char * const room,
    const char * const new_nick, const char * const old_nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        g_hash_table_insert(chat_room->nick_changes, strdup(new_nick), strdup(old_nick));
        muc_remove_from_roster(room, old_nick);
    }
}

/*
 * Complete the pending nick name change for a contact in the room's roster
 * The new nick name will be added to the roster
 * The old nick name will be returned in a new string which must be freed by
 * the caller
 */
char *
muc_complete_roster_nick_change(const char * const room,
    const char * const nick)
{
    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        char *old_nick = g_hash_table_lookup(chat_room->nick_changes, nick);
        if (old_nick != NULL) {
            char *old_nick_cpy = strdup(old_nick);
            g_hash_table_remove(chat_room->nick_changes, nick);

            return old_nick_cpy;
        }
    }

    return NULL;
}

void
muc_autocomplete(char *input, int *size)
{
    if (rooms == NULL) {
        return;
    }

    char *recipient = ui_current_recipient();
    ChatRoom *chat_room = g_hash_table_lookup(rooms, recipient);

    if (chat_room == NULL) {
        return;
    }

    if (chat_room->nick_ac == NULL) {
        return;
    }

    input[*size] = '\0';
    char *search_str = NULL;

    gchar *last_space = g_strrstr(input, " ");
    if (last_space == NULL) {
        search_str = input;
        if (chat_room->autocomplete_prefix == NULL) {
            chat_room->autocomplete_prefix = strdup("");
        }
    } else {
        search_str = last_space+1;
        if (chat_room->autocomplete_prefix == NULL) {
            chat_room->autocomplete_prefix = g_strndup(input, search_str - input);
        }
    }

    char *result = autocomplete_complete(chat_room->nick_ac, search_str, FALSE);
    if (result != NULL) {
        GString *replace_with = g_string_new(chat_room->autocomplete_prefix);
        g_string_append(replace_with, result);
        if (last_space == NULL || (*(last_space+1) == '\0')) {
            g_string_append(replace_with, ": ");
        }
        ui_replace_input(input, replace_with->str, size);
        g_string_free(replace_with, TRUE);
        g_free(result);
    }

    return;
}

void
muc_reset_autocomplete(const char * const room)
{
    if (rooms == NULL) {
        return;
    }

    ChatRoom *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room == NULL) {
        return;
    }

    if (chat_room->nick_ac != NULL) {
        autocomplete_reset(chat_room->nick_ac);
    }

    if (chat_room->autocomplete_prefix != NULL) {
        free(chat_room->autocomplete_prefix);
        chat_room->autocomplete_prefix = NULL;
    }
}

static void
_free_room(ChatRoom *room)
{
    if (room != NULL) {
        free(room->room);
        free(room->nick);
        free(room->subject);
        free(room->password);
        if (room->autocomplete_prefix != NULL) {
            free(room->autocomplete_prefix);
        }
        if (room->roster != NULL) {
            g_hash_table_destroy(room->roster);
        }
        if (room->nick_ac != NULL) {
            autocomplete_free(room->nick_ac);
        }
        if (room->nick_changes != NULL) {
            g_hash_table_destroy(room->nick_changes);
        }
        if (room->pending_broadcasts != NULL) {
            g_list_free_full(room->pending_broadcasts, free);
        }
        free(room);
    }
}

static
gint _compare_participants(PContact a, PContact b)
{
    const char * utf8_str_a = p_contact_barejid(a);
    const char * utf8_str_b = p_contact_barejid(b);

    gchar *key_a = g_utf8_collate_key(utf8_str_a, -1);
    gchar *key_b = g_utf8_collate_key(utf8_str_b, -1);

    gint result = g_strcmp0(key_a, key_b);

    g_free(key_a);
    g_free(key_b);

    return result;
}
