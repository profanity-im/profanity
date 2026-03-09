/*
 * plugin_download.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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
