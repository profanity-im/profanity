#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "prof_history.h"

struct p_history_t {
    GList *items; // the history
    GList *items_curr; // pointer to the current line in the history
    GList *session; // a copy of the history for edits
    GList *sess_curr; // pointer to the current item in the session
    GList *sess_new; // pointer to a possible new element in the session
    guint max_size;
};

static void _replace_history_with_session(PHistory history);

PHistory p_history_new(unsigned int size)
{
    PHistory new_history = malloc(sizeof(struct p_history_t));
    new_history->items = NULL;
    new_history->items_curr = NULL;
    new_history->session = NULL;
    new_history->sess_curr = NULL;
    new_history->sess_new = NULL;
    new_history->max_size = size;

    return new_history;
}

void p_history_append(PHistory history, char *item)
{
    char *copied = strdup(item);

    // if not editing history (no session)
    if (history->session == NULL) {
        
        // if already at max size
        if (g_list_length(history->items) == history->max_size) {

            // remove first element
            GList *first = g_list_first(history->items);
            const char *first_item = (char *) first->data;
            history->items = g_list_remove(history->items, first_item);
        }
    
        // append the new item onto the history
        history->items = g_list_append(history->items, copied);

    // if editing history (session exists with possible changes)
    } else {
        
        // if adding a new element, copy the session over the history
        if (history->sess_curr == history->sess_new) {
            _replace_history_with_session(history);

        // otherwise, adding edited history item
        } else {
            // remove the new item, from the session
            history->session = g_list_reverse(history->session);
            GList *first = g_list_first(history->session);
            const char *first_item = (char *) first->data;
            history->session = g_list_remove(history->session, first_item);
            history->session = g_list_reverse(history->session);

            // copy sess_curr to the end of the session
            char *new_data = strdup(history->sess_curr->data);
            history->session = g_list_append(history->session, new_data);

            // replace the edited version with the data from the history
            history->sess_curr->data = strdup(history->items_curr->data);
            
            // rewrite history from the session
            _replace_history_with_session(history);
        }
    }
}

static void _replace_history_with_session(PHistory history)
{
    g_list_free(history->items);
    history->items = g_list_copy(history->session);
    
    // remove first if overrun max size
    if (g_list_length(history->items) > history->max_size) {
        GList *first = g_list_first(history->items);
        const char *first_item = (char *) first->data;
        history->items = g_list_remove(history->items, first_item);
    }
    
    // reset the session
    history->items_curr = NULL;
    history->session = NULL;
    history->sess_curr = NULL;
    history->sess_new = NULL;
}

char * p_history_previous(PHistory history, char *item)
{
    if (history->items == NULL) {
        return item;
    }

    if (history->session == NULL) {
        history->session = g_list_copy(history->items);
        history->sess_curr = g_list_last(history->session);
    } else {
        history->sess_curr = g_list_previous(history->sess_curr);
    }

    // set to first if rolled over beginning
    if (history->sess_curr == NULL) {
        history->sess_curr = g_list_first(history->session);
    }

    char *curr = history->sess_curr->data;
    char *result = malloc((strlen(curr) + 1) * sizeof(char));
    strcpy(result, curr);

    return result;
}

char * p_history_next(PHistory history, char *item)
{
    if (history->session == NULL) {
        return NULL;
    } else {
        history->sess_curr = g_list_next(history->sess_curr);
    }

    // set to last if rolled over end
    if (history->sess_curr == NULL) {
        history->sess_curr = g_list_last(history->session);
    }

    char *curr = history->sess_curr->data;
    char *result = malloc((strlen(curr) + 1) * sizeof(char));
    strcpy(result, curr);

    return result;
}
