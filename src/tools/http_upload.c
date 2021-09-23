/*
 * http_upload.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
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

#define FALLBACK_MIMETYPE           "application/octet-stream"
#define FALLBACK_CONTENTTYPE_HEADER "Content-Type: application/octet-stream"
#define FALLBACK_MSG                ""
#define FILE_HEADER_BYTES           512

struct curl_data_t
{
    char* buffer;
    size_t size;
};

GSList* http_uploader_processes = NULL;

static int
_xferinfo(void* userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    HTTPUploader* uploader = (HTTPUploader*)userdata;

    pthread_mutex_lock(&lock);

    if (uploader->cancel) {
        pthread_mutex_unlock(&lock);
        return 1;
    }

    if (uploader->bytes_sent == ulnow) {
        pthread_mutex_unlock(&lock);
        return 0;
    } else {
        uploader->bytes_sent = ulnow;
    }

    unsigned int ulperc = 0;
    if (ultotal != 0) {
        ulperc = (100 * ulnow) / ultotal;
    }

    gchar* msg = g_strdup_printf("Uploading '%s': %d%%", upload->filename, ulperc);
    if (!msg) {
        msg = g_strdup(FALLBACK_MSG);
    }
    win_update_entry_message(upload->window, upload->put_url, msg);
    g_free(msg);

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

static size_t
_data_callback(void* ptr, size_t size, size_t nmemb, void* data)
{
    size_t realsize = size * nmemb;
    struct curl_data_t* mem = (struct curl_data_t*)data;
    mem->buffer = realloc(mem->buffer, mem->size + realsize + 1);

    if (mem->buffer) {
        memcpy(&(mem->buffer[mem->size]), ptr, realsize);
        mem->size += realsize;
        mem->buffer[mem->size] = 0;
    }

    return realsize;
}

void* http_uploader_start(void* userdata) {
    HTTPUploader* uploader = (HTTPUploader*)userdata;
    switch(uploader->type) {
        case AESGCM_UPLOAD: return aesgcm_upload_start(uploader);
        case HTTP_UPLOAD: return http_upload_start(uploader);
    }
}

void*
http_uploader_put(HTTPUploader* uploader)
{
    FILE* fd = NULL;

    char* err = NULL;
    gchar* content_type_header;

    // Optional headers.
    gchar* auth_header = NULL;
    gchar* cookie_header = NULL;
    gchar* expires_header = NULL;

    CURL* curl;
    CURLcode res;

    uploader->cancel = 0;
    uploader->bytes_sent = 0;

    pthread_mutex_lock(&lock);

    gchar* msg = g_strdup_printf( "Uploading '%s': 0%%", uploader->filename);
    if (!msg) {
        msg = g_strdup(FALLBACK_MSG);
    }
    win_print_http_upload(uploader->window, msg, uploader->put_url);
    g_free(msg);

    char* cert_path = prefs_get_string(PREF_TLS_CERTPATH);
    pthread_mutex_unlock(&lock);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, uploader->put_url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

    struct curl_slist* headers = NULL;
    content_type_header = g_strdup_printf("Content-Type: %s", uploader->mime_type);
    if (!content_type_header) {
        content_type_header = g_strdup(FALLBACK_CONTENTTYPE_HEADER);
    }
    headers = curl_slist_append(headers, content_type_header);
    headers = curl_slist_append(headers, "Expect:");

    // Optional headers.
    if (upload->authorization) {
        auth_header = g_strdup_printf("Authorization: %s", upload->authorization);
        if (!auth_header) {
            auth_header = g_strdup(FALLBACK_MSG);
        }
        headers = curl_slist_append(headers, auth_header);
    }
    if (upload->cookie) {
        cookie_header = g_strdup_printf("Cookie: %s", upload->cookie);
        if (!cookie_header) {
            cookie_header = g_strdup(FALLBACK_MSG);
        }
        headers = curl_slist_append(headers, cookie_header);
    }
    if (upload->expires) {
        expires_header = g_strdup_printf("Expires: %s", upload->expires);
        if (!expires_header) {
            expires_header = g_strdup(FALLBACK_MSG);
        }
        headers = curl_slist_append(headers, expires_header);
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

#if LIBCURL_VERSION_NUM >= 0x072000
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, _xferinfo);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, uploader);
#else
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, _older_progress);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, uploader);
#endif
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    struct curl_data_t output;
    output.buffer = NULL;
    output.size = 0;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _data_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&output);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "profanity");

    if (!(fd = fopen(uploader->filename, "rb"))) {
        err = g_strdup_printf("failed to open '%s'", uploader->filename);
        goto end;
    }

    if (cert_path) {
        curl_easy_setopt(curl, CURLOPT_CAPATH, cert_path);
    }

    curl_easy_setopt(curl, CURLOPT_READDATA, fd);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)(uploader->filesize)));
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
            err = g_strdup_printf("Server returned %lu", http_code);
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
    free(output.buffer);

    g_free(content_type_header);
    g_free(auth_header);
    g_free(cookie_header);
    g_free(expires_header);

    pthread_mutex_lock(&lock);
    free(cert_path);

    if (err) {
        char* msg;
        if (uploader->cancel) {
            msg = g_strdup_printf("Uploading '%s' failed: Upload was canceled", uploader->filename);
            if (!msg) {
                msg = g_strdup(FALLBACK_MSG);
            }
        } else {
            msg = g_strdup_printf("Uploading '%s' failed: %s", uploader->filename, err);
            if (!msg) {
                msg = g_strdup(FALLBACK_MSG);
            }
            win_update_entry_message(uploader->window, uploader->put_url, msg);
        }
        cons_show_error(msg);
        g_free(msg);
        free(err);
    } else {
        if (!uploader->cancel) {
            msg = g_strdup_printf("Uploading '%s': 100%%", uploader->filename);
            if (!msg) {
                msg = g_strdup(FALLBACK_MSG);
            }
            win_update_entry_message(uploader->window, uploader->put_url, msg);
            win_mark_received(uploader->window, uploader->put_url);
            g_free(msg);

            switch (uploader->window->type) {
            case WIN_CHAT:
            {
                ProfChatWin* chatwin = (ProfChatWin*)(uploader->window);
                assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
                cl_ev_send_msg(chatwin, uploader->get_url, uploader->get_url);
                break;
            }
            case WIN_PRIVATE:
            {
                ProfPrivateWin* privatewin = (ProfPrivateWin*)(uploader->window);
                assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
                cl_ev_send_priv_msg(privatewin, uploader->get_url, uploader->get_url);
                break;
            }
            case WIN_MUC:
            {
                ProfMucWin* mucwin = (ProfMucWin*)(uploader->window);
                assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
                cl_ev_send_muc_msg(mucwin, uploader->get_url, uploader->get_url);
                break;
            }
            default:
                break;
            }
        }
    }

    http_uploader_processes = g_slist_remove(http_uploader_processes, uploader);
    pthread_mutex_unlock(&lock);

    free(upload->filename);
    free(upload->mime_type);
    free(upload->get_url);
    free(upload->put_url);
    free(upload->alt_scheme);
    free(upload->alt_fragment);
    free(upload->authorization);
    free(upload->cookie);
    free(upload->expires);
    free(upload);

    return NULL;
}


void*
http_upload_start(Uploader* uploader)
{
    http_uploader_put(uploader);
}

char*
file_mime_type(const char* const filename)
{
    char* out_mime_type;
    char file_header[FILE_HEADER_BYTES];
    FILE* fd;
    if (!(fd = fopen(filename, "rb"))) {
        return strdup(FALLBACK_MIMETYPE);
    }
    size_t file_header_size = fread(file_header, 1, FILE_HEADER_BYTES, fd);
    fclose(fd);

    char* content_type = g_content_type_guess(filename, (unsigned char*)file_header, file_header_size, NULL);
    if (content_type != NULL) {
        char* mime_type = g_content_type_get_mime_type(content_type);
        out_mime_type = strdup(mime_type);
        g_free(mime_type);
    } else {
        return strdup(FALLBACK_MIMETYPE);
    }
    g_free(content_type);
    return out_mime_type;
}

off_t
file_size(const char* const filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}

void
http_uploader_cancel_processes(ProfWin* window)
{
    GSList* process = http_uploader_processes;
    while (process) {
        HTTPUpload* uploader = process->data;
        if (uploader->window == window) {
            uploader->cancel = 1;
            break;
        }
        process = g_slist_next(process);
    }
}

void
http_uploader_add(HTTPUploader* uploader)
{
    http_uploader_processes = g_slist_append(http_uploader_processes, uploader);
}
