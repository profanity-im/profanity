/*
 * plugin_download.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2024 Michael Vetter <jubalh@iodoru.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <curl/curl.h>
#include <gio/gio.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

#include "profanity.h"
#include "event/client_events.h"
#include "tools/http_common.h"
#include "tools/plugin_download.h"
#include "config/preferences.h"
#include "plugins/plugins.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "common.h"

void*
plugin_download_install(void* userdata)
{
    HTTPDownload* plugin_dl = (HTTPDownload*)userdata;

    /* We need to dup those, because they're free'd inside `http_file_get()` */
    auto_char char* path = strdup(plugin_dl->filename);
    auto_char char* https_url = strdup(plugin_dl->url);

    http_file_get(plugin_dl);

    if (is_regular_file(path)) {
        GString* error_message = g_string_new(NULL);
        auto_char char* plugin_name = basename_from_url(https_url);
        gboolean result = plugins_install(plugin_name, path, error_message);
        if (result) {
            cons_show("Plugin installed and loaded: %s", plugin_name);
        } else {
            cons_show("Failed to install plugin: %s. %s", plugin_name, error_message->str);
        }
        g_string_free(error_message, TRUE);
    } else {
        cons_show_error("Downloaded file is not a file (?)");
    }

    remove(path);

    return NULL;
}

void
plugin_download_add_download(HTTPDownload* plugin_dl)
{
    http_download_add_download(plugin_dl);
}
