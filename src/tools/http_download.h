/*
 * http_download.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2020 William Wennerström <william@wstrm.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef TOOLS_HTTP_DOWNLOAD_H
#define TOOLS_HTTP_DOWNLOAD_H

#ifdef PLATFORM_CYGWIN
#define SOCKET int
#endif

#include <sys/select.h>
#include <curl/curl.h>

#include "ui/win_types.h"
#include "tools/http_common.h"

typedef struct http_download_t
{
    char* id;
    char* url;
    char* filename;
    char* cmd_template;
    curl_off_t bytes_received;
    ProfWin* window;
    pthread_t worker;
    int cancel;
    gboolean silent;
    gboolean return_bytes_received;
} HTTPDownload;

void* http_file_get(void* userdata);

void http_download_cancel_processes(ProfWin* window);
void http_download_add_download(HTTPDownload* download);

#endif
