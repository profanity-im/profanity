/*
 * parser.c
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include <string.h>

#include <glib.h>

/*
 * Take a full line of input and return an array of strings representing
 * the arguments of a command.
 * If the number of arguments found is less than min, or more than max
 * NULL is returned.
 *
 * inp - The line of input
 * min - The minimum allowed number of arguments
 * max - The maxmimum allowed number of arguments
 *
 * Returns - An NULL terminated array of strings representing the aguments
 * of the command, or NULL if the validation fails.
 *
 * E.g. the following input line:
 *
 * /cmd arg1 arg2
 *
 * Will return a pointer to the following array:
 *
 * { "arg1", "arg2", NULL }
 *
 */
gchar **
parse_args(const char * const inp, int min, int max)
{
    if (inp == NULL) {
        return NULL;
    }

    // copy and strip input of leading/trailing whitepsace
    char *copy = strdup(inp);
    g_strstrip(copy);

    int inp_size = g_utf8_strlen(copy, -1);
    gboolean in_token = FALSE;
    gboolean in_quotes = FALSE;
    char *token_start = &copy[0];
    int token_size = 0;
    GSList *tokens = NULL;

    // add tokens to GSList
    int i;
    for (i = 0; i < inp_size; i++) {
        gchar *curr_ch = g_utf8_offset_to_pointer(copy, i);
        gunichar curr_uni = g_utf8_get_char(curr_ch);

        if (!in_token) {
            if (curr_uni  == ' ') {
                continue;
            } else {
                in_token = TRUE;
                if (curr_uni == '"') {
                    in_quotes = TRUE;
                    i++;
                    gchar *next_ch = g_utf8_next_char(curr_ch);
                    gunichar next_uni = g_utf8_get_char(next_ch);
                    token_start = next_ch;
                    token_size += g_unichar_to_utf8(next_uni, NULL);
                } else {
                    token_start = curr_ch;
                    token_size += g_unichar_to_utf8(curr_uni, NULL);
                }
            }
        } else {
            if (in_quotes) {
                if (curr_uni == '"') {
                    tokens = g_slist_append(tokens, g_strndup(token_start,
                        token_size));
                    token_size = 0;
                    in_token = FALSE;
                    in_quotes = FALSE;
                } else {
                    token_size += g_unichar_to_utf8(curr_uni, NULL);
                }
            } else {
                if (curr_uni == ' ') {
                    tokens = g_slist_append(tokens, g_strndup(token_start,
                        token_size));
                    token_size = 0;
                    in_token = FALSE;
                } else {
                    token_size += g_unichar_to_utf8(curr_uni, NULL);
                }
            }
        }
    }

    if (in_token) {
        tokens = g_slist_append(tokens, g_strndup(token_start, token_size));
    }

    int num = g_slist_length(tokens) - 1;

    // if num args not valid return NULL
    if ((num < min) || (num > max)) {
        g_slist_free_full(tokens, free);
        g_free(copy);
        return NULL;

    // if min allowed is 0 and 0 found, return empty char* array
    } else if (min == 0 && num == 0) {
        g_slist_free_full(tokens, free);
        gchar **args = malloc((num + 1) * sizeof(*args));
        args[0] = NULL;
        g_free(copy);
        return args;

    // otherwise return args array
    } else {
        gchar **args = malloc((num + 1) * sizeof(*args));
        GSList *token = tokens;
        token = g_slist_next(token);
        int arg_count = 0;

        while (token != NULL) {
            args[arg_count++] = strdup(token->data);
            token = g_slist_next(token);
        }

        args[arg_count] = NULL;
        g_slist_free_full(tokens, free);
        g_free(copy);

        return args;
    }
}

/*
 * Take a full line of input and return an array of strings representing
 * the arguments of a command.  This function handles when the last parameter
 * to the command is free text e.g.
 *
 * /msg user@host here is a message
 *
 * If the number of arguments found is less than min, or more than max
 * NULL is returned.
 *
 * inp - The line of input
 * min - The minimum allowed number of arguments
 * max - The maxmimum allowed number of arguments
 *
 * Returns - An NULL terminated array of strings representing the aguments
 * of the command, or NULL if the validation fails.
 *
 * E.g. the following input line:
 *
 * /cmd arg1 arg2 some free text
 *
 * Will return a pointer to the following array:
 *
 * { "arg1", "arg2", "some free text", NULL }
 *
 */
