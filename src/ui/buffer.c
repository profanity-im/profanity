#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <glib.h>
#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "ui/window.h"
#include "ui/buffer.h"

#define BUFF_SIZE 1200

struct prof_buff_t {
    GSList *entries;
};

static void _free_entry(ProfBuffEntry *entry);

ProfBuff
buffer_create()
{
    ProfBuff new_buff = malloc(sizeof(struct prof_buff_t));
    new_buff->entries = NULL;
    return new_buff;
}

int
buffer_size(ProfBuff buffer)
{
    return g_slist_length(buffer->entries);
}

void
buffer_free(ProfBuff buffer)
{
    g_slist_free_full(buffer->entries, (GDestroyNotify)_free_entry);
    free(buffer);
    buffer = NULL;
}

void
buffer_push(ProfBuff buffer, const char show_char, const char * const date_fmt,
    int flags, int attrs, const char * const from, const char * const message)
{
    ProfBuffEntry *e = malloc(sizeof(struct prof_buff_entry_t));
    e->show_char = show_char;
    e->flags = flags;
    e->attrs = attrs;

    e->date_fmt = malloc(strlen(date_fmt)+1);
    strcpy(e->date_fmt, date_fmt);

    e->from = malloc(strlen(from)+1);
    strcpy(e->from, from);

    e->message = malloc(strlen(message)+1);
    strcpy(e->message, message);

    if (g_slist_length(buffer->entries) == BUFF_SIZE) {
        _free_entry(buffer->entries->data);
        buffer->entries = g_slist_delete_link(buffer->entries, buffer->entries);
    }

    buffer->entries = g_slist_append(buffer->entries, e);
}

ProfBuffEntry*
buffer_yield_entry(ProfBuff buffer, int entry)
{
    GSList *node = g_slist_nth(buffer->entries, entry);
    return node->data;
}

static void
_free_entry(ProfBuffEntry *entry)
{
    free(entry->message);
    free(entry->from);
    free(entry->date_fmt);
}

