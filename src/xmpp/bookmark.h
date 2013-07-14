
#ifndef BOOKMARK_H
#define BOOKMARK_H

#include <glib.h>

struct bookmark_t {
    char *jid;
    char *nick;
    gboolean autojoin;
};

typedef struct bookmark_t Bookmark;

void bookmark_request(void);
void bookmark_add(const char *jid, const char *nick, gboolean autojoin);
void bookmark_remove(const char *jid, gboolean autojoin);
const GList *bookmark_get_list(void);
char *bookmark_find(char *search_str);
void bookmark_autocomplete_reset(void);

#endif
