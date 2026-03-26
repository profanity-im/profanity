/*
 * spellcheck.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#ifdef HAVE_SPELLCHECK

#include <enchant.h>
#include <string.h>
#include <glib.h>

#include "tools/spellcheck.h"
#include "config/preferences.h"
#include "log.h"
#include "common.h"

static EnchantBroker* broker = NULL;
static EnchantDict* dict = NULL;
static char* current_lang = NULL;

void
spellcheck_init(void)
{
    if (broker) {
        return;
    }

    broker = enchant_broker_init();
    if (!broker) {
        log_error("Failed to initialize Enchant broker");
        return;
    }

    prof_add_shutdown_routine(spellcheck_deinit);

    char* lang = prefs_get_string(PREF_SPELLCHECK_LANG);
    spellcheck_set_lang(lang);
    g_free(lang);
}

void
spellcheck_deinit(void)
{
    if (dict) {
        enchant_broker_free_dict(broker, dict);
        dict = NULL;
    }
    if (broker) {
        enchant_broker_free(broker);
        broker = NULL;
    }
    g_free(current_lang);
    current_lang = NULL;
}

gboolean
spellcheck_set_lang(const char* lang)
{
    if (!broker || !lang) {
        return FALSE;
    }

    if (current_lang && strcmp(current_lang, lang) == 0 && dict) {
        return TRUE;
    }

    EnchantDict* new_dict = enchant_broker_request_dict(broker, lang);
    if (!new_dict) {
        log_error("Failed to request Enchant dictionary for language: %s", lang);
        return FALSE;
    }

    if (dict) {
        enchant_broker_free_dict(broker, dict);
    }

    dict = new_dict;
    g_free(current_lang);
    current_lang = g_strdup(lang);

    return TRUE;
}

const char*
spellcheck_get_lang(void)
{
    return current_lang;
}

gboolean
spellcheck_is_misspelled(const char* word)
{
    if (!dict || !word || word[0] == '\0') {
        return FALSE;
    }

    int result = enchant_dict_check(dict, word, strlen(word));
    return (result != 0);
}

#endif
