#include "prof_cmocka.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config/preferences.h"

void
prefs_get_string__returns__all_for_console_default(void** state)
{
    gchar* setting = prefs_get_string(PREF_STATUSES_CONSOLE);

    assert_non_null(setting);
    assert_string_equal("all", setting);
    g_free(setting);
}

void
prefs_get_string__returns__none_for_chat_default(void** state)
{
    gchar* setting = prefs_get_string(PREF_STATUSES_CHAT);

    assert_non_null(setting);
    assert_string_equal("none", setting);
    g_free(setting);
}

void
prefs_get_string__returns__none_for_muc_default(void** state)
{
    gchar* setting = prefs_get_string(PREF_STATUSES_MUC);

    assert_non_null(setting);
    assert_string_equal("none", setting);
    g_free(setting);
}
