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

gchar** parse_args(const gchar* const inp, int min, int max, gboolean* result);
gchar** parse_args_with_freetext(const gchar* const inp, int min, int max, gboolean* result);
gchar** parse_args_as_one(const gchar* const inp, int min, int max, gboolean* result);
int count_tokens(const gchar* const string);
gchar* get_start(const gchar* const string, int tokens);
GHashTable* parse_options(gchar** args, gchar** keys, gboolean* res);
void options_destroy(GHashTable* options);

#endif
