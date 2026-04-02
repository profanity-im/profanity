/*
 * editor.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2022 - 2026 Michael Vetter <jubalh@iodoru.org>
 * Copyright (C) 2022 MarcoPolo PasTonMolo  <marcopolopastonmolo@protonmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef TOOLS_EDITOR_H
#define TOOLS_EDITOR_H

#include <glib.h>

gboolean launch_editor(gchar* initial_content, void (*callback)(gchar* content, void* data), void* user_data);

#endif