gchar **
parse_args_with_freetext(const char * const inp, int min, int max)
{
    if (inp == NULL) {
        return NULL;
    }

    // copy and strip input of leading/trailing whitepsace
    char *copy = strdup(inp);
    g_strstrip(copy);

    int inp_size = g_utf8_strlen(copy, -1);
    gboolean in_token = FALSE;
    gboolean in_freetext = FALSE;
    gboolean in_quotes = FALSE;
    char *token_start = &copy[0];
    int token_size = 0;
    int num_tokens = 0;
    GSList *tokens = NULL;

    // add tokens to GSList
    int i;
    for (i = 0; i < inp_size; i++) {
        gchar *curr_ch = g_utf8_offset_to_pointer(copy, i);
        gunichar curr_uni = g_utf8_get_char(curr_ch);

        if (!in_token) {
            if (curr_uni == ' ') {
                continue;
            } else {
                in_token = TRUE;
                num_tokens++;
                if ((num_tokens == max + 1) && (curr_uni != '"')) {
                    in_freetext = TRUE;
                } else if (curr_uni == '"') {
                    in_quotes = TRUE;
                    i++;
                    gchar *next_ch = g_utf8_next_char(curr_ch);
                    gunichar next_uni = g_utf8_get_char(next_ch);
                    token_start = next_ch;
                    token_size += g_unichar_to_utf8(next_uni, NULL);
                }
                if (curr_uni == '"') {
                    gchar *next_ch = g_utf8_next_char(curr_ch);
                    token_start = next_ch;
                } else {
                    token_start = curr_ch;
                    token_size += g_unichar_to_utf8(curr_uni, NULL);
                }
            }
        } else {
            if (in_quotes) {
                if (curr_uni == '"') {
                    tokens = g_slist_append(tokens, g_strndup(token_start,
                        token_size));
                    token_size = 0;
                    in_token = FALSE;
                    in_quotes = FALSE;
                } else {
                    if (curr_uni != '"') {
                        token_size += g_unichar_to_utf8(curr_uni, NULL);
                    }
                }
            } else {
                if (in_freetext) {
                    token_size += g_unichar_to_utf8(curr_uni, NULL);
                } else if (curr_uni == ' ') {
                    tokens = g_slist_append(tokens, g_strndup(token_start,
                        token_size));
                    token_size = 0;
                    in_token = FALSE;
                } else if (curr_uni != '"') {
                    token_size += g_unichar_to_utf8(curr_uni, NULL);
                }
            }
        }
    }

    if (in_token) {
        tokens = g_slist_append(tokens, g_strndup(token_start, token_size));
    }

    free(copy);

    int num = g_slist_length(tokens) - 1;

    // if num args not valid return NULL
    if ((num < min) || (num > max)) {
        g_slist_free_full(tokens, free);
        return NULL;

    // if min allowed is 0 and 0 found, return empty char* array
    } else if (min == 0 && num == 0) {
        gchar **args = malloc((num + 1) * sizeof(*args));
        args[0] = NULL;
        return args;

    // otherwise return args array
    } else {
        gchar **args = malloc((num + 1) * sizeof(*args));
        GSList *token = tokens;
        token = g_slist_next(token);
        int arg_count = 0;

        while (token != NULL) {
            args[arg_count++] = strdup(token->data);
            token = g_slist_next(token);
        }

        args[arg_count] = NULL;
        g_slist_free_full(tokens, free);

        return args;
    }
}

int
count_tokens(char *string)
{
    int length = g_utf8_strlen(string, -1);
    gboolean in_quotes = FALSE;
    int num_tokens = 0;
    int i = 0;

    // include first token
    num_tokens++;

    for (i = 0; i < length; i++) {
        gchar *curr_ch = g_utf8_offset_to_pointer(string, i);
        gunichar curr_uni = g_utf8_get_char(curr_ch);

        if (curr_uni == ' ') {
            if (!in_quotes) {
                num_tokens++;
            }
        } else if (curr_uni == '"') {
            if (in_quotes) {
                in_quotes = FALSE;
            } else {
                in_quotes = TRUE;
            }
        }
    }

    return num_tokens;
}

char *
get_start(char *string, int tokens)
{
    GString *result = g_string_new("");
    int length = g_utf8_strlen(string, -1);
    gboolean in_quotes = FALSE;
    char *result_str = NULL;
    int num_tokens = 0;
    int i = 0;

    // include first token
    num_tokens++;

    for (i = 0; i < length; i++) {
        gchar *curr_ch = g_utf8_offset_to_pointer(string, i);
        gunichar curr_uni = g_utf8_get_char(curr_ch);

        if (num_tokens < tokens) {
            gchar *uni_char = malloc(7);
            int len = g_unichar_to_utf8(curr_uni, uni_char);
            uni_char[len] = '\0';
            g_string_append(result, uni_char);
        }
        if (curr_uni == ' ') {
            if (!in_quotes) {
                num_tokens++;
            }
        } else if (curr_uni == '"') {
            if (in_quotes) {
                in_quotes = FALSE;
            } else {
                in_quotes = TRUE;
            }
        }
    }

    result_str = result->str;
    g_string_free(result, FALSE);

    return result_str;
}
