/*
 * http_common.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2020 William Wennerström <william@wstrm.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef TOOLS_HTTP_COMMON_H
#define TOOLS_HTTP_COMMON_H

#include "ui/window.h"

void http_print_transfer(ProfWin* window, char* id, theme_item_t theme_item, const char* fmt, ...);
void http_print_transfer_update(ProfWin* window, char* id, theme_item_t theme_item, int flags, const char* fmt, ...);

#endif
