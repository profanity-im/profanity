#define _XOPEN_SOURCE 600
#include <glib.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <ftw.h>
#include <stdlib.h>
#include <cmocka.h>

#include "common.h"

static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

static int rmrf(char *path)
{
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

void create_config_dir(void **state)
{
    setenv("XDG_CONFIG_HOME", "./tests/files/xdg_config_home", 1);
    gchar *xdg_config = xdg_get_config_home();

    GString *profanity_dir = g_string_new(xdg_config);
    g_string_append(profanity_dir, "/profanity");

    if (!mkdir_recursive(profanity_dir->str)) {
        assert_true(FALSE);
    }
    g_string_free(profanity_dir, TRUE);

    g_free(xdg_config);
}

void delete_config_dir(void **state)
{
   rmrf("./tests/files");
}
