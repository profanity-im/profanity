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

ProfBuff*
buffer_create()
{
    ProfBuff* new_buff = malloc(sizeof(struct prof_buff_t));
    new_buff->wrap = 0;
    new_buff->current = 0;
    return new_buff;
}

int
buffer_size(ProfBuff* buffer)
{
    if(buffer->wrap == 1) {
        return BUFF_SIZE;
    } else {
        return buffer->current;
    }
}

void
buffer_free(ProfBuff* buffer)
{
    int i = 0;
    int imax = buffer->current;

    if (buffer->wrap == 1) {
        imax = BUFF_SIZE;
    }

    for (i = 0; i < imax; i++) {
        free(buffer->entry[i].message);
        free(buffer->entry[i].from);
        free(buffer->entry[i].date_fmt);
    }

    free(buffer);
}

void
buffer_push(ProfBuff* buffer, const char show_char, const char * const date_fmt,
    int flags, int attrs, const char * const from, const char * const message)
{
    int *current = &(buffer->current);
    ProfBuffEntry* e = &(buffer->entry[buffer->current]);

    if (buffer->wrap == 1) {
        free(e->message);
        free(e->from);
    }

    e->show_char = show_char;
    e->flags = flags;
    e->attrs = attrs;

    e->date_fmt = malloc(strlen(date_fmt)+1);
    strcpy(e->date_fmt, date_fmt);

    e->from = malloc(strlen(from)+1);
    strcpy(e->from, from);

    e->message = malloc(strlen(message)+1);
    strcpy(e->message, message);

    *current += 1;
    if (*current >= BUFF_SIZE) {
        *current = 0;
        buffer->wrap = 1;
    }
}

ProfBuffEntry
buffer_yield_entry(ProfBuff* buffer, int entry)
{
    return (buffer->entry)[entry];

    if (buffer->wrap == 1) {
        return buffer->entry[(buffer->current + entry) % BUFF_SIZE];
    } else {
        return buffer->entry[entry % (buffer->current)];
    }
}