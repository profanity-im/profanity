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
#include <sys/wait.h>
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

void
plugins_message_filter_chain(void** state)
{
    const char* plugin_dir = "./tests/functionaltests/files/xdg_data_home/profanity/plugins";
    g_mkdir_with_parents(plugin_dir, 0700);

    // Plugin 1: EARLY priority, transforms "apple" to "banana"
    const char* script_early = "prof_priority = prof.PRIORITY_EARLY\n"
                               "function prof_pre_chat_message_send(barejid, msg)\n"
                               "    local modified, count = string.gsub(msg, 'apple', 'banana')\n"
                               "    return modified\n"
                               "end\n";

    // Plugin 2: NORMAL priority, transforms "banana" to "cherry"
    const char* script_normal = "prof_priority = prof.PRIORITY_NORMAL\n"
                                "function prof_pre_chat_message_send(barejid, msg)\n"
                                "    local modified, count = string.gsub(msg, 'banana', 'cherry')\n"
                                "    return modified\n"
                                "end\n";

    char* path_early = g_build_filename(plugin_dir, "filter_early.lua", NULL);
    char* path_normal = g_build_filename(plugin_dir, "filter_normal.lua", NULL);
    g_file_set_contents(path_early, script_early, -1, NULL);
    g_file_set_contents(path_normal, script_normal, -1, NULL);
    g_free(path_early);
    g_free(path_normal);

    prof_input("/plugins load filter_early.lua");
    assert_true(prof_output_exact("Loaded plugin: filter_early.lua"));

    prof_input("/plugins load filter_normal.lua");
    assert_true(prof_output_exact("Loaded plugin: filter_normal.lua"));

    prof_connect();

    prof_input("/msg buddy@localhost");
    prof_input("I like apple");

    assert_true(stbbr_received(
        "<message id='*' to='buddy@localhost' type='chat'>"
        "<body>I like cherry</body>"
        "</message>"));

    assert_true(prof_output_exact("me: I like cherry"));

    prof_input("/win 1");

    prof_input("/plugins unload filter_early.lua");
    assert_true(prof_output_exact("Unloaded plugin: filter_early.lua"));

    prof_input("/plugins unload filter_normal.lua");
    assert_true(prof_output_exact("Unloaded plugin: filter_normal.lua"));
}

void
plugins_stanza_interception_and_drop(void** state)
{
    const char* plugin_dir = "./tests/functionaltests/files/xdg_data_home/profanity/plugins";
    g_mkdir_with_parents(plugin_dir, 0700);

    const char* script = "function prof_on_message_stanza_receive(stanza)\n"
                         "    if string.find(stanza, 'SPAM_BLOCK_ME') then\n"
                         "        prof.cons.show('Dropped spam stanza')\n"
                         "        return false\n"
                         "    end\n"
                         "    return true\n"
                         "end\n";

    char* script_path = g_build_filename(plugin_dir, "stanza_drop.lua", NULL);
    g_file_set_contents(script_path, script, -1, NULL);
    g_free(script_path);

    prof_input("/plugins load stanza_drop.lua");
    assert_true(prof_output_exact("Loaded plugin: stanza_drop.lua"));

    prof_connect();

    // 1. Send stanza matching the spam condition; plugin drops it
    stbbr_send(
        "<message id='spam1' to='stabber@localhost' from='spammer@localhost/bot' type='chat'>"
        "<body>SPAM_BLOCK_ME</body>"
        "</message>");

    assert_true(prof_output_exact("Dropped spam stanza"));

    // 2. Send normal stanza; plugin allows it, normal chat notification appears
    stbbr_send(
        "<message id='legit1' to='stabber@localhost' from='friend@localhost/home' type='chat'>"
        "<body>Legit message</body>"
        "</message>");

    assert_true(prof_output_regex("<< chat message: friend@localhost/home"));

    prof_input("/plugins unload stanza_drop.lua");
    assert_true(prof_output_exact("Unloaded plugin: stanza_drop.lua"));
}

