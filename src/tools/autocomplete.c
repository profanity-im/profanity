/*
 * autocomplete.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#ifdef HAVE_ICU_I18N
#include <unicode/utypes.h>
#include <unicode/localpointer.h>
#include <unicode/urep.h>
#include <unicode/parseerr.h>
#include <unicode/uenum.h>
#include <unicode/uset.h>
#include <unicode/utrans.h>
#include <unicode/ustring.h>
#endif

#include "common.h"
#include "tools/autocomplete.h"
#include "tools/parser.h"

struct autocomplete_t {
    GSList *items;
    GSList *last_found;
    gchar *search_str;
#ifdef HAVE_ICU_I18N
    UTransliterator *utrans;
#endif
};

static gchar* _search_from(Autocomplete ac, GSList *curr, gboolean quote, gboolean ignore_accents);

Autocomplete
autocomplete_new(void)
{
    Autocomplete new = malloc(sizeof(struct autocomplete_t));
    new->items = NULL;
    new->last_found = NULL;
    new->search_str = NULL;
#ifdef HAVE_ICU_I18N
    U_STRING_DECL(u_str, "NFD; [:M:] Remove; NFC", 22);
    UErrorCode code = U_ZERO_ERROR;
    new->utrans = utrans_openU(u_str, -1, UTRANS_FORWARD, NULL,
			       -1 /* NULL-terminated */, NULL, &code);
    if(!new->utrans){
	printf("%s\n", u_errorName(code));
	abort();
    }
#endif
    return new;
}

void
autocomplete_clear(Autocomplete ac)
{
    if (ac) {
        g_slist_free_full(ac->items, free);
        ac->items = NULL;

        autocomplete_reset(ac);
    }
}

void
autocomplete_reset(Autocomplete ac)
{
    ac->last_found = NULL;
    FREE_SET_NULL(ac->search_str);
}

void
autocomplete_free(Autocomplete ac)
{
    if (ac) {
        autocomplete_clear(ac);
#ifdef HAVE_ICU_I18N
	utrans_close(ac->utrans);
#endif
        free(ac);
    }
}

gint
autocomplete_length(Autocomplete ac)
{
    if (!ac) {
        return 0;
    } else if (!ac->items) {
        return 0;
    } else {
        return g_slist_length(ac->items);
    }
}

#ifdef HAVE_ICU_I18N
static int ucompare(gconstpointer a, gconstpointer b, gpointer data)
{
  UChar ua[strlen (a) + 1];
  UChar ub[strlen (b) + 1];
  UTransliterator *utrans = data;

  UErrorCode code = U_ZERO_ERROR;
  u_strFromUTF8 (ua, sizeof (ua), NULL, a, -1, &code);
  if (U_FAILURE (code)) {
      goto fallback;
  }
  u_strFromUTF8 (ub, sizeof (ub), NULL, b, -1, &code);
  if (U_FAILURE (code)) {
      goto fallback;
  }
  int32_t limit = u_strlen (ua);
  utrans_transUChars (utrans, ua, NULL, sizeof (ua), 0, &limit, &code);
  if (U_FAILURE (code)) {
      goto fallback;
  }
  limit = u_strlen (ub);
  utrans_transUChars (utrans, ub, NULL, sizeof (ub), 0, &limit, &code);
  if (U_FAILURE (code)) {
      goto fallback;
  }
  return u_strcmp(ua, ub);
fallback:
  return strcmp (a, b);
}
#endif

void
autocomplete_add(Autocomplete ac, const char *item)
{
    if (ac) {
        char *item_cpy;
        GSList *curr = g_slist_find_custom(ac->items, item, (GCompareFunc)strcmp);

        // if item already exists
        if (curr) {
            return;
        }

        item_cpy = strdup(item);
#ifdef HAVE_ICU_I18N
        ac->items = g_slist_insert_sorted_with_data(ac->items, item_cpy, ucompare, ac->utrans);
#else
        ac->items = g_slist_insert_sorted(ac->items, item_cpy, (GCompareFunc)strcmp);
#endif
    }

    return;
}

void
autocomplete_add_all(Autocomplete ac, char **items)
{
    int i = 0;
    for (i = 0; i < g_strv_length(items); i++) {
        autocomplete_add(ac, items[i]);
    }
}

void
autocomplete_remove(Autocomplete ac, const char *const item)
{
    if (ac) {
        GSList *curr = g_slist_find_custom(ac->items, item, (GCompareFunc)strcmp);

        if (!curr) {
            return;
        }

        // reset last found if it points to the item to be removed
        if (ac->last_found == curr) {
            ac->last_found = NULL;
        }

        free(curr->data);
        ac->items = g_slist_delete_link(ac->items, curr);
    }

    return;
}

void
autocomplete_remove_all(Autocomplete ac, char **items)
{
    int i = 0;
    for (i = 0; i < g_strv_length(items); i++) {
        autocomplete_remove(ac, items[i]);
    }
}

GSList*
autocomplete_create_list(Autocomplete ac)
{
    GSList *copy = NULL;
    GSList *curr = ac->items;

    while(curr) {
        copy = g_slist_append(copy, strdup(curr->data));
        curr = g_slist_next(curr);
    }

    return copy;
}

gboolean
autocomplete_contains(Autocomplete ac, const char *value)
{
    GSList *curr = ac->items;

    while(curr) {
        if (strcmp(curr->data, value) == 0) {
            return TRUE;
        }
        curr = g_slist_next(curr);
    }

    return FALSE;
}

