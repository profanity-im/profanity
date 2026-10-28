/*
 * test_plugins.c
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "prof_cmocka.h"
#include <stabber.h>

#include "proftest.h"
#include "test_plugins.h"

void
plugins_load_and_run_command(void** state)
{
    const char* plugin_dir = "./tests/functionaltests/files/xdg_data_home/profanity/plugins";
    g_mkdir_with_parents(plugin_dir, 0700);

    const char* script = "function prof_init(version, status, account, fulljid)\n"
                         "    prof.register_command('/simplecmd', 0, 1, cmd_handler, '/simplecmd', 'Simple test command')\n"
                         "end\n"
                         "function cmd_handler(arg)\n"
                         "    prof.cons.show('simplecmd received: ' .. (arg or 'none'))\n"
                         "end\n";

    char* script_path = g_build_filename(plugin_dir, "simple_test.lua", NULL);
    g_file_set_contents(script_path, script, -1, NULL);
    g_free(script_path);

    prof_input("/plugins load simple_test.lua");
    assert_true(prof_output_exact("Loaded plugin: simple_test.lua"));

    prof_input("/simplecmd hello");
    assert_true(prof_output_exact("simplecmd received: hello"));

    prof_input("/plugins unload simple_test.lua");
    assert_true(prof_output_exact("Unloaded plugin: simple_test.lua"));
}

void
plugins_priority_ordering(void** state)
{
    const char* plugin_dir = "./tests/functionaltests/files/xdg_data_home/profanity/plugins";
    g_mkdir_with_parents(plugin_dir, 0700);

    // plugin_default.lua has no prof_priority, auto-assigned PRIORITY_NORMAL (100)
    const char* script_default = "function prof_on_win_open(title, win_type)\n"
                                 "    prof.cons.show('[ORDER: NORMAL]')\n"
                                 "end\n";

    // plugin_early.lua explicitly sets PRIORITY_EARLY (1)
    const char* script_early = "prof_priority = prof.PRIORITY_EARLY\n"
                               "function prof_on_win_open(title, win_type)\n"
                               "    prof.cons.show('[ORDER: EARLY]')\n"
                               "end\n";

    char* path_default = g_build_filename(plugin_dir, "plugin_default.lua", NULL);
    char* path_early = g_build_filename(plugin_dir, "plugin_early.lua", NULL);
    g_file_set_contents(path_default, script_default, -1, NULL);
    g_file_set_contents(path_early, script_early, -1, NULL);
    g_free(path_default);
    g_free(path_early);

    // Load default first, then early
    prof_input("/plugins load plugin_default.lua");
    assert_true(prof_output_exact("Loaded plugin: plugin_default.lua"));

    prof_input("/plugins load plugin_early.lua");
    assert_true(prof_output_exact("Loaded plugin: plugin_early.lua"));

    // Open xmlconsole window to trigger prof_on_win_open hook, then switch back to console
    prof_input("/xmlconsole");
    prof_input("/win 1");
    assert_true(prof_output_regex("\\[ORDER: EARLY\\].*\\[ORDER: NORMAL\\]"));

    prof_input("/plugins unload plugin_early.lua");
    assert_true(prof_output_exact("Unloaded plugin: plugin_early.lua"));

    prof_input("/plugins unload plugin_default.lua");
    assert_true(prof_output_exact("Unloaded plugin: plugin_default.lua"));
}

void
plugins_priority_alphabetical_tie_break(void** state)
{
    const char* plugin_dir = "./tests/functionaltests/files/xdg_data_home/profanity/plugins";
    g_mkdir_with_parents(plugin_dir, 0700);

    // Both plugins have identical default NORMAL priority
    const char* script_beta = "function prof_on_win_open(title, win_type)\n"
                              "    prof.cons.show('[TIE: BETA]')\n"
                              "end\n";

    const char* script_alpha = "function prof_on_win_open(title, win_type)\n"
                               "    prof.cons.show('[TIE: ALPHA]')\n"
                               "end\n";

    char* path_beta = g_build_filename(plugin_dir, "plugin_beta.lua", NULL);
    char* path_alpha = g_build_filename(plugin_dir, "plugin_alpha.lua", NULL);
    g_file_set_contents(path_beta, script_beta, -1, NULL);
    g_file_set_contents(path_alpha, script_alpha, -1, NULL);
    g_free(path_beta);
    g_free(path_alpha);

    // Load beta first, alpha second to verify alphabetical tie-breaking overrides insertion order
    prof_input("/plugins load plugin_beta.lua");
    assert_true(prof_output_exact("Loaded plugin: plugin_beta.lua"));

    prof_input("/plugins load plugin_alpha.lua");
    assert_true(prof_output_exact("Loaded plugin: plugin_alpha.lua"));

    // Open xmlconsole to trigger prof_on_win_open hook, then switch back to console
    prof_input("/xmlconsole");
    prof_input("/win 1");
    assert_true(prof_output_regex("\\[TIE: ALPHA\\].*\\[TIE: BETA\\]"));

    prof_input("/plugins unload plugin_alpha.lua");
    assert_true(prof_output_exact("Unloaded plugin: plugin_alpha.lua"));

    prof_input("/plugins unload plugin_beta.lua");
    assert_true(prof_output_exact("Unloaded plugin: plugin_beta.lua"));
}
