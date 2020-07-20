/*
 * aesgcm_download.c
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
#include "tools/aesgcm_download.h"
#include "omemo/omemo.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "common.h"

#define FALLBACK_MSG ""

void*
aesgcm_file_get(void* userdata)
{
    AESGCMDownload* aesgcm_dl = (AESGCMDownload*)userdata;

    char* https_url = NULL;
    char* fragment = NULL;

    if (omemo_parse_aesgcm_url(aesgcm_dl->url, &https_url, &fragment) != 0) {
        http_print_transfer_update(aesgcm_dl->window, aesgcm_dl->url,
                                   "Download failed: Cannot parse URL.");
        return NULL;
    }

    int tmpfd;
    char* tmpname = NULL;
    if ((tmpfd = g_file_open_tmp("profanity.XXXXXX", &tmpname, NULL)) == -1) {
        http_print_transfer_update(aesgcm_dl->window, aesgcm_dl->url,
                                   "Downloading '%s' failed: Unable to create "
                                   "temporary ciphertext file for writing.",
                                   https_url);
        return NULL;
    }
    FILE* tmpfh = fdopen(tmpfd, "wb");

    // Remove the file once it is closed.
    remove(tmpname);
    free(tmpname);

    HTTPDownload* http_dl = malloc(sizeof(HTTPDownload));
    http_dl->window = aesgcm_dl->window;
    http_dl->worker = aesgcm_dl->worker;
    http_dl->url = https_url;
    http_dl->filehandle = tmpfh;
    http_dl->close = 0;

    aesgcm_dl->http_dl = http_dl;

    // TODO: Verify result.
    http_file_get(http_dl);

    // Force flush as the decrypt function will read from the same stream.
    fflush(tmpfh);
    rewind(tmpfh);

    int crypt_res = omemo_decrypt_file(tmpfh, aesgcm_dl->filehandle,
                                       http_dl->bytes_received, fragment);

    fclose(tmpfh);

    if (crypt_res != 0) {
        http_print_transfer_update(aesgcm_dl->window, aesgcm_dl->url,
                                   "Downloading '%s' failed: Failed to decrypt"
                                   "file.",
                                   https_url);
    }

    fclose(aesgcm_dl->filehandle);

    return NULL;
}

void
aesgcm_download_cancel_processes(ProfWin* window)
{
    http_download_cancel_processes(window);
}

void
aesgcm_download_add_download(AESGCMDownload* aesgcm_dl)
{
    http_download_add_download(aesgcm_dl->http_dl);
}
