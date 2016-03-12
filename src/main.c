/*
 * main.c
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
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

#include "prof_config.h"

#include <gtk/gtk.h>
#include <string.h>
#include <glib.h>
#include <assert.h>

#ifdef PROF_HAVE_GIT_VERSION
#include "gitversion.h"
#endif

#include "profanity.h"
#include "common.h"
#include "command/command.h"

static gboolean version = FALSE;
static char *log = "INFO";
static char *account_name = NULL;

int
main(int argc, char **argv)
{
    if (argc == 2 && g_strcmp0(argv[1], "docgen") == 0 && g_strcmp0(PROF_PACKAGE_STATUS, "development") == 0) {
        command_docgen();
        return 0;
    }

    static GOptionEntry entries[] =
    {
        { "version", 'v', 0, G_OPTION_ARG_NONE, &version, "Show version information", NULL },
        { "account", 'a', 0, G_OPTION_ARG_STRING, &account_name, "Auto connect to an account on startup" },
        { "log",'l', 0, G_OPTION_ARG_STRING, &log, "Set logging levels, DEBUG, INFO (default), WARN, ERROR", "LEVEL" },
        { NULL }
    };

    GError *error = NULL;
    GOptionContext *context;

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
        if (strcmp(PROF_PACKAGE_STATUS, "development") == 0) {
#ifdef PROF_HAVE_GIT_VERSION
            g_print("Profanity, version %sdev.%s.%s\n", PROF_PACKAGE_VERSION, PROF_GIT_BRANCH, PROF_GIT_REVISION);
#else
            g_print("Profanity, version %sdev\n", PROF_PACKAGE_VERSION);
#endif
        } else {
            g_print("Profanity, version %s\n", PROF_PACKAGE_VERSION);
        }

        g_print("Copyright (C) 2012 - 2016 James Booth <%s>.\n", PROF_PACKAGE_BUGREPORT);
        g_print("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
        g_print("\n");
        g_print("This is free software; you are free to change and redistribute it.\n");
        g_print("There is NO WARRANTY, to the extent permitted by law.\n");
        g_print("\n");

        g_print("Build information:\n");

#ifdef PROF_HAVE_LIBMESODE
        g_print("XMPP library: libmesode\n");
#endif
#ifdef PROF_HAVE_LIBSTROPHE
        g_print("XMPP library: libstrophe\n");
#endif

        if (is_notify_enabled()) {
            g_print("Desktop notification support: Enabled\n");
        } else {
            g_print("Desktop notification support: Disabled\n");
        }

#ifdef PROF_HAVE_LIBOTR
        g_print("OTR support: Enabled\n");
#else
        g_print("OTR support: Disabled\n");
#endif

#ifdef PROF_HAVE_LIBGPGME
        g_print("PGP support: Enabled\n");
#else
        g_print("PGP support: Disabled\n");
#endif

#ifdef PROF_HAVE_C
        g_print("C plugins: Enabled\n");
#else
        g_print("C plugins: Disabled\n");
#endif

#ifdef PROF_HAVE_PYTHON
        g_print("Python plugins: Enabled\n");
#else
        g_print("Python plugins: Disabled\n");
#endif

        return 0;
    }

    assert (gtk_init_check(&argc, &argv) == true);
    gtk_init(&argc, &argv);
    prof_run(log, account_name);

    return 0;
}
