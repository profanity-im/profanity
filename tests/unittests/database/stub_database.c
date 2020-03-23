/*
 * stub_database.c
 *
 * Copyright (C) 2020 Michael Vetter <jubalh@iodoru.org>
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

#include <glib.h>
#include <setjmp.h>
#include <cmocka.h>

#include "database.h"

gboolean log_database_init(ProfAccount *account) { return TRUE; }
void log_database_add(ProfMessage *message, gboolean is_muc) {}
void log_database_close(void) {}
