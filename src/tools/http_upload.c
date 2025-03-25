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
#include "config/cafile.h"
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

GSList* upload_processes = NULL;

static int
_xferinfo(void* userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    HTTPUpload* upload = (HTTPUpload*)userdata;

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

int
format_alt_url(char* original_url, char* new_scheme, char* new_fragment, char** new_url)
{
    int ret = 0;
    CURLU* h = curl_url();

    if ((ret = curl_url_set(h, CURLUPART_URL, original_url, 0)) != 0) {
        goto out;
    }

    if (new_scheme != NULL) {
        if ((ret = curl_url_set(h, CURLUPART_SCHEME, new_scheme, CURLU_NON_SUPPORT_SCHEME)) != 0) {
            goto out;
        }
    }

    if (new_fragment != NULL) {
        if ((ret = curl_url_set(h, CURLUPART_FRAGMENT, new_fragment, 0)) != 0) {
            goto out;
        }
    }

    ret = curl_url_get(h, CURLUPART_URL, new_url, 0);

out:
    curl_url_cleanup(h);
    return ret;
}

void*
http_file_put(void* userdata)
{
    HTTPUpload* upload = (HTTPUpload*)userdata;

    FILE* fh = NULL;

    auto_char char* err = NULL;

    CURL* curl;
    CURLcode res;

    upload->cancel = 0;
    upload->bytes_sent = 0;

    pthread_mutex_lock(&lock);
    auto_gchar gchar* msg = g_strdup_printf("Uploading '%s': 0%%", upload->filename);
    if (!msg) {
        msg = g_strdup(FALLBACK_MSG);
    }
    win_print_http_transfer(upload->window, msg, upload->put_url);

    auto_gchar gchar* cert_path = prefs_get_string(PREF_TLS_CERTPATH);
    auto_gchar gchar* cafile = cafile_get_name();
    gboolean insecure = FALSE;
    ProfAccount* account = accounts_get_account(session_get_account_name());
    if (account) {
        insecure = account->tls_policy && strcmp(account->tls_policy, "trust") == 0;
    }
    account_free(account);
    pthread_mutex_unlock(&lock);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, upload->put_url);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

    struct curl_slist* headers = NULL;
    auto_gchar gchar* content_type_header = g_strdup_printf("Content-Type: %s", upload->mime_type);
    if (!content_type_header) {
        content_type_header = g_strdup(FALLBACK_CONTENTTYPE_HEADER);
    }
    headers = curl_slist_append(headers, content_type_header);
    headers = curl_slist_append(headers, "Expect:");

    // Optional headers
    auto_gchar gchar* auth_header = NULL;
    if (upload->authorization) {
        auth_header = g_strdup_printf("Authorization: %s", upload->authorization);
        if (!auth_header) {
            auth_header = g_strdup(FALLBACK_MSG);
        }
        headers = curl_slist_append(headers, auth_header);
    }
    auto_gchar gchar* cookie_header = NULL;
    if (upload->cookie) {
        cookie_header = g_strdup_printf("Cookie: %s", upload->cookie);
        if (!cookie_header) {
            cookie_header = g_strdup(FALLBACK_MSG);
        }
        headers = curl_slist_append(headers, cookie_header);
    }
    auto_gchar gchar* expires_header = NULL;
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
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&output);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "profanity");

    fh = upload->filehandle;

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

    curl_easy_setopt(curl, CURLOPT_READDATA, fh);
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
            err = g_strdup_printf("Server returned %lu", http_code);
        }

#if 0
        printf("HTTP Status: %lu\n", http_code);
        printf("%s\n", output.buffer);
        printf("%lu bytes retrieved\n", (long)output.size);
#endif
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    curl_slist_free_all(headers);

    if (fh) {
        fclose(fh);
    }
    free(output.buffer);

    pthread_mutex_lock(&lock);

    if (err) {
        auto_gchar gchar* err_msg = NULL;
        if (upload->cancel) {
            err_msg = g_strdup_printf("Uploading '%s' failed: Upload was canceled", upload->filename);
            if (!err_msg) {
                err_msg = g_strdup(FALLBACK_MSG);
            }
        } else {
            err_msg = g_strdup_printf("Uploading '%s' failed: %s", upload->filename, err);
            if (!err_msg) {
                err_msg = g_strdup(FALLBACK_MSG);
            }
            win_update_entry_message(upload->window, upload->put_url, err_msg);
        }
        cons_show_error(err_msg);
    } else {
        if (!upload->cancel) {
            auto_gchar gchar* status_msg = g_strdup_printf("Uploading '%s': 100%%", upload->filename);
            if (!status_msg) {
                status_msg = g_strdup(FALLBACK_MSG);
            }
            win_update_entry_message(upload->window, upload->put_url, status_msg);
            win_mark_received(upload->window, upload->put_url);

            char* url = NULL;
            if (format_alt_url(upload->get_url, upload->alt_scheme, upload->alt_fragment, &url) != 0) {
                auto_gchar gchar* fail_msg = g_strdup_printf("Uploading '%s' failed: Bad URL ('%s')", upload->filename, upload->get_url);
                if (!fail_msg) {
                    fail_msg = g_strdup(FALLBACK_MSG);
                }
                cons_show_error(fail_msg);
            } else {
                switch (upload->window->type) {
                case WIN_CHAT:
                {
                    ProfChatWin* chatwin = (ProfChatWin*)(upload->window);
                    assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
                    cl_ev_send_msg(chatwin, url, url);
                    break;
                }
                case WIN_PRIVATE:
                {
                    ProfPrivateWin* privatewin = (ProfPrivateWin*)(upload->window);
                    assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
                    cl_ev_send_priv_msg(privatewin, url, url);
                    break;
                }
                case WIN_MUC:
                {
                    ProfMucWin* mucwin = (ProfMucWin*)(upload->window);
                    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
                    cl_ev_send_muc_msg(mucwin, url, url);
                    break;
                }
                default:
                    break;
                }

                curl_free(url);
            }
        }
    }

    upload_processes = g_slist_remove(upload_processes, upload);
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

char*
file_mime_type(const char* const filename)
{
    char* out_mime_type;
    char file_header[FILE_HEADER_BYTES];
    FILE* fh;
    if (!(fh = fopen(filename, "rb"))) {
        return strdup(FALLBACK_MIMETYPE);
    }
    size_t file_header_size = fread(file_header, 1, FILE_HEADER_BYTES, fh);
    fclose(fh);

    auto_gchar gchar* content_type = g_content_type_guess(filename, (unsigned char*)file_header, file_header_size, NULL);
    if (content_type != NULL) {
        auto_gchar gchar* mime_type = g_content_type_get_mime_type(content_type);
        out_mime_type = strdup(mime_type);
    } else {
        return strdup(FALLBACK_MIMETYPE);
    }
    return out_mime_type;
}

off_t
file_size(int filedes)
{
    struct stat st;
    fstat(filedes, &st);
    return st.st_size;
}

void
http_upload_cancel_processes(ProfWin* window)
{
    GSList* upload_process = upload_processes;
    while (upload_process) {
        HTTPUpload* upload = upload_process->data;
        if (upload->window == window) {
            upload->cancel = 1;
            break;
        }
        upload_process = g_slist_next(upload_process);
    }
}

void
http_upload_add_upload(HTTPUpload* upload)
{
    upload_processes = g_slist_append(upload_processes, upload);
}
