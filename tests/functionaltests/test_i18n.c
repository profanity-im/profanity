/*
 * test_i18n.c
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <glib.h>
#include "prof_cmocka.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <stabber.h>

#include "proftest.h"

void
i18n_msg_nickname(void** state)
{
    // Connect with a roster containing a UTF-8 nickname: Σωκράτης (Socrates)
    prof_connect_with_roster(
        "<item jid='socrates@localhost' subscription='both' name='Σωκράτης'/>"
        "<item jid='buddy2@localhost' subscription='both' name='Buddy2'/>");

    // Test that we can /msg the UTF-8 nickname successfully.
    // This verifies that the UI and command parsing handle UTF-8.
    prof_input("/msg Σωκράτης Hello");

    // Check if we switched to the socrates window
    assert_true(prof_output_regex("socrates@localhost"));
}

void
i18n_win_nickname(void** state)
{
    prof_connect_with_roster(
        "<item jid='socrates@localhost' subscription='both' name='Σωκράτης'/>");

    // Open a chat with Socrates
    prof_input("/msg Σωκράτης");
    assert_true(prof_output_regex("socrates@localhost"));

    // Switch back to console
    prof_input("/win 1");
    assert_true(prof_output_regex("Console"));

    // Switch to Socrates window using name
    prof_input("/win Σωκράτης");
    assert_true(prof_output_regex("socrates@localhost"));
}

void
i18n_autocomplete_tab_utf8(void** state)
{
    prof_connect_with_roster(
        "<item jid='socrates@localhost' subscription='both' name='Σωκράτης'/>");

    prof_send_raw("/msg Σω");
    // TAB for completion
    prof_send_raw("\t");
    prof_input(" Hello");

    // If autocompletion worked, we should be in the Socrates window
    assert_true(prof_output_regex("socrates@localhost"));

    // And stabber should have received the message
    assert_true(stbbr_received(
        "<message to='socrates@localhost' id='*' type='chat'>"
        "<body>Hello</body>"
        "</message>"));
}

void
i18n_autocomplete_tab_latin(void** state)
{
    prof_connect_with_roster(
        "<item jid='plato@localhost' subscription='both' name='Plato'/>");

    prof_send_raw("/msg Pl");
    prof_send_raw("\t");
    prof_input(" Hello");

    assert_true(prof_output_regex("plato@localhost"));

    assert_true(stbbr_received(
        "<message to='plato@localhost' id='*' type='chat'>"
        "<body>Hello</body>"
        "</message>"));
}
