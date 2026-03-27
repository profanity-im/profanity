/*
 * spellcheck.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef TOOLS_SPELLCHECK_H
#define TOOLS_SPELLCHECK_H

#include "config.h"
#include <glib.h>

#ifdef HAVE_SPELLCHECK

void spellcheck_init(void);
void spellcheck_deinit(void);
gboolean spellcheck_is_misspelled(const char* word);
gboolean spellcheck_set_lang(const char* lang);
const char* spellcheck_get_lang(void);
GList* spellcheck_get_available_langs(void);

#else

static inline void
spellcheck_init(void)
{
}
static inline void
spellcheck_deinit(void)
{
}
static inline gboolean
spellcheck_is_misspelled(const char* word)
{
    return FALSE;
}
static inline gboolean
spellcheck_set_lang(const char* lang)
{
    return FALSE;
}
static inline const char*
spellcheck_get_lang(void)
{
    return NULL;
}
static inline GList*
spellcheck_get_available_langs(void)
{
    return NULL;
}

#endif

#endif
