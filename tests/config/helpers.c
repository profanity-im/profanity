#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <cmocka.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "config/preferences.h"

void init_preferences(void **state)
{
    setenv("XDG_CONFIG_HOME", "./tests/files/xdg_config_home", 1);
    gchar *xdg_config = xdg_get_config_home();

    GString *profanity_dir = g_string_new(xdg_config);
    g_string_append(profanity_dir, "/profanity");

    if (!mkdir_recursive(profanity_dir->str)) {
        assert_true(FALSE);
    }
    g_string_free(profanity_dir, TRUE);

    FILE *f = fopen("./tests/files/xdg_config_home/profanity/profrc", "ab+");
    if (f) {
        g_free(xdg_config);
        prefs_load();
    }
}

void close_preferences(void **state)
{
    prefs_close();
    remove("./tests/files/xdg_config_home/profanity/profrc");
    rmdir("./tests/files/xdg_config_home/profanity");
    rmdir("./tests/files/xdg_config_home");
    rmdir("./tests/files");
}
