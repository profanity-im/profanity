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

gboolean get_message_from_editor(gchar* message, gchar** returned_message);

#endif
