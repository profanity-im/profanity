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
#include "tools/http_common.h"
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

    const size_t err_len = 100;
    char err_buf[err_len];

    if (omemo_parse_aesgcm_url(aesgcm_dl->url, &https_url, &fragment) != 0) {
        http_print_transfer_update(aesgcm_dl->window, aesgcm_dl->url,
                                   "Download failed: Cannot parse URL '%s'.",
                                   aesgcm_dl->url);
        return NULL;
    }

    char* tmpname = NULL;
    if (g_file_open_tmp("profanity.XXXXXX", &tmpname, NULL) == -1) {
        strerror_r(errno, err_buf, err_len);
        http_print_transfer_update(aesgcm_dl->window, aesgcm_dl->url,
                                   "Downloading '%s' failed: Unable to create "
                                   "temporary ciphertext file for writing "
                                   "(%s).",
                                   https_url, err_buf);
        return NULL;
    }

    FILE* outfh = fopen(aesgcm_dl->filename, "wb");
    if (outfh == NULL) {
        strerror_r(errno, err_buf, err_len);
        http_print_transfer_update(aesgcm_dl->window, aesgcm_dl->url,
                                   "Downloading '%s' failed: Unable to open "
                                   "output file at '%s' for writing (%s).",
                                   https_url, aesgcm_dl->filename, err_buf);
        return NULL;
    }

    HTTPDownload* http_dl = malloc(sizeof(HTTPDownload));
    http_dl->window = aesgcm_dl->window;
    http_dl->worker = aesgcm_dl->worker;
    http_dl->url = strdup(https_url);
    http_dl->filename = strdup(tmpname);

    aesgcm_dl->http_dl = http_dl;

    http_file_get(http_dl); // TODO(wstrm): Verify result.

    FILE* tmpfh = fopen(tmpname, "rb");
    if (tmpfh == NULL) {
        strerror_r(errno, err_buf, err_len);
        http_print_transfer_update(aesgcm_dl->window, aesgcm_dl->url,
                                   "Downloading '%s' failed: Unable to open "
                                   "temporary file at '%s' for reading (%s).",
                                   aesgcm_dl->url, aesgcm_dl->filename, err_buf);
        return NULL;
    }

    gcry_error_t crypt_res;
    crypt_res = omemo_decrypt_file(tmpfh, outfh,
                                   http_dl->bytes_received, fragment);

    if (fclose(tmpfh) == EOF) {
        strerror_r(errno, err_buf, err_len);
        cons_show_error(err_buf);
    }

    remove(tmpname);
    free(tmpname);

    if (crypt_res != GPG_ERR_NO_ERROR) {
        http_print_transfer_update(aesgcm_dl->window, aesgcm_dl->url,
                                   "Downloading '%s' failed: Failed to decrypt "
                                   "file (%s).",
                                   https_url, gcry_strerror(crypt_res));
    }

    if (fclose(outfh) == EOF) {
        strerror_r(errno, err_buf, err_len);
        cons_show_error(err_buf);
    }

    free(https_url);
    free(fragment);

    free(aesgcm_dl->filename);
    free(aesgcm_dl->url);
    free(aesgcm_dl);

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
