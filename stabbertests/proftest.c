#include <sys/stat.h>
#include <glib.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <cmocka.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"

#include "config/preferences.h"
#include "log.h"
#include "xmpp/xmpp.h"
#include "command/command.h"
#include "roster_list.h"

#ifdef HAVE_GIT_VERSION
#include "gitversion.h"
#endif

#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif


#define XDG_CONFIG_HOME "./stabbertests/files/xdg_config_home"
#define XDG_DATA_HOME   "./stabbertests/files/xdg_data_home"

gboolean
_create_dir(char *name)
{
    struct stat sb;

    if (stat(name, &sb) != 0) {
        if (errno != ENOENT || mkdir(name, S_IRWXU) != 0) {
            return FALSE;
        }
    } else {
        if ((sb.st_mode & S_IFDIR) != S_IFDIR) {
            return FALSE;
        }
    }

    return TRUE;
}

gboolean
_mkdir_recursive(const char *dir)
{
    int i;
    gboolean result = TRUE;

    for (i = 1; i <= strlen(dir); i++) {
        if (dir[i] == '/' || dir[i] == '\0') {
            gchar *next_dir = g_strndup(dir, i);
            result = create_dir(next_dir);
            g_free(next_dir);
            if (!result) {
                break;
            }
        }
    }

    return result;
}

void
_create_config_dir(void)
{
    GString *profanity_dir = g_string_new(XDG_CONFIG_HOME);
    g_string_append(profanity_dir, "/profanity");

    if (!_mkdir_recursive(profanity_dir->str)) {
        assert_true(FALSE);
    }

    g_string_free(profanity_dir, TRUE);
}

void
_create_data_dir(void)
{
    GString *profanity_dir = g_string_new(XDG_DATA_HOME);
    g_string_append(profanity_dir, "/profanity");

    if (!_mkdir_recursive(profanity_dir->str)) {
        assert_true(FALSE);
    }

    g_string_free(profanity_dir, TRUE);
}

void
_create_chatlogs_dir(void)
{
    GString *chatlogs_dir = g_string_new(XDG_DATA_HOME);
    g_string_append(chatlogs_dir, "/profanity/chatlogs");

    if (!_mkdir_recursive(chatlogs_dir->str)) {
        assert_true(FALSE);
    }

    g_string_free(chatlogs_dir, TRUE);
}

void
_create_logs_dir(void)
{
    GString *logs_dir = g_string_new(XDG_DATA_HOME);
    g_string_append(logs_dir, "/profanity/logs");

    if (!_mkdir_recursive(logs_dir->str)) {
        assert_true(FALSE);
    }

    g_string_free(logs_dir, TRUE);
}

void
_cleanup_dirs(void)
{
    system("rm -rf ./stabbertests/files");
}

void
init_prof_test(void **state)
{
    setenv("XDG_CONFIG_HOME", XDG_CONFIG_HOME, 1);
    setenv("XDG_DATA_HOME", XDG_DATA_HOME, 1);

    _create_config_dir();
    _create_data_dir();
    _create_chatlogs_dir();
    _create_logs_dir();

    log_level_t prof_log_level = log_level_from_string("DEBUG");
    prefs_load();
    log_init(prof_log_level);
    if (strcmp(PACKAGE_STATUS, "development") == 0) {
#ifdef HAVE_GIT_VERSION
            log_info("Starting Profanity (%sdev.%s.%s)...", PACKAGE_VERSION, PROF_GIT_BRANCH, PROF_GIT_REVISION);
#else
            log_info("Starting Profanity (%sdev)...", PACKAGE_VERSION);
#endif
    } else {
        log_info("Starting Profanity (%s)...", PACKAGE_VERSION);
    }
    chat_log_init();
    groupchat_log_init();
    accounts_load();
    char *theme = prefs_get_string(PREF_THEME);
    theme_init(theme);
    prefs_free_string(theme);
    jabber_init(FALSE);
    cmd_init();
    log_info("Initialising contact list");
    roster_init();
    muc_init();
#ifdef HAVE_LIBOTR
    otr_init();
#endif
}

void
close_prof_test(void **state)
{
    jabber_disconnect();
    jabber_shutdown();
    roster_free();
    muc_close();
    caps_close();
#ifdef HAVE_LIBOTR
    otr_shutdown();
#endif
    chat_log_close();
    prefs_close();
    theme_close();
    accounts_close();
    cmd_uninit();
    log_close();

    _cleanup_dirs();
}
