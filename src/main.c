/*
 * main.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2024 Michael Vetter <jubalh@iodoru.org>
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

#include "config.h"

#include <string.h>
#include <glib.h>

#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif

#ifdef HAVE_LIBGPGME
#include "pgp/gpg.h"
#endif

#ifdef HAVE_PYTHON
#include "plugins/python_plugins.h"
#endif

#include "profanity.h"
#include "common.h"
#include "command/cmd_defs.h"

static gboolean version = FALSE;
static char* log = NULL;
static char* log_file = NULL;
static char* account_name = NULL;
static char* config_file = NULL;
static char* theme_name = NULL;

int
main(int argc, char** argv)
{
    if (argc == 2 && g_strcmp0(PACKAGE_STATUS, "development") == 0) {
        if (g_strcmp0(argv[1], "docgen") == 0) {
            command_docgen();
            return 0;
        } else if (g_strcmp0(argv[1], "mangen") == 0) {
            command_mangen();
            return 0;
        }
    }

    static GOptionEntry entries[] = {
        { "version", 'v', 0, G_OPTION_ARG_NONE, &version, "Show version information", NULL },
        { "account", 'a', 0, G_OPTION_ARG_STRING, &account_name, "Auto connect to an account on startup" },
        { "log", 'l', 0, G_OPTION_ARG_STRING, &log, "Set logging levels, DEBUG, INFO, WARN (default), ERROR", "LEVEL" },
        { "config", 'c', 0, G_OPTION_ARG_STRING, &config_file, "Use an alternative configuration file", NULL },
        { "logfile", 'f', 0, G_OPTION_ARG_STRING, &log_file, "Specify log file", NULL },
        { "theme", 't', 0, G_OPTION_ARG_STRING, &theme_name, "Specify theme name", NULL },
        { NULL }
    };

    GError* error = NULL;
    GOptionContext* context;

    context = g_option_context_new(NULL);
    g_option_context_add_main_entries(context, entries, NULL);
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_print("%s\n", error->message);
        g_option_context_free(context);
        g_error_free(error);
        return 1;
    }

    g_option_context_free(context);

    if (version == TRUE) {
        auto_gchar gchar* prof_version = prof_get_version();
        g_print("Profanity, version %s\n", prof_version);

        // lets use fixed email instead of PACKAGE_BUGREPORT
        g_print("Copyright (C) 2012 - 2019 James Booth <boothj5web@gmail.com>.\n");
        g_print("Copyright (C) 2019 - 2024 Michael Vetter <jubalh@iodoru.org>.\n");
        g_print("License GPLv3+: GNU GPL version 3 or later <https://www.gnu.org/licenses/gpl.html>\n");
        g_print("\n");
        g_print("This is free software; you are free to change and redistribute it.\n");
        g_print("There is NO WARRANTY, to the extent permitted by law.\n");
        g_print("\n");

        g_print("Build information:\n");

        g_print("XMPP library: libstrophe\n");

        if (is_notify_enabled()) {
            g_print("Desktop notification support: Enabled\n");
        } else {
            g_print("Desktop notification support: Disabled\n");
        }

#ifdef HAVE_LIBOTR
        char* otr_version = otr_libotr_version();
        g_print("OTR support: Enabled (libotr %s)\n", otr_version);
#else
        g_print("OTR support: Disabled\n");
#endif

#ifdef HAVE_LIBGPGME
        const char* pgp_version = p_gpg_libver();
        g_print("PGP support: Enabled (libgpgme %s)\n", pgp_version);
#else
        g_print("PGP support: Disabled\n");
#endif

#ifdef HAVE_OMEMO
        g_print("OMEMO support: Enabled\n");
#else
        g_print("OMEMO support: Disabled\n");
#endif

#ifdef HAVE_C
        g_print("C plugins: Enabled\n");
#else
        g_print("C plugins: Disabled\n");
#endif

#ifdef HAVE_PYTHON
        auto_gchar gchar* python_version = python_get_version_number();
        g_print("Python plugins: Enabled (%s)\n", python_version);
#else
        g_print("Python plugins: Disabled\n");
#endif

#ifdef HAVE_GTK
        g_print("GTK icons/clipboard: Enabled\n");
#else
        g_print("GTK icons/clipboard: Disabled\n");
#endif

#ifdef HAVE_PIXBUF
        g_print("GDK Pixbuf: Enabled\n");
#else
        g_print("GDK Pixbuf: Disabled\n");
#endif

        return 0;
    }

    /* Default logging WARN */
    prof_run(log ? log : "WARN", account_name, config_file, log_file, theme_name);

    /* Free resources allocated by GOptionContext */
    g_free(log);
    g_free(account_name);
    g_free(config_file);
    g_free(log_file);
    g_free(theme_name);

    return 0;
}
