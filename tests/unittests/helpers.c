#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <cmocka.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"
#include "config/preferences.h"
#include "xmpp/chat_session.h"

void create_config_dir(void **state)
{
    setenv("XDG_CONFIG_HOME", "./tests/files/xdg_config_home", 1);
    if (!mkdir_recursive("./tests/files/xdg_config_home/profanity")) {
        assert_true(FALSE);
    }
}

void remove_config_dir(void **state)
{
    rmdir("./tests/files/xdg_config_home/profanity");
    rmdir("./tests/files/xdg_config_home");
}

void create_data_dir(void **state)
{
    setenv("XDG_DATA_HOME", "./tests/files/xdg_data_home", 1);
    if (!mkdir_recursive("./tests/files/xdg_data_home/profanity")) {
        assert_true(FALSE);
    }
}

void remove_data_dir(void **state)
{
    rmdir("./tests/files/xdg_data_home/profanity");
    rmdir("./tests/files/xdg_data_home");
}

void load_preferences(void **state)
{
    create_config_dir(state);
    FILE *f = fopen("./tests/files/xdg_config_home/profanity/profrc", "ab+");
    if (f) {
        prefs_load(NULL);
    }
    fclose(f);
}

void close_preferences(void **state)
{
    prefs_close();
    remove("./tests/files/xdg_config_home/profanity/profrc");
    remove_config_dir(state);
    rmdir("./tests/files");
}

void init_chat_sessions(void **state)
{
    load_preferences(NULL);
    chat_sessions_init();
}

void close_chat_sessions(void **state)
{
    chat_sessions_clear();
    close_preferences(NULL);
}

int
utf8_pos_to_col(char *str, int utf8_pos)
{
    int col = 0;

    int i = 0;
    for (i = 0; i<utf8_pos; i++) {
        col++;
        gchar *ch = g_utf8_offset_to_pointer(str, i);
        gunichar uni = g_utf8_get_char(ch);
        if (g_unichar_iswide(uni)) {
            col++;
        }
    }

    return col;
}

static GCompareFunc cmp_func;

void
glist_set_cmp(GCompareFunc func)
{
    cmp_func = func;
}

int
glist_contents_equal(const void *actual, const void *expected)
{
    GList *ac = (GList *)actual;
    GList *ex = (GList *)expected;

    GList *p = ex;
    printf("\nExpected\n");
    while(ex) {
        printf("\n\n%s\n", (char*)p->data);
        ex = g_list_next(ex);
    }
    printf("\n\n");
    p = ac;
    printf("\nActual\n");
    while(ac) {
        printf("\n\n%s\n", (char *)p->data);
        ac = g_list_next(ac);
    }
    printf("\n\n");

    if (g_list_length(ex) != g_list_length(ac)) {
        return 0;
    }

    GList *ex_curr = ex;
    while (ex_curr != NULL) {
        if (g_list_find_custom(ac, ex_curr->data, cmp_func) == NULL) {
            return 0;
        }
        ex_curr = g_list_next(ex_curr);
    }

    return 1;
}
