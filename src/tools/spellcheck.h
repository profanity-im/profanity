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
gboolean spellcheck_is_misspelled(const gchar* word);
gboolean spellcheck_set_lang(const gchar* lang);
const gchar* spellcheck_get_lang(void);
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
spellcheck_is_misspelled(const gchar* word)
{
    return FALSE;
}
static inline gboolean
spellcheck_set_lang(const gchar* lang)
{
    return FALSE;
}
static inline const gchar*
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
