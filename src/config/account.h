/*
 * account.h
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

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "common.h"

typedef struct prof_account_t {
    gchar *name;
    gchar *jid;
    gchar *password;
    gchar *resource;
    gchar *server;
    int port;
    gchar *last_presence;
    gchar *login_presence;
    gint priority_online;
    gint priority_chat;
    gint priority_away;
    gint priority_xa;
    gint priority_dnd;
    gchar *muc_service;
    gchar *muc_nick;
    gboolean enabled;
} ProfAccount;

ProfAccount* account_new(const gchar * const name, const gchar * const jid,
    const gchar * const passord, gboolean enabled, const gchar * const server,
    int port, const gchar * const resource, const gchar * const last_presence,
    const gchar * const login_presence, int priority_online, int priority_chat,
    int priority_away, int priority_xa, int priority_dnd,
    const gchar * const muc_service, const gchar * const muc_nick);

char* account_create_full_jid(ProfAccount *account);

void account_free(ProfAccount *account);

#endif
