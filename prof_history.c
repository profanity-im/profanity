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
    char *copied = "";
    if (item != NULL) {
        copied = strdup(item);
    }

    // if no history yet, just add the item
    if (history->items == NULL) {
        history->items = g_list_append(history->items, copied);
        return;
    }
        
    // if not editing history (no session started)
    if (history->session == NULL) {
        
        // lose first element if hit max size
        if (g_list_length(history->items) == history->max_size) {

            // remove first element
            GList *first = g_list_first(history->items);
            char *first_item = first->data;
            history->items = g_list_remove(history->items, first_item);
        }
    
        // append the new item onto the history
        history->items = g_list_append(history->items, copied);

    // if editing history (session exists with possible changes)
    } else {

        // if adding a new item, copy the session over the history
        if ((history->sess_new != NULL) && 
                (history->sess_curr == history->sess_new)) {
            _replace_history_with_session(history);

        // otherwise, adding edited history item
        } else {
            
            // discard the new item if there was one
            if (history->sess_new != NULL) {
                history->session = g_list_reverse(history->session);
                GList *first = g_list_first(history->session);
                char *first_item = first->data;
                history->session = g_list_remove(history->session, first_item);
                history->session = g_list_reverse(history->session);
            }

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
    // no history
    if (history->items == NULL) {
        return item;
    }

    char *copied = "";
    if (item != NULL) {
        copied = strdup(item);
    }
    
    // no session, create one
    if (history->session == NULL) {
        history->session = g_list_copy(history->items);
        history->sess_curr = g_list_last(history->session);
        history->items_curr = g_list_last(history->items);
        
        // add the new item including empty string
        g_list_append(history->session, copied);
        history->sess_new = g_list_last(history->session);

        char *result = strdup(history->sess_curr->data);
        return result;

    // session exists
    } else {
        // update the currently pointed to item with passed data
        history->sess_curr->data = copied;
        
        // move to previous
        history->sess_curr = g_list_previous(history->sess_curr);
        history->items_curr = g_list_previous(history->items_curr);

        // set to first if rolled over beginning
        if (history->sess_curr == NULL) {
            history->sess_curr = g_list_first(history->session);
            history->items_curr = g_list_first(history->items);
        }
    }

    char *result = strdup(history->sess_curr->data);
    return result;
}

char * p_history_next(PHistory history, char *item)
{

    // no history, or no session, return item
    if ((history->items == NULL) || (history->session == NULL)) {
        return item;
    }

    char *copied = "";
    if (item != NULL) {
        copied = strdup(item);
    }

    // session exists
    
    // update the currently pointed to item with passed data
    history->sess_curr->data = copied;
    
    // move to next
    history->sess_curr = g_list_next(history->sess_curr);
    history->items_curr = g_list_next(history->items_curr);

    // set to last if rolled over beginning
    if (history->sess_curr == NULL) {
        history->sess_curr = g_list_last(history->session);
        history->items_curr = NULL;
    }

    char *result = strdup(history->sess_curr->data);
    return result;
}
