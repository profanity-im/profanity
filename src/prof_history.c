/*
 * prof_history.c
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

#include "prof_history.h"

struct p_history_session_t {
    GList *items;
    GList *sess_curr;
    GList *sess_new;
    GList *orig_curr;
};

struct p_history_t {
    GList *items;
    guint max_size;
    struct p_history_session_t session;
};

static void _replace_history_with_session(PHistory history);
static gboolean _adding_new(PHistory history);
static void _reset_session(PHistory history);
static gboolean _has_session(PHistory history);
static void _remove_first(PHistory history);
static void _update_current_session_item(PHistory history, char *item);
static void _add_to_history(PHistory history, char *item);
static void _remove_last_session_item(PHistory history);
static void _replace_current_with_original(PHistory history);
static void _create_session(PHistory history);
static void _session_previous(PHistory history);
static void _session_next(PHistory history);

PHistory
p_history_new(unsigned int size)
{
    PHistory new_history = malloc(sizeof(struct p_history_t));
    new_history->items = NULL;
    new_history->max_size = size;

    _reset_session(new_history);

    return new_history;
}

void
p_history_append(PHistory history, char *item)
{
    char *copied = "";
    if (item != NULL) {
        copied = strdup(item);
    }

    if (history->items == NULL) {
        _add_to_history(history, copied);
        return;
    }

    if (!_has_session(history)) {
        if (g_list_length(history->items) == history->max_size) {
            _remove_first(history);
        }

        _add_to_history(history, copied);

    } else {
        _update_current_session_item(history, copied);

        if (_adding_new(history)) {
            if (strcmp(history->session.sess_curr->data, "") == 0) {
                _remove_last_session_item(history);
            }

            _replace_history_with_session(history);

        } else {
            _remove_last_session_item(history);

            char *new = strdup(history->session.sess_curr->data);
            history->session.items = g_list_append(history->session.items, new);

            _replace_current_with_original(history);
            _replace_history_with_session(history);
        }
    }
}

char *
p_history_previous(PHistory history, char *item)
{
    // no history
    if (history->items == NULL) {
        return NULL;
    }

    char *copied = "";
    if (item != NULL) {
        copied = strdup(item);
    }

    if (!_has_session(history)) {
        _create_session(history);

        // add the new item
        history->session.items = g_list_append(history->session.items, copied);
        history->session.sess_new = g_list_last(history->session.items);

        char *result = strdup(history->session.sess_curr->data);
        return result;
    } else {
        _update_current_session_item(history, copied);
        _session_previous(history);
    }

    char *result = strdup(history->session.sess_curr->data);
    return result;
}

char *
p_history_next(PHistory history, char *item)
{
    // no history, or no session, return NULL
    if ((history->items == NULL) || (history->session.items == NULL)) {
        return NULL;
    }

    char *copied = "";
    if (item != NULL) {
        copied = strdup(item);
    }

    _update_current_session_item(history, copied);
    _session_next(history);

    char *result = strdup(history->session.sess_curr->data);
    return result;
}

static void
_replace_history_with_session(PHistory history)
{
    g_list_free(history->items);
    history->items = g_list_copy(history->session.items);

    if (g_list_length(history->items) > history->max_size) {
        _remove_first(history);
    }

    _reset_session(history);
}

static gboolean
_adding_new(PHistory history)
{
    return (history->session.sess_curr ==
        g_list_last(history->session.items));
}

static void
_reset_session(PHistory history)
{
    history->session.items = NULL;
    history->session.sess_curr = NULL;
    history->session.sess_new = NULL;
    history->session.orig_curr = NULL;
}

static gboolean
_has_session(PHistory history)
{
    return (history->session.items != NULL);
}

static void
_remove_first(PHistory history)
{
    GList *first = g_list_first(history->items);
    char *first_item = first->data;
    history->items = g_list_remove(history->items, first_item);
}

static void
_update_current_session_item(PHistory history, char *item)
{
    history->session.sess_curr->data = item;
}

static void
_add_to_history(PHistory history, char *item)
{
    history->items = g_list_append(history->items, item);
}

static void
_remove_last_session_item(PHistory history)
{
    history->session.items = g_list_reverse(history->session.items);
    GList *first = g_list_first(history->session.items);
    history->session.items =
        g_list_remove(history->session.items, first->data);
    history->session.items = g_list_reverse(history->session.items);
}

static void
_replace_current_with_original(PHistory history)
{
    history->session.sess_curr->data = strdup(history->session.orig_curr->data);
}

static void
_create_session(PHistory history)
{
    history->session.items = g_list_copy(history->items);
    history->session.sess_curr = g_list_last(history->session.items);
    history->session.orig_curr = g_list_last(history->items);
}

static void
_session_previous(PHistory history)
{
    history->session.sess_curr =
        g_list_previous(history->session.sess_curr);
    if (history->session.orig_curr == NULL)
        history->session.orig_curr = g_list_last(history->items);
    else
        history->session.orig_curr =
            g_list_previous(history->session.orig_curr);

    if (history->session.sess_curr == NULL) {
        history->session.sess_curr = g_list_first(history->session.items);
        history->session.orig_curr = g_list_first(history->items);
    }
}

static void
_session_next(PHistory history)
{
    history->session.sess_curr = g_list_next(history->session.sess_curr);
    history->session.orig_curr = g_list_next(history->session.orig_curr);

    if (history->session.sess_curr == NULL) {
        history->session.sess_curr = g_list_last(history->session.items);
        history->session.orig_curr = NULL;
    }
}
