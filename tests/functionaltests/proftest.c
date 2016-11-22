#include <sys/stat.h>
#include <glib.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <cmocka.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <stabber.h>
#include <expect.h>

#include "proftest.h"

char *config_orig;
char *data_orig;

int fd = 0;

gboolean
_create_dir(char *name)
{
    struct stat sb;

    if (stat(name, &sb) != 0) {
        if (errno != ENOENT || mkdir(name, S_IRWXU) != 0) {
            return FALSE;
        }
    } else {
        if ((sb.st_mode & S_IFDIR) != S_IFDIR) {
            return FALSE;
        }
    }

    return TRUE;
}

gboolean
_mkdir_recursive(const char *dir)
{
    int i;
    gboolean result = TRUE;

    for (i = 1; i <= strlen(dir); i++) {
        if (dir[i] == '/' || dir[i] == '\0') {
            gchar *next_dir = g_strndup(dir, i);
            result = _create_dir(next_dir);
            g_free(next_dir);
            if (!result) {
                break;
            }
        }
    }

    return result;
}

void
_create_config_dir(void)
{
    GString *profanity_dir = g_string_new(XDG_CONFIG_HOME);
    g_string_append(profanity_dir, "/profanity");

    if (!_mkdir_recursive(profanity_dir->str)) {
        assert_true(FALSE);
    }

    g_string_free(profanity_dir, TRUE);
}

void
_create_data_dir(void)
{
    GString *profanity_dir = g_string_new(XDG_DATA_HOME);
    g_string_append(profanity_dir, "/profanity");

    if (!_mkdir_recursive(profanity_dir->str)) {
        assert_true(FALSE);
    }

    g_string_free(profanity_dir, TRUE);
}

void
_create_chatlogs_dir(void)
{
    GString *chatlogs_dir = g_string_new(XDG_DATA_HOME);
    g_string_append(chatlogs_dir, "/profanity/chatlogs");

    if (!_mkdir_recursive(chatlogs_dir->str)) {
        assert_true(FALSE);
    }

    g_string_free(chatlogs_dir, TRUE);
}

void
_create_logs_dir(void)
{
    GString *logs_dir = g_string_new(XDG_DATA_HOME);
    g_string_append(logs_dir, "/profanity/logs");

    if (!_mkdir_recursive(logs_dir->str)) {
        assert_true(FALSE);
    }

    g_string_free(logs_dir, TRUE);
}

void
_cleanup_dirs(void)
{
    int res = system("rm -rf ./tests/functionaltests/files");
    if (res == -1) {
        assert_true(FALSE);
    }
}

void
prof_start(void)
{
    // helper script sets terminal columns, avoids assertions failing
    // based on the test runner terminal size
    fd = exp_spawnl("sh",
        "sh",
        "-c",
        "./tests/functionaltests/start_profanity.sh",
        NULL);
    FILE *fp = fdopen(fd, "r+");

    assert_true(fp != NULL);

    setbuf(fp, (char *)0);
}

void
init_prof_test(void **state)
{
    if (stbbr_start(STBBR_LOGDEBUG ,5230, 0) != 0) {
        assert_true(FALSE);
        return;
    }

    config_orig = getenv("XDG_CONFIG_HOME");
    data_orig = getenv("XDG_DATA_HOME");

    setenv("XDG_CONFIG_HOME", XDG_CONFIG_HOME, 1);
    setenv("XDG_DATA_HOME", XDG_DATA_HOME, 1);

    _cleanup_dirs();

    _create_config_dir();
    _create_data_dir();
    _create_chatlogs_dir();
    _create_logs_dir();

    prof_start();
    assert_true(prof_output_exact("Profanity"));

    // set UI options to make expect assertions faster and more reliable
    prof_input("/inpblock timeout 5");
    assert_true(prof_output_exact("Input blocking set to 5 milliseconds"));
    prof_input("/inpblock dynamic off");
    assert_true(prof_output_exact("Dynamic input blocking disabled"));
    prof_input("/notify chat off");
    assert_true(prof_output_exact("Chat notifications disabled"));
    prof_input("/notify room off");
    assert_true(prof_output_exact("Room notifications disabled"));
    prof_input("/wrap off");
    assert_true(prof_output_exact("Word wrap disabled"));
    prof_input("/roster hide");
    assert_true(prof_output_exact("Roster disabled"));
    prof_input("/occupants default hide");
    assert_true(prof_output_exact("Occupant list disabled"));
    prof_input("/time console off");
    prof_input("/time console off");
    assert_true(prof_output_exact("Console time display disabled."));
    prof_input("/time chat off");
    assert_true(prof_output_exact("Chat time display disabled."));
    prof_input("/time muc off");
    assert_true(prof_output_exact("MUC time display disabled."));
    prof_input("/time mucconfig off");
    assert_true(prof_output_exact("MUC config time display disabled."));
    prof_input("/time private off");
    assert_true(prof_output_exact("Private chat time display disabled."));
    prof_input("/time xml off");
    assert_true(prof_output_exact("XML Console time display disabled."));
}

void
close_prof_test(void **state)
{
    prof_input("/quit");
    waitpid(exp_pid, NULL, 0);
    _cleanup_dirs();

    setenv("XDG_CONFIG_HOME", config_orig, 1);
    setenv("XDG_DATA_HOME", data_orig, 1);

    stbbr_stop();
}

void
prof_input(char *input)
{
    GString *inp_str = g_string_new(input);
    g_string_append(inp_str, "\r");
    write(fd, inp_str->str, inp_str->len);
    g_string_free(inp_str, TRUE);
}

int
prof_output_exact(char *text)
{
    return (1 == exp_expectl(fd, exp_exact, text, 1, exp_end));
}

int
prof_output_regex(char *text)
{
    return (1 == exp_expectl(fd, exp_regexp, text, 1, exp_end));
}

void
prof_connect_with_roster(char *roster)
{
    GString *roster_str = g_string_new(
        "<iq type='result' to='stabber@localhost/profanity'>"
            "<query xmlns='jabber:iq:roster' ver='362'>"
    );
    g_string_append(roster_str, roster);
    g_string_append(roster_str,
            "</query>"
        "</iq>"
    );

    stbbr_for_query("jabber:iq:roster", roster_str->str);
    g_string_free(roster_str, TRUE);

    stbbr_for_id("prof_presence_1",
        "<presence id='prof_presence_1' lang='en' to='stabber@localhost/profanity' from='stabber@localhost/profanity'>"
            "<priority>0</priority>"
            "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' node='http://www.profanity.im' ver='f8mrtdyAmhnj8Ca+630bThSL718='/>"
        "</presence>"
    );

    prof_input("/connect stabber@localhost server 127.0.0.1 port 5230 tls allow");
    prof_input("password");

    // Allow time for profanity to connect
    exp_timeout = 30;
    assert_true(prof_output_regex("stabber@localhost/profanity logged in successfully, .+online.+ \\(priority 0\\)\\."));
    exp_timeout = 10;
    stbbr_wait_for("prof_presence_*");
}

void
prof_timeout(int timeout)
{
    exp_timeout = timeout;
}

void
prof_timeout_reset(void)
{
    exp_timeout = 10;
}

void
prof_connect(void)
{
    prof_connect_with_roster(
        "<item jid='buddy1@localhost' subscription='both' name='Buddy1'/>"
        "<item jid='buddy2@localhost' subscription='both' name='Buddy2'/>"
    );
}
