/*
 * bookmark_ignore.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2020 Michael Vetter <jubalh@iodoru.org>
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
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
 */

#include "config.h"

#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include "config/files.h"
#include "config/preferences.h"

#include "log.h"

static GKeyFile* bookmark_ignore_keyfile = NULL;
static char* account_jid = NULL;

static void
_bookmark_ignore_load()
{
    char* bi_loc;

    bi_loc = files_get_data_path(FILE_BOOKMARK_AUTOJOIN_IGNORE);

    if (g_file_test(bi_loc, G_FILE_TEST_EXISTS)) {
        g_chmod(bi_loc, S_IRUSR | S_IWUSR);
    }

    bookmark_ignore_keyfile = g_key_file_new();
    g_key_file_load_from_file(bookmark_ignore_keyfile, bi_loc, G_KEY_FILE_KEEP_COMMENTS, NULL);

    free(bi_loc);
}

static void
_bookmark_save()
{
    gsize g_data_size;
    gchar* g_bookmark_ignore_data = g_key_file_to_data(bookmark_ignore_keyfile, &g_data_size, NULL);

    char* bi_loc;
    bi_loc = files_get_data_path(FILE_BOOKMARK_AUTOJOIN_IGNORE);

    g_file_set_contents(bi_loc, g_bookmark_ignore_data, g_data_size, NULL);
    g_chmod(bi_loc, S_IRUSR | S_IWUSR);

    free(bi_loc);
    g_free(g_bookmark_ignore_data);
}

void
bookmark_ignore_on_connect(const char* const barejid)
{
    if (bookmark_ignore_keyfile == NULL) {
        _bookmark_ignore_load();
        account_jid = strdup(barejid);
    }
}

void
bookmark_ignore_on_disconnect()
{
    g_key_file_free(bookmark_ignore_keyfile);
    bookmark_ignore_keyfile = NULL;
    free(account_jid);
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
