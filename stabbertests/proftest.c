#include <sys/stat.h>
#include <glib.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <cmocka.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <stabber.h>
#include <expect.h>

#define XDG_CONFIG_HOME "./stabbertests/files/xdg_config_home"
#define XDG_DATA_HOME   "./stabbertests/files/xdg_data_home"

char *config_orig;
char *data_orig;

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
            result = _create_dir(next_dir);
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
    int res = system("rm -rf ./stabbertests/files");
    if (res == -1) {
        assert_true(FALSE);
    }
}

void
init_prof_test(void **state)
{
    exp_timeout = 2;

    if (stbbr_start(5230) != 0) {
        assert_true(FALSE);
        return;
    }

    config_orig = getenv("XDG_CONFIG_HOME");
    data_orig = getenv("XDG_DATA_HOME");

    setenv("XDG_CONFIG_HOME", XDG_CONFIG_HOME, 1);
    setenv("XDG_DATA_HOME", XDG_DATA_HOME, 1);

    _cleanup_dirs();

    _create_config_dir();
    _create_data_dir();
    _create_chatlogs_dir();
    _create_logs_dir();
}

void
close_prof_test(void **state)
{
    _cleanup_dirs();

    setenv("XDG_CONFIG_HOME", config_orig, 1);
    setenv("XDG_DATA_HOME", data_orig, 1);

    stbbr_stop();
}