void
plugins_disco_add_feature_advertised(void** state)
{
    const char* plugin_dir = "./tests/functionaltests/files/xdg_data_home/profanity/plugins";
    g_mkdir_with_parents(plugin_dir, 0700);

    const char* script = "function prof_init(version, status, account, fulljid)\n"
                         "    prof.disco.add_feature('urn:xmpp:test-feature-x')\n"
                         "end\n";

    char* script_path = g_build_filename(plugin_dir, "disco_test.lua", NULL);
    g_file_set_contents(script_path, script, -1, NULL);
    g_free(script_path);

    prof_input("/plugins load disco_test.lua");
    assert_true(prof_output_exact("Loaded plugin: disco_test.lua"));

    prof_connect();

    // Query Profanity for its disco#info features
    stbbr_send(
        "<iq id='disco1' to='stabber@localhost/profanity' from='buddy1@localhost/mobile' type='get'>"
        "<query xmlns='http://jabber.org/protocol/disco#info'/>"
        "</iq>");

    assert_true(stbbr_received(
        "<iq id='disco1' to='buddy1@localhost/mobile' type='result'>"
        "<query xmlns='http://jabber.org/protocol/disco#info'>"
        "<feature var='urn:xmpp:test-feature-x'/>"
        "</query>"
        "</iq>"));

    prof_input("/plugins unload disco_test.lua");
    assert_true(prof_output_exact("Unloaded plugin: disco_test.lua"));
}

void
plugins_window_lifecycle_and_input(void** state)
{
    const char* plugin_dir = "./tests/functionaltests/files/xdg_data_home/profanity/plugins";
    g_mkdir_with_parents(plugin_dir, 0700);

    const char* script = "function prof_init(version, status, account, fulljid)\n"
                         "    prof.register_command('/testcustomwin', 0, 0, cmd_open, '/testcustomwin', 'Open custom window')\n"
                         "end\n"
                         "function cmd_open()\n"
                         "    prof.win.create('customtag', on_win_input)\n"
                         "    prof.win.show('customtag', 'CUSTOM_WIN_STARTED')\n"
                         "    prof.win.focus('customtag')\n"
                         "end\n"
                         "function on_win_input(tag, line)\n"
                         "    prof.win.show(tag, 'ECHO: ' .. line)\n"
                         "end\n";

    char* script_path = g_build_filename(plugin_dir, "custom_win.lua", NULL);
    g_file_set_contents(script_path, script, -1, NULL);
    g_free(script_path);

    prof_input("/plugins load custom_win.lua");
    assert_true(prof_output_exact("Loaded plugin: custom_win.lua"));

    prof_input("/testcustomwin");
    assert_true(prof_output_exact("CUSTOM_WIN_STARTED"));

    prof_input("ping from test");
    assert_true(prof_output_exact("ECHO: ping from test"));

    prof_input("/win close");
    prof_input("/plugins unload custom_win.lua");
    assert_true(prof_output_exact("Unloaded plugin: custom_win.lua"));
}

