/*
 * titlebar.h
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

#ifndef UI_TITLEBAR_H
#define UI_TITLEBAR_H

void create_title_bar(void);
void title_bar_update_virtual(void);
void title_bar_resize(void);
void title_bar_console(void);
void title_bar_set_presence(contact_presence_t presence);
void title_bar_set_recipient(const char * const from);
void title_bar_set_typing(gboolean is_typing);

#endif