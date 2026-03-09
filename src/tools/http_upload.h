/*
 * http_upload.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef TOOLS_HTTP_UPLOAD_H
#define TOOLS_HTTP_UPLOAD_H

#ifdef PLATFORM_CYGWIN
#define SOCKET int
#endif

#include <sys/select.h>
#include <curl/curl.h>

#include "ui/win_types.h"

typedef struct http_upload_t
{
    char* filename;
    FILE* filehandle;
    off_t filesize;
    curl_off_t bytes_sent;
    char* mime_type;
    char* get_url;
    char* put_url;
    char* alt_scheme;
    char* alt_fragment;
    ProfWin* window;
    pthread_t worker;
    int cancel;
    // Additional headers
    // (NULL if they shouldn't be send in the PUT)
    char* authorization;
    char* cookie;
    char* expires;
} HTTPUpload;

void* http_file_put(void* userdata);

char* file_mime_type(const char* const filename);
off_t file_size(int filedes);

void http_upload_cancel_processes(ProfWin* window);
void http_upload_add_upload(HTTPUpload* upload);

#endif
