/*
 * files.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2018 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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
gchar* files_get_data_path(const char* const location);
gchar* files_get_download_path(const char* const jid);
gchar* files_get_account_data_path(const char* const specific_dir, const char* const jid);

gchar* files_get_log_file(const char* const log_file);
gchar* files_get_inputrc_file(void);

gchar*
files_file_in_account_data_path(const char* const specific_dir, const char* const jid, const char* const file_name);

#endif
