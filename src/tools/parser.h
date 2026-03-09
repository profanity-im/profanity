/*
 * parser.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef TOOLS_PARSER_H
#define TOOLS_PARSER_H

#include <glib.h>

gchar** parse_args(const char* const inp, int min, int max, gboolean* result);
gchar** parse_args_with_freetext(const char* const inp, int min, int max, gboolean* result);
gchar** parse_args_as_one(const char* const inp, int min, int max, gboolean* result);
int count_tokens(const char* const string);
char* get_start(const char* const string, int tokens);
GHashTable* parse_options(gchar** args, gchar** keys, gboolean* res);
void options_destroy(GHashTable* options);

#endif
