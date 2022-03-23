/*
 * http_download.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2020 William Wennerström <william@wstrm.dev>
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
#include "tools/http_download.h"
#include "config/cafile.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "common.h"

GSList* download_processes = NULL;

static int
_xferinfo(void* userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    HTTPDownload* download = (HTTPDownload*)userdata;

    pthread_mutex_lock(&lock);

    if (download->cancel) {
        pthread_mutex_unlock(&lock);
        return 1;
    }

    if (download->bytes_received == dlnow) {
        pthread_mutex_unlock(&lock);
        return 0;
    } else {
        download->bytes_received = dlnow;
    }

    unsigned int dlperc = 0;
    if (dltotal != 0) {
        dlperc = (100 * dlnow) / dltotal;
    }

    http_print_transfer_update(download->window, download->url,
                               "Downloading '%s': %d%%", download->url, dlperc);

    pthread_mutex_unlock(&lock);

    return 0;
}

#if LIBCURL_VERSION_NUM < 0x072000
static int
_older_progress(void* p, double dltotal, double dlnow, double ultotal, double ulnow)
{
    return _xferinfo(p, (curl_off_t)dltotal, (curl_off_t)dlnow, (curl_off_t)ultotal, (curl_off_t)ulnow);
}
#endif

void*
http_file_get(void* userdata)
{
    HTTPDownload* download = (HTTPDownload*)userdata;

    char* err = NULL;

    CURL* curl;
    CURLcode res;

    download->cancel = 0;
    download->bytes_received = 0;

    pthread_mutex_lock(&lock);
    http_print_transfer(download->window, download->url,
                        "Downloading '%s': 0%%", download->url);

    FILE* outfh = fopen(download->filename, "wb");
    if (outfh == NULL) {
        http_print_transfer_update(download->window, download->url,
                                   "Downloading '%s' failed: Unable to open "
                                   "output file at '%s' for writing (%s).",
                                   download->url, download->filename,
                                   g_strerror(errno));
        goto out;
    }

    char* cert_path = prefs_get_string(PREF_TLS_CERTPATH);
    gchar* cafile = cafile_get_name();
    ProfAccount* account = accounts_get_account(session_get_account_name());
    gboolean insecure = strcmp(account->tls_policy, "trust") == 0;
    account_free(account);
    pthread_mutex_unlock(&lock);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, download->url);

#if LIBCURL_VERSION_NUM >= 0x072000
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, _xferinfo);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, download);
#else
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, _older_progress);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, download);
#endif
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)outfh);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "profanity");

    if (cafile) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, cafile);
    }
    if (cert_path) {
        curl_easy_setopt(curl, CURLOPT_CAPATH, cert_path);
    }
    if (insecure) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }

    if ((res = curl_easy_perform(curl)) != CURLE_OK) {
        err = strdup(curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    if (fclose(outfh) == EOF) {
        err = strdup(g_strerror(errno));
    }

    pthread_mutex_lock(&lock);
    g_free(cafile);
    g_free(cert_path);
    if (err) {
        if (download->cancel) {
            http_print_transfer_update(download->window, download->url,
                                       "Downloading '%s' failed: "
                                       "Download was canceled",
                                       download->url);
        } else {
            http_print_transfer_update(download->window, download->url,
                                       "Downloading '%s' failed: %s",
                                       download->url, err);
        }
        free(err);
    } else {
        if (!download->cancel) {
            http_print_transfer_update(download->window, download->url,
                                       "Downloading '%s': done",
                                       download->url);
            win_mark_received(download->window, download->url);
        }
    }

    if (download->cmd_template != NULL) {
        gchar** argv = format_call_external_argv(download->cmd_template,
                                                 download->url,
                                                 download->filename);

        // TODO: Log the error.
        if (!call_external(argv, NULL, NULL)) {
            http_print_transfer_update(download->window, download->url,
                                       "Downloading '%s' failed: Unable to call "
                                       "command '%s' with file at '%s' (%s).",
                                       download->url,
                                       download->cmd_template,
                                       download->filename,
                                       "TODO: Log the error");
        }

        g_strfreev(argv);
        free(download->cmd_template);
    }

out:

    download_processes = g_slist_remove(download_processes, download);
    pthread_mutex_unlock(&lock);

    free(download->url);
    free(download->filename);
    free(download);

    return NULL;
}

void
http_download_cancel_processes(ProfWin* window)
{
    GSList* download_process = download_processes;
    while (download_process) {
        HTTPDownload* download = download_process->data;
        if (download->window == window) {
            download->cancel = 1;
            break;
        }
        download_process = g_slist_next(download_process);
    }
}

void
http_download_add_download(HTTPDownload* download)
{
    download_processes = g_slist_append(download_processes, download);
}
