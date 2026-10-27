/*
 * api.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PLUGINS_API_H
#define PLUGINS_API_H

#ifdef HAVE_PLUGINS
#include <lua.h>

void api_init_lua(const char* const plugin_name, lua_State* L);
#endif

#endif
