#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config/preferences.h"

void
statuses_console_defaults_to_all(void** state)
{
    gchar* setting = prefs_get_string(PREF_STATUSES_CONSOLE);

    assert_non_null(setting);
    assert_string_equal("all", setting);
    g_free(setting);
}

void
statuses_chat_defaults_to_all(void** state)
{
    gchar* setting = prefs_get_string(PREF_STATUSES_CHAT);

    assert_non_null(setting);
    assert_string_equal("none", setting);
    g_free(setting);
}

void
statuses_muc_defaults_to_all(void** state)
{
    gchar* setting = prefs_get_string(PREF_STATUSES_MUC);

    assert_non_null(setting);
    assert_string_equal("none", setting);
    g_free(setting);
}
