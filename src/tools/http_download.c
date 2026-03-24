/*
 * http_download.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2020 William Wennerström <william@wstrm.dev>
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

    if (!download->silent) {
        const char* url = download->display_url ? download->display_url : download->url;
        if (dlnow == dltotal && dltotal > 0) {
            http_print_transfer_update(download->window, download->id,
                                       "Downloading '%s': done", url);
        } else {
            http_print_transfer_update(download->window, download->id,
                                       "Downloading '%s': %d%%", url, dlperc);
        }
    }

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
    ssize_t* ret = NULL;

    char* err = NULL;

    CURL* curl;
    CURLcode res;

    download->cancel = 0;
    download->bytes_received = 0;

    pthread_mutex_lock(&lock);
    const char* display_url = download->display_url ? download->display_url : download->url;
    if (!download->silent) {
        http_print_transfer(download->window, download->id,
                            "Downloading '%s': 0%%", display_url);
    }

    FILE* outfh = fopen(download->filename, "wb");
    if (outfh == NULL) {
        http_print_transfer_update(download->window, download->id,
                                   "Downloading '%s' failed: Unable to open "
                                   "output file at '%s' for writing (%s).",
                                   download->url, download->filename,
                                   g_strerror(errno));
        goto out;
    }

    gchar* cert_path = prefs_get_string(PREF_TLS_CERTPATH);
    gchar* cafile = cafile_get_name();
    ProfAccount* account = accounts_get_account(session_get_account_name());
    gboolean insecure = FALSE;
    if (account) {
        insecure = account->tls_policy && strcmp(account->tls_policy, "trust") == 0;
    }
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

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

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

    if (!err && ftell(outfh) == 0) {
        err = strdup("Output file is empty.");
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    if (fclose(outfh) == EOF) {
        if (!err) {
            err = strdup(g_strerror(errno));
        }
    }

    pthread_mutex_lock(&lock);
    g_free(cafile);
    g_free(cert_path);
    if (err) {
        if (download->cancel) {
            http_print_transfer_update(download->window, download->id,
                                       "Downloading '%s' failed: "
                                       "Download was canceled",
                                       display_url);
        } else {
            http_print_transfer_update(download->window, download->id,
                                       "Downloading '%s' failed: %s",
                                       display_url, err);
        }
        free(err);
    } else {
        if (!download->cancel && !download->silent && !download->silent_done) {
            http_print_transfer_update(download->window, download->id,
                                       "Downloading '%s': done\nSaved to '%s'",
                                       display_url, download->filename);
            win_mark_received(download->window, download->id);
            if (download->return_bytes_received) {
                ret = malloc(sizeof(*ret));
                if (ret) {
                    *ret = download->bytes_received;
                }
            }
        }
    }

    if (download->cmd_template != NULL) {
        gchar** argv = format_call_external_argv(download->cmd_template,
                                                 download->url,
                                                 download->filename);

        // TODO: Log the error.
        if (!call_external(argv)) {
            http_print_transfer_update(download->window, download->id,
                                       "Downloading '%s' failed: Unable to call "
                                       "command '%s' with file at '%s' (%s).",
                                       display_url,
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

    free(download->filename);
    free(download->url);
    free(download->display_url);
    free(download->id);
    free(download);

    return ret;
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