void
plugins_settings_persistence(void** state)
{
    const char* plugin_dir = "./tests/functionaltests/files/xdg_data_home/profanity/plugins";
    g_mkdir_with_parents(plugin_dir, 0700);

    const char* script = "function prof_init(version, status, account, fulljid)\n"
                         "    prof.register_command('/setval', 1, 1, cmd_set, '/setval <val>', 'Set setting')\n"
                         "    prof.register_command('/getval', 0, 0, cmd_get, '/getval', 'Get setting')\n"
                         "end\n"
                         "function cmd_set(val)\n"
                         "    prof.settings.set('testgroup', 'mykey', val)\n"
                         "    prof.cons.show('Saved: ' .. val)\n"
                         "end\n"
                         "function cmd_get()\n"
                         "    local val = prof.settings.get('testgroup', 'mykey', 'missing')\n"
                         "    prof.cons.show('Loaded: ' .. val)\n"
                         "end\n";

    char* script_path = g_build_filename(plugin_dir, "settings_test.lua", NULL);
    g_file_set_contents(script_path, script, -1, NULL);
    g_free(script_path);

    prof_input("/plugins load settings_test.lua");
    assert_true(prof_output_exact("Loaded plugin: settings_test.lua"));

    // 1. Save setting
    prof_input("/setval persistent_data_42");
    assert_true(prof_output_exact("Saved: persistent_data_42"));

    // 2. Read setting in same session
    prof_input("/getval");
    assert_true(prof_output_exact("Loaded: persistent_data_42"));

    // 3. Unload plugin
    prof_input("/plugins unload settings_test.lua");
    assert_true(prof_output_exact("Unloaded plugin: settings_test.lua"));

    // 4. Reload plugin and verify setting was persisted
    prof_input("/plugins load settings_test.lua");
    assert_true(prof_output_exact("Loaded plugin: settings_test.lua"));

    prof_input("/getval");
    assert_true(prof_output_exact("Loaded: persistent_data_42"));

    prof_input("/plugins unload settings_test.lua");
    assert_true(prof_output_exact("Unloaded plugin: settings_test.lua"));
}

void
plugins_autocompletion(void** state)
{
    const char* plugin_dir = "./tests/functionaltests/files/xdg_data_home/profanity/plugins";
    g_mkdir_with_parents(plugin_dir, 0700);

    const char* script = "function prof_init(version, status, account, fulljid)\n"
                         "    prof.register_command('/actest', 0, 1, cmd_ac, '/actest [arg]', 'Autocomplete test')\n"
                         "    prof.completer.add('/actest', { 'banana', 'blueberry' })\n"
                         "end\n"
                         "function cmd_ac(arg)\n"
                         "    prof.cons.show('actest: ' .. (arg or 'none'))\n"
                         "end\n";

    char* script_path = g_build_filename(plugin_dir, "ac_test.lua", NULL);
    g_file_set_contents(script_path, script, -1, NULL);
    g_free(script_path);

    prof_input("/plugins load ac_test.lua");
    assert_true(prof_output_exact("Loaded plugin: ac_test.lua"));

    // Type partial subcommand and send TAB, then Enter
    prof_send_raw("/actest ban\t\n");
    assert_true(prof_output_exact("actest: banana"));

    prof_input("/plugins unload ac_test.lua");
    assert_true(prof_output_exact("Unloaded plugin: ac_test.lua"));
}

void
plugins_periodic_timer(void** state)
{
    const char* plugin_dir = "./tests/functionaltests/files/xdg_data_home/profanity/plugins";
    g_mkdir_with_parents(plugin_dir, 0700);

    const char* script = "local tick = 0\n"
                         "function prof_init(version, status, account, fulljid)\n"
                         "    prof.register_timed(on_timer, 1)\n"
                         "end\n"
                         "function on_timer()\n"
                         "    tick = tick + 1\n"
                         "    prof.cons.show('TIMER_TICK_' .. tostring(tick))\n"
                         "end\n";

    char* script_path = g_build_filename(plugin_dir, "timer_test.lua", NULL);
    g_file_set_contents(script_path, script, -1, NULL);
    g_free(script_path);

    prof_input("/plugins load timer_test.lua");
    assert_true(prof_output_exact("Loaded plugin: timer_test.lua"));

    // Wait for the 1-second timer to fire and render in the console
    assert_true(prof_output_exact("TIMER_TICK_1"));

    // Unload the plugin and verify timer is safely unregistered without crashing
    prof_input("/plugins unload timer_test.lua");
    assert_true(prof_output_exact("Unloaded plugin: timer_test.lua"));
}

