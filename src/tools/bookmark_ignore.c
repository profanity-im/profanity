/*
 * bookmark_ignore.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2020 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include "config/files.h"
#include "config/preferences.h"

#include "log.h"

#include "xmpp/xmpp.h"

static prof_keyfile_t bookmark_ignore_prof_keyfile;
static GKeyFile* bookmark_ignore_keyfile = NULL;
static gchar* account_jid = NULL;

static void
_bookmark_ignore_load()
{
    load_data_keyfile(&bookmark_ignore_prof_keyfile, FILE_BOOKMARK_AUTOJOIN_IGNORE);
    bookmark_ignore_keyfile = bookmark_ignore_prof_keyfile.keyfile;
}

static void
_bookmark_save()
{
    save_keyfile(&bookmark_ignore_prof_keyfile);
}

void
bookmark_ignore_on_connect(const char* const barejid)
{
    if (bookmark_ignore_keyfile == NULL) {
        _bookmark_ignore_load();
        account_jid = g_strdup(barejid);
    }
}

void
bookmark_ignore_on_disconnect(void)
{
    free_keyfile(&bookmark_ignore_prof_keyfile);
    bookmark_ignore_keyfile = NULL;
    g_free(account_jid);
    account_jid = NULL;
}

gboolean
bookmark_ignored(Bookmark* bookmark)
{
    return g_key_file_get_boolean(bookmark_ignore_keyfile, account_jid, bookmark->barejid, NULL);
}

gchar**
bookmark_ignore_list(gsize* len)
{
    return g_key_file_get_keys(bookmark_ignore_keyfile, account_jid, len, NULL);
}

void
bookmark_ignore_add(const char* const barejid)
{
    g_key_file_set_boolean(bookmark_ignore_keyfile, account_jid, barejid, TRUE);
    _bookmark_save();
}

void
bookmark_ignore_remove(const char* const barejid)
{
    g_key_file_remove_key(bookmark_ignore_keyfile, account_jid, barejid, NULL);
    _bookmark_save();
}
