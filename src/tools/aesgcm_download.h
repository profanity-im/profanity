/*
 * aesgcm_download.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2020 William Wennerström <william@wstrm.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef TOOLS_AESGCM_DOWNLOAD_H
#define TOOLS_AESGCM_DOWNLOAD_H

#ifdef PLATFORM_CYGWIN
#define SOCKET int
#endif

#include <sys/select.h>
#include <curl/curl.h>
#include "tools/http_common.h"
#include "tools/http_download.h"

#include "ui/win_types.h"

typedef struct aesgcm_download_t
{
    gchar* id;
    char* url;
    char* filename;
    char* cmd_template;
    ProfWin* window;
    pthread_t worker;
    HTTPDownload* http_dl;
} AESGCMDownload;

void* aesgcm_file_get(void* userdata);

void aesgcm_download_cancel_processes(ProfWin* window);
void aesgcm_download_add_download(AESGCMDownload* download);

#endif