void
plugins_error_handling_crash_immunity(void** state)
{
    const char* plugin_dir = "./tests/functionaltests/files/xdg_data_home/profanity/plugins";
    g_mkdir_with_parents(plugin_dir, 0700);

    const char* script = "function prof_init(version, status, account, fulljid)\n"
                         "    prof.register_command('/buggycmd', 0, 0, cmd_buggy, '/buggycmd', 'Buggy command')\n"
                         "    prof.register_command('/goodcmd', 0, 0, cmd_good, '/goodcmd', 'Good command')\n"
                         "end\n"
                         "function cmd_buggy()\n"
                         "    error('intentional plugin crash!')\n"
                         "end\n"
                         "function cmd_good()\n"
                         "    prof.cons.show('goodcmd executed successfully')\n"
                         "end\n";

    char* script_path = g_build_filename(plugin_dir, "error_test.lua", NULL);
    g_file_set_contents(script_path, script, -1, NULL);
    g_free(script_path);

    prof_input("/plugins load error_test.lua");
    assert_true(prof_output_exact("Loaded plugin: error_test.lua"));

    // Execute buggy command; must be safely caught by lua_pcall and logged to console
    prof_input("/buggycmd");
    assert_true(prof_output_regex("Plugin error in command .*intentional plugin crash!"));

    // Verify VM and client remain healthy by running subsequent command
    prof_input("/goodcmd");
    assert_true(prof_output_exact("goodcmd executed successfully"));

    prof_input("/plugins unload error_test.lua");
    assert_true(prof_output_exact("Unloaded plugin: error_test.lua"));
}

void
plugins_startup_autoloading(void** state)
{
    const char* plugin_dir = "./tests/functionaltests/files/xdg_data_home/profanity/plugins";
    g_mkdir_with_parents(plugin_dir, 0700);

    const char* script = "function prof_init(version, status, account, fulljid)\n"
                         "    prof.cons.show('STARTUP_PLUGIN_INITIALIZED')\n"
                         "    prof.register_command('/startupcmd', 0, 0, cmd_startup, '/startupcmd', 'Startup command')\n"
                         "end\n"
                         "function cmd_startup()\n"
                         "    prof.cons.show('startupcmd executed!')\n"
                         "end\n";

    const char* session_script = "function prof_init(version, status, account, fulljid)\n"
                                 "    prof.cons.show('SESSION_PLUGIN_INITIALIZED')\n"
                                 "end\n";

    char* script_path = g_build_filename(plugin_dir, "startup_test.lua", NULL);
    g_file_set_contents(script_path, script, -1, NULL);
    g_free(script_path);

    char* session_path = g_build_filename(plugin_dir, "session_test.lua", NULL);
    g_file_set_contents(session_path, session_script, -1, NULL);
    g_free(session_path);

    // 1. Load session_test.lua (session only) and enable startup_test.lua (persistent)
    prof_input("/plugins load session_test.lua");
    assert_true(prof_output_exact("Loaded plugin: session_test.lua"));

    prof_input("/plugins enable startup_test.lua");
    assert_true(prof_output_exact("Plugin enabled and loaded: startup_test.lua"));

    // 2. Quit Profanity
    prof_input("/quit");
    waitpid(prof_pid, NULL, 0);

    // 3. Restart Profanity with existing config and data dirs intact
    prof_start();
    usleep(1000 * 500);
    prof_expect_timeout = 30;
    assert_true(prof_output_regex("Profanity"));
    prof_input("/inpblock timeout 5");

    // 4. Assert that the enabled plugin was automatically loaded on startup
    assert_true(prof_output_exact("STARTUP_PLUGIN_INITIALIZED"));

    // 5. Assert that its registered command is immediately callable
    prof_input("/startupcmd");
    assert_true(prof_output_exact("startupcmd executed!"));

    // 6. Disable the plugin
    prof_input("/plugins disable startup_test.lua");
    assert_true(prof_output_exact("Plugin disabled and unloaded: startup_test.lua"));
}
