/*
 * plugin_download.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef TOOLS_PLUGIN_DOWNLOAD_H
#define TOOLS_PLUGIN_DOWNLOAD_H

#ifdef PLATFORM_CYGWIN
#define SOCKET int
#endif

#include <sys/select.h>
#include <curl/curl.h>
#include "tools/http_common.h"
#include "tools/http_download.h"

#include "ui/win_types.h"

void* plugin_download_install(void* userdata);

void plugin_download_add_download(HTTPDownload* download);

#endif
