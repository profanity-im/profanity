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

    const char* script =
        "function prof_init(version, status, account, fulljid)\n"
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
