/*
 * cmd_defs.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef COMMAND_CMD_DEFS_H
#define COMMAND_CMD_DEFS_H

#include <glib.h>

#include "ui/ui.h"

void cmd_init(void);

Command* cmd_get(const gchar* const command);
GList* cmd_get_ordered(const gchar* const tag);

gboolean cmd_valid_tag(const gchar* const str);

void command_docgen(void);
void command_mangen(void);

GList* cmd_search_index_all(const gchar* const term);
GList* cmd_search_index_any(const gchar* const term);

#endif
