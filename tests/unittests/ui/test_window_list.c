#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "prof_cmocka.h"
#include "ui/window.h"
#include "ui/window_list.h"
#include "ui/buffer.h"
#include "tests/unittests/ui/stub_ui.h"
#include "tests/unittests/ui/test_window_list.h"

static ProfBuffEntry*
_create_test_entry(const char* message)
{
    ProfBuffEntry* e = g_new0(ProfBuffEntry, 1);
    e->message = g_strdup(message);
    return e;
}

static void
_free_test_entry(ProfBuffEntry* e)
{
    if (e) {
        g_free(e->message);
        g_free(e);
    }
}

void
wins_get_url__returns__null_when_buffer_empty(void** state)
{
    ProfWin win;
    memset(&win, 0, sizeof(ProfWin));
    win.type = WIN_CHAT;

    ProfLayoutSimple layout;
    memset(&layout, 0, sizeof(ProfLayoutSimple));
    layout.base.buffer = (ProfBuff)0x1;
    win.layout = &layout.base;

    wins_reset_url_ac();
    stub_ui_set_mock_buffer_entries(NULL);

    char* url = wins_get_url("", FALSE, &win);
    assert_null(url);

    wins_reset_url_ac();
}

void
wins_get_url__returns__urls_from_newest_to_oldest(void** state)
{
    ProfWin win;
    memset(&win, 0, sizeof(ProfWin));
    win.type = WIN_CHAT;

    ProfLayoutSimple layout;
    memset(&layout, 0, sizeof(ProfLayoutSimple));
    layout.base.buffer = (ProfBuff)0x1;
    win.layout = &layout.base;

    wins_reset_url_ac();
    GList* entries = NULL;
    entries = g_list_append(entries, _create_test_entry("Old message with https://profanity-im.github.io"));
    entries = g_list_append(entries, _create_test_entry("New message with https://xmpp.org and https://jabber.org"));
    stub_ui_set_mock_buffer_entries(entries);

    // 1st Tab: right-most URL of the newest message
    char* url1 = wins_get_url("", FALSE, &win);
    assert_non_null(url1);
    assert_string_equal(url1, "https://jabber.org");

    // 2nd Tab: first URL of the newest message
    char* url2 = wins_get_url("", FALSE, &win);
    assert_non_null(url2);
    assert_string_equal(url2, "https://xmpp.org");

    // 3rd Tab: URL from the older message
    char* url3 = wins_get_url("", FALSE, &win);
    assert_non_null(url3);
    assert_string_equal(url3, "https://profanity-im.github.io");

    wins_reset_url_ac();
    g_list_free_full(entries, (GDestroyNotify)_free_test_entry);
    stub_ui_set_mock_buffer_entries(NULL);
}

void
wins_get_url__returns__deduplicated_urls_favoring_newest(void** state)
{
    ProfWin win;
    memset(&win, 0, sizeof(ProfWin));
    win.type = WIN_CHAT;

    ProfLayoutSimple layout;
    memset(&layout, 0, sizeof(ProfLayoutSimple));
    layout.base.buffer = (ProfBuff)0x1;
    win.layout = &layout.base;

    wins_reset_url_ac();
    GList* entries = NULL;
    entries = g_list_append(entries, _create_test_entry("Old: https://example.com/a"));
    entries = g_list_append(entries, _create_test_entry("Middle: https://example.com/b"));
    entries = g_list_append(entries, _create_test_entry("New: https://example.com/a"));
    stub_ui_set_mock_buffer_entries(entries);

    // 1st Tab: newest URL is /a
    char* url1 = wins_get_url("", FALSE, &win);
    assert_non_null(url1);
    assert_string_equal(url1, "https://example.com/a");

    // 2nd Tab: next URL is /b
    char* url2 = wins_get_url("", FALSE, &win);
    assert_non_null(url2);
    assert_string_equal(url2, "https://example.com/b");

    // 3rd Tab: /a is already seen, wraps to 1st or NULL
    char* url3 = wins_get_url("", FALSE, &win);
    assert_string_equal(url3, "https://example.com/a");

    wins_reset_url_ac();
    g_list_free_full(entries, (GDestroyNotify)_free_test_entry);
    stub_ui_set_mock_buffer_entries(NULL);
}

void
wins_get_url__returns__match_by_prefix(void** state)
{
    ProfWin win;
    memset(&win, 0, sizeof(ProfWin));
    win.type = WIN_CHAT;

    ProfLayoutSimple layout;
    memset(&layout, 0, sizeof(ProfLayoutSimple));
    layout.base.buffer = (ProfBuff)0x1;
    win.layout = &layout.base;

    wins_reset_url_ac();
    GList* entries = NULL;
    entries = g_list_append(entries, _create_test_entry("Links: https://alpha.org and https://beta.org"));
    stub_ui_set_mock_buffer_entries(entries);

    char* url = wins_get_url("https://b", FALSE, &win);
    assert_non_null(url);
    assert_string_equal(url, "https://beta.org");

    wins_reset_url_ac();
    g_list_free_full(entries, (GDestroyNotify)_free_test_entry);
    stub_ui_set_mock_buffer_entries(NULL);
}

void
wins_get_url__returns__urls_in_console_window(void** state)
{
    ProfWin win;
    memset(&win, 0, sizeof(ProfWin));
    win.type = WIN_CONSOLE;

    ProfLayoutSimple layout;
    memset(&layout, 0, sizeof(ProfLayoutSimple));
    layout.base.buffer = (ProfBuff)0x1;
    win.layout = &layout.base;

    wins_reset_url_ac();
    GList* entries = NULL;
    entries = g_list_append(entries, _create_test_entry("Console announcement: visit https://profanity-im.github.io/news"));
    stub_ui_set_mock_buffer_entries(entries);

    char* url = wins_get_url("", FALSE, &win);
    assert_non_null(url);
    assert_string_equal(url, "https://profanity-im.github.io/news");

    wins_reset_url_ac();
    g_list_free_full(entries, (GDestroyNotify)_free_test_entry);
    stub_ui_set_mock_buffer_entries(NULL);
}

void
wins_reset_url_ac__updates__clears_cached_urls(void** state)
{
    ProfWin win;
    memset(&win, 0, sizeof(ProfWin));
    win.type = WIN_CHAT;

    ProfLayoutSimple layout;
    memset(&layout, 0, sizeof(ProfLayoutSimple));
    layout.base.buffer = (ProfBuff)0x1;
    win.layout = &layout.base;

    wins_reset_url_ac();
    GList* entries = NULL;
    entries = g_list_append(entries, _create_test_entry("Old: https://example.com/one"));
    stub_ui_set_mock_buffer_entries(entries);

    // Initial completion caches the URL
    char* url = wins_get_url("", FALSE, &win);
    assert_non_null(url);
    assert_string_equal(url, "https://example.com/one");

    // Add a newer entry to the buffer
    entries = g_list_append(entries, _create_test_entry("New: https://example.com/two"));
    stub_ui_set_mock_buffer_entries(entries);

    // Without reset, it continues cycling the old cached list
    char* url_cached = wins_get_url("", FALSE, &win);
    assert_string_equal(url_cached, "https://example.com/one");

    // After reset, next completion rescans the buffer and picks up the new URL
    wins_reset_url_ac();
    char* url_fresh = wins_get_url("", FALSE, &win);
    assert_non_null(url_fresh);
    assert_string_equal(url_fresh, "https://example.com/two");

    wins_reset_url_ac();
    g_list_free_full(entries, (GDestroyNotify)_free_test_entry);
    stub_ui_set_mock_buffer_entries(NULL);
}
