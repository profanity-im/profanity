/*
 * files.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2018 - 2023 Michael Vetter <jubalh@idoru.org>
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

#ifndef CONFIG_FILES_H
#define CONFIG_FILES_H

#include <glib.h>

#define FILE_PROFRC                   "profrc"
#define FILE_ACCOUNTS                 "accounts"
#define FILE_TLSCERTS                 "tlscerts"
#define FILE_PLUGIN_SETTINGS          "plugin_settings"
#define FILE_PLUGIN_THEMES            "plugin_themes"
#define FILE_CAPSCACHE                "capscache"
#define FILE_PROFANITY_IDENTIFIER     "profident"
#define FILE_BOOKMARK_AUTOJOIN_IGNORE "bookmark_ignore"

#define DIR_THEMES    "themes"
#define DIR_ICONS     "icons"
#define DIR_SCRIPTS   "scripts"
#define DIR_CHATLOGS  "chatlogs"
#define DIR_OTR       "otr"
#define DIR_PGP       "pgp"
#define DIR_OMEMO     "omemo"
#define DIR_PLUGINS   "plugins"
#define DIR_DATABASE  "database"
#define DIR_DOWNLOADS "downloads"
#define DIR_EDITOR    "editor"
#define DIR_CERTS     "certs"
#define DIR_PHOTOS    "photos"

void files_create_directories(void);

gchar* files_get_config_path(const char* const config_base);
gchar* files_get_data_path(const char* const data_base);
gchar* files_get_account_data_path(const char* const specific_dir, const char* const jid);

gchar* files_get_log_file(const char* const log_file);
gchar* files_get_inputrc_file(void);

gchar*
files_file_in_account_data_path(const char* const specific_dir, const char* const jid, const char* const file_name);

#endif
