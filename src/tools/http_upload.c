/*
 * http_upload.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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

#define _GNU_SOURCE 1

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

#include "profanity.h"
#include "event/client_events.h"
#include "tools/http_upload.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "common.h"

#define FALLBACK_MIMETYPE "application/octet-stream"
#define FALLBACK_CONTENTTYPE_HEADER "Content-Type: application/octet-stream"
#define FALLBACK_MSG ""
#define FILE_HEADER_BYTES 512

struct curl_data_t {
    char *buffer;
    size_t size;
};


static int
_xferinfo(void *userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    HTTPUpload *upload = (HTTPUpload *)userdata;

    pthread_mutex_lock(&lock);

    if (upload->cancel) {
        pthread_mutex_unlock(&lock);
        return 1;
    }

    if (upload->bytes_sent == ulnow) {
        pthread_mutex_unlock(&lock);
        return 0;
    } else {
        upload->bytes_sent = ulnow;
    }

    unsigned int ulperc = 0;
    if (ultotal != 0) {
        ulperc = (100 * ulnow) / ultotal;
    }

    char *msg;
    if (asprintf(&msg, "Uploading '%s': %d%%", upload->filename, ulperc) == -1) {
        msg = strdup(FALLBACK_MSG);
    }
    win_update_entry_message(upload->window, upload->put_url, msg);
    free(msg);

    pthread_mutex_unlock(&lock);

    return 0;
}

#if LIBCURL_VERSION_NUM < 0x072000
static int
_older_progress(void *p, double dltotal, double dlnow, double ultotal, double ulnow)
{
    return _xferinfo(p, (curl_off_t)dltotal, (curl_off_t)dlnow, (curl_off_t)ultotal, (curl_off_t)ulnow);
}
#endif

static size_t
_data_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    struct curl_data_t *mem = (struct curl_data_t *) data;
    mem->buffer = realloc(mem->buffer, mem->size + realsize + 1);

    if (mem->buffer)
    {
        memcpy( &( mem->buffer[ mem->size ] ), ptr, realsize );
        mem->size += realsize;
        mem->buffer[ mem->size ] = 0;
    }

    return realsize;
}

void *
http_file_put(void *userdata)
{
    HTTPUpload *upload = (HTTPUpload *)userdata;

    FILE *fd = NULL;

    char *err = NULL;
    char *content_type_header;

    CURL *curl;
    CURLcode res;

    upload->cancel = 0;
    upload->bytes_sent = 0;

    pthread_mutex_lock(&lock);
    char* msg;
    if (asprintf(&msg, "Uploading '%s': 0%%", upload->filename) == -1) {
        msg = strdup(FALLBACK_MSG);
    }
    win_print_http_upload(upload->window, msg, upload->put_url);
    free(msg);

    char *cert_path = prefs_get_string(PREF_TLS_CERTPATH);
    pthread_mutex_unlock(&lock);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, upload->put_url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

    struct curl_slist *headers = NULL;
    if (asprintf(&content_type_header, "Content-Type: %s", upload->mime_type) == -1) {
        content_type_header = strdup(FALLBACK_CONTENTTYPE_HEADER);
    }
    headers = curl_slist_append(headers, content_type_header);
    headers = curl_slist_append(headers, "Expect:");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    #if LIBCURL_VERSION_NUM >= 0x072000
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, _xferinfo);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, upload);
    #else
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, _older_progress);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, upload);
    #endif
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    struct curl_data_t output;
    output.buffer = NULL;
    output.size = 0;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _data_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&output);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "profanity");

    if (!(fd = fopen(upload->filename, "rb"))) {
        if (asprintf(&err, "failed to open '%s'", upload->filename) == -1) {
            err = NULL;
        }
        goto end;
    }

    if (cert_path) {
        curl_easy_setopt(curl, CURLOPT_CAPATH, cert_path);
    }

    curl_easy_setopt(curl, CURLOPT_READDATA, fd);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)(upload->filesize));
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    if ((res = curl_easy_perform(curl)) != CURLE_OK) {
        err = strdup(curl_easy_strerror(res));
    } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (output.buffer) {
            output.buffer[output.size++] = '\0';
        }

        // XEP-0363 specifies 201 but prosody returns 200
        if (http_code != 200 && http_code != 201) {
            if (asprintf(&err, "Server returned %lu", http_code) == -1) {
                err = NULL;
            }
        }

        #if 0
        printf("HTTP Status: %lu\n", http_code);
        printf("%s\n", output.buffer);
        printf("%lu bytes retrieved\n", (long)output.size);
        #endif
    }

end:
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    curl_slist_free_all(headers);
    if (fd) {
        fclose(fd);
    }
    free(content_type_header);
    free(output.buffer);

    pthread_mutex_lock(&lock);
    prefs_free_string(cert_path);

    if (err) {
        char *msg;
        if (upload->cancel) {
            if (asprintf(&msg, "Uploading '%s' failed: Upload was canceled", upload->filename) == -1) {
                msg = strdup(FALLBACK_MSG);
            }
        } else {
            if (asprintf(&msg, "Uploading '%s' failed: %s", upload->filename, err) == -1) {
                msg = strdup(FALLBACK_MSG);
            }
            win_update_entry_message(upload->window, upload->put_url, msg);
        }
        cons_show_error(msg);
        free(msg);
        free(err);
    } else {
        if (!upload->cancel) {
            if (asprintf(&msg, "Uploading '%s': 100%%", upload->filename) == -1) {
                msg = strdup(FALLBACK_MSG);
            }
            win_update_entry_message(upload->window, upload->put_url, msg);
            win_mark_received(upload->window, upload->put_url);
            free(msg);

            switch (upload->window->type) {
            case WIN_CHAT:
            {
                ProfChatWin *chatwin = (ProfChatWin*)(upload->window);
                assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
                cl_ev_send_msg(chatwin, upload->get_url, upload->get_url);
                break;
            }
            case WIN_PRIVATE:
            {
                ProfPrivateWin *privatewin = (ProfPrivateWin*)(upload->window);
                assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
                cl_ev_send_priv_msg(privatewin, upload->get_url, upload->get_url);
                break;
            }
            case WIN_MUC:
            {
                ProfMucWin *mucwin = (ProfMucWin*)(upload->window);
                assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
                cl_ev_send_muc_msg(mucwin, upload->get_url, upload->get_url);
                break;
            }
            default:
                break;
            }
        }
    }

    upload_processes = g_slist_remove(upload_processes, upload);
    pthread_mutex_unlock(&lock);

    free(upload->filename);
    free(upload->mime_type);
    free(upload->get_url);
    free(upload->put_url);
    free(upload);

    return NULL;
}

char*
file_mime_type(const char* const file_name)
{
    char *out_mime_type;
    char file_header[FILE_HEADER_BYTES];
    FILE *fd;
    if (!(fd = fopen(file_name, "rb"))) {
        return strdup(FALLBACK_MIMETYPE);
    }
    size_t file_header_size = fread(file_header, 1, FILE_HEADER_BYTES, fd);
    fclose(fd);

    char *content_type = g_content_type_guess(file_name, (unsigned char*)file_header, file_header_size, NULL);
    if (content_type != NULL) {
        char *mime_type = g_content_type_get_mime_type(content_type);
        out_mime_type = strdup(mime_type);
        g_free(mime_type);
    }
    else {
        return strdup(FALLBACK_MIMETYPE);
    }
    g_free(content_type);
    return out_mime_type;
}

off_t file_size(const char* const filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}
