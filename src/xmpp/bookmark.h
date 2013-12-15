
#ifndef XMPP_BOOKMARK_H
#define XMPP_BOOKMARK_H

#include <glib.h>

struct bookmark_t {
    char *jid;
    char *nick;
    gboolean autojoin;
};

typedef struct bookmark_t Bookmark;

void bookmark_request(void);

#endif