static gchar*
autocomplete_complete_internal(Autocomplete ac, const gchar *search_str, gboolean quote,
		      gboolean ignore_accents)
{
    gchar *found = NULL;

    // no autocomplete to search
    if (!ac) {
        return NULL;
    }

    // no items to search
    if (!ac->items) {
        return NULL;
    }

    // first search attempt
    if (!ac->last_found) {
        if (ac->search_str) {
            FREE_SET_NULL(ac->search_str);
        }

        ac->search_str = strdup(search_str);
        found = _search_from(ac, ac->items, quote, ignore_accents);

        return found;

    // subsequent search attempt
    } else {
        // search from here+1 to end
        found = _search_from(ac, g_slist_next(ac->last_found), quote, ignore_accents);
        if (found) {
            return found;
        }

        // search from beginning
        found = _search_from(ac, ac->items, quote, ignore_accents);
        if (found) {
            return found;
        }

        // we found nothing, reset search
        autocomplete_reset(ac);

        return NULL;
    }
}

gchar*
autocomplete_complete_ignore_accents(Autocomplete ac, const gchar *search_str,
				     gboolean quote)
{
  return autocomplete_complete_internal(ac, search_str, quote, TRUE);
}

gchar*
autocomplete_complete(Autocomplete ac, const gchar *search_str, gboolean quote)
{
  return autocomplete_complete_internal(ac, search_str, quote, FALSE);
}

char*
autocomplete_param_with_func(const char *const input, char *command, autocomplete_func func)
{
    GString *auto_msg = NULL;
    char *result = NULL;
    char command_cpy[strlen(command) + 2];
    sprintf(command_cpy, "%s ", command);
    int len = strlen(command_cpy);

    if (strncmp(input, command_cpy, len) == 0) {
        int i;
        int inp_len = strlen(input);
        char prefix[inp_len];
        for(i = len; i < inp_len; i++) {
            prefix[i-len] = input[i];
        }
        prefix[inp_len - len] = '\0';

        char *found = func(prefix);
        if (found) {
            auto_msg = g_string_new(command_cpy);
            g_string_append(auto_msg, found);
            free(found);
            result = auto_msg->str;
            g_string_free(auto_msg, FALSE);
        }
    }

    return result;
}

char*
autocomplete_param_with_ac(const char *const input, char *command, Autocomplete ac, gboolean quote)
{
    GString *auto_msg = NULL;
    char *result = NULL;
    char *command_cpy = malloc(strlen(command) + 2);
    sprintf(command_cpy, "%s ", command);
    int len = strlen(command_cpy);
    int inp_len = strlen(input);
    if (strncmp(input, command_cpy, len) == 0) {
        int i;
        char prefix[inp_len];
        for(i = len; i < inp_len; i++) {
            prefix[i-len] = input[i];
        }
        prefix[inp_len - len] = '\0';

        char *found = autocomplete_complete(ac, prefix, quote);
        if (found) {
            auto_msg = g_string_new(command_cpy);
            g_string_append(auto_msg, found);
            free(found);
            result = auto_msg->str;
            g_string_free(auto_msg, FALSE);
        }
    }
    free(command_cpy);

    return result;
}

char*
autocomplete_param_no_with_func(const char *const input, char *command, int arg_number, autocomplete_func func)
{
    if (strncmp(input, command, strlen(command)) == 0) {
        GString *result_str = NULL;

        // count tokens properly
        int num_tokens = count_tokens(input);

        // if correct number of tokens, then candidate for autocompletion of last param
        if (num_tokens == arg_number) {
            gchar *start_str = get_start(input, arg_number);
            gchar *comp_str = g_strdup(&input[strlen(start_str)]);

            // autocomplete param
            if (comp_str) {
                char *found = func(comp_str);
                if (found) {
                    result_str = g_string_new("");
                    g_string_append(result_str, start_str);
                    g_string_append(result_str, found);
                    char *result = result_str->str;
                    g_string_free(result_str, FALSE);
                    return result;
                }
            }
        }
    }

    return NULL;
}

static gchar*
_search_from(Autocomplete ac, GSList *curr, gboolean quote, gboolean ignore_accents)
{
    while(curr) {
	char *cmp_against = curr->data;
	if(ignore_accents) {
#ifdef HAVE_ICU_I18N
	    const size_t len = strlen (cmp_against);
	    UChar utf16[len + 1];
	    char utf8[len + 1];

	    UErrorCode code = U_ZERO_ERROR;
	    u_strFromUTF8 (utf16, sizeof (utf16), NULL, cmp_against, -1, &code);
	    if (U_SUCCESS(code)) {
		int32_t limit = u_strlen (utf16);
		utrans_transUChars (ac->utrans, utf16, NULL, sizeof (utf16), 0,
				    &limit, &code);
		if (U_SUCCESS(code)) {
		    int32_t written = 0;
		    u_strToUTF8 (utf8, sizeof (utf8), &written, utf16, -1, &code);
		    if (U_SUCCESS(code)) {
			cmp_against = utf8;
		    }
		}
	    }
#endif
	}
        // match found
        if (strncmp(cmp_against, ac->search_str, strlen(ac->search_str)) == 0) {

            // set pointer to last found
            ac->last_found = curr;

            // if contains space, quote before returning
            if (quote && g_strrstr(curr->data, " ")) {
                GString *quoted = g_string_new("\"");
                g_string_append(quoted, curr->data);
                g_string_append(quoted, "\"");

                gchar *result = quoted->str;
                g_string_free(quoted, FALSE);

                return result;

            // otherwise just return the string
            } else {
		return strdup(curr->data);
            }
        }

        curr = g_slist_next(curr);
    }

    return NULL;
}
