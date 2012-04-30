#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "prof_history.h"

struct p_history_t {
    GList *items; // the history
    GList *session; // a copy of the history for edits
    GList *session_current; // pointer to the current item in the session
    guint max_size;
};

PHistory p_history_new(unsigned int size)
{
    PHistory new_history = malloc(sizeof(struct p_history_t));
    new_history->items = NULL;
    new_history->session = NULL;
    new_history->max_size = size;

    return new_history;
}

void p_history_append(PHistory history, char *item)
{
    char *copied = strdup(item);
    // if already at max size
    if (g_list_length(history->items) == history->max_size) {

        // remove first element
        GList *first = g_list_first(history->items);
        const char *first_item = (char *) first->data;
        history->items = g_list_remove(history->items, first_item);
    }
    
    // append the new item onto the history
    history->items = g_list_append(history->items, copied);

    // delete the current session
    if (history->session != NULL) {
        g_list_free(history->session);
        history->session = NULL;
    }
}

char * p_history_previous(PHistory history)
{
    if (history->items == NULL) {
        return NULL;
    }

    if (history->session == NULL) {
        history->session = g_list_copy(history->items);
        history->session_current = g_list_last(history->session);
    } else {
        history->session_current = g_list_previous(history->session_current);
    }

    // set to first if rolled over beginning
    if (history->session_current == NULL) {
        history->session_current = g_list_first(history->session);
    }

    char *item = (char *) history->session_current->data;
    char *result = malloc((strlen(item) + 1) * sizeof(char));
    strcpy(result, item);

    return result;
}

char * p_history_next(PHistory history)
{
    if (history->session == NULL) {
        return NULL;
    } else {
        history->session_current = g_list_next(history->session_current);
    }

    // set to last if rolled over end
    if (history->session_current == NULL) {
        history->session_current = g_list_last(history->session);
    }

    char *item = (char *) history->session_current->data;
    char *result = malloc((strlen(item) + 1) * sizeof(char));
    strcpy(result, item);

    return result;
}
