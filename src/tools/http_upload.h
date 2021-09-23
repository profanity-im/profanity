/*
 * http_upload.h
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

#ifndef TOOLS_HTTP_UPLOAD_H
#define TOOLS_HTTP_UPLOAD_H

#ifdef PLATFORM_CYGWIN
#define SOCKET int
#endif

#include <sys/select.h>
#include <curl/curl.h>

#include "ui/win_types.h"

typedef enum {
#ifdef HAVE_OMEMO
    AESGCM_UPLOAD,
#endif
    HTTP_UPLOAD,
} http_uploader_type_t;

typedef struct http_uploader_t
{
    http_uploader_type_t type;

    char* filename;
    char* mime_type;
    char* get_url;
    char* put_url;

    // Optional headers (NULL if they shouldn't be send in the PUT).
    char* authorization;
    char* cookie;
    char* expires;

    off_t filesize;
    curl_off_t bytes_sent;

    ProfWin* window;
    pthread_t worker;

    int cancel;
} HTTPUploader;

typedef struct http_upload_t
{
    HTTPUploader* uploader;
} HTTPUpload;

#ifdef HAVE_OMEMO
typedef struct aesgcm_upload_t
{
    HTTPUploader* uploader;
} AESGCMUpload;
#endif

void* http_uploader_start(void* userdata);
void http_uploader_cancel_processes(ProfWin* window);
void http_uploader_add(HTTPUploader* uploader);

off_t file_size(const char* const filename);
char* file_mime_type(const char* const filename);

#endif
