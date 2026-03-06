#include <sys/stat.h>
#include <sys/wait.h>
#include <glib.h>

#include <stdlib.h>
#include "prof_cmocka.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pty.h>
#include <utmp.h>
#include <poll.h>
#include <stdarg.h>
#include <termios.h>

#include <stabber.h>

#include "proftest.h"

int prof_expect_timeout = 10;
int prof_pid = 0;

char* config_orig;
char* data_orig;

int current_port = 5230;

int fd = 0;

static GString* accumulated_output = NULL;

int
prof_expect(int fd, ...)
{
    va_list args;
    va_start(args, fd);

    enum prof_expect_type type = va_arg(args, enum prof_expect_type);
    if (type == prof_expect_end) {
        va_end(args);
        return 0;
    }

    char* pattern = va_arg(args, char*);
    int result_val = va_arg(args, int);
    va_end(args);

    if (accumulated_output == NULL) {
        accumulated_output = g_string_new("");
    }

    usleep(1000 * 100); // Wait 100ms for buffer to settle

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;

    GError *error = NULL;
    GRegex *ansi = g_regex_new("\\x1b\\[[0-9;]*[a-zA-Z]|\\x1b\\([a-zA-Z]", 0, 0, &error);
    if (error) {
        g_error_free(error);
        return 0;
    }
    GTimer* timer = g_timer_new();
    while (g_timer_elapsed(timer, NULL) < prof_expect_timeout) {
        // Check if pattern is already in accumulated output
        gchar* clean_output = g_regex_replace(ansi, accumulated_output->str, -1, 0, "", 0, NULL);

        if (type == prof_expect_exact) {
            gchar* escaped = g_regex_escape_string(pattern, -1);
            error = NULL;
            GRegex *regex = g_regex_new(escaped, G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, &error);
            if (error) {
                g_error_free(error);
                g_free(escaped);
                g_free(clean_output);
                g_timer_destroy(timer);
                g_regex_unref(ansi);
                return 0;
            }
            g_free(escaped);
            GMatchInfo *match_info;
            if (g_regex_match(regex, clean_output, 0, &match_info)) {
                g_string_truncate(accumulated_output, 0); 
                
                g_match_info_free(match_info);
                g_regex_unref(regex);
                g_free(clean_output);
                g_timer_destroy(timer);
                g_regex_unref(ansi);
                return result_val;
            }
            g_match_info_free(match_info);
            g_regex_unref(regex);
        } else if (type == prof_expect_regexp) {
            GMatchInfo *match_info;
            error = NULL;
            GRegex *regex = g_regex_new(pattern, G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, &error);
            if (error) {
                g_error_free(error);
                g_free(clean_output);
                g_timer_destroy(timer);
                g_regex_unref(ansi);
                return 0;
            }
            if (g_regex_match(regex, clean_output, 0, &match_info)) {
                gint start_pos, end_pos;
                g_match_info_fetch_pos(match_info, 0, &start_pos, &end_pos);
                
                g_string_truncate(accumulated_output, 0);
                
                g_match_info_free(match_info);
                g_regex_unref(regex);
                g_free(clean_output);
                g_timer_destroy(timer);
                g_regex_unref(ansi);
                return result_val;
            }
            g_match_info_free(match_info);
            g_regex_unref(regex);
        }
        
        g_free(clean_output);

        int ret = poll(&pfd, 1, 10); // 10ms timeout for poll
        if (ret > 0 && (pfd.revents & POLLIN)) {
            char buf[1024];
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                g_string_append(accumulated_output, buf);
            } else if (n == 0) {
                break; // EOF
            }
        } else {
            usleep(1000 * 5); // 5ms sleep if no data
        }
    }

    g_timer_destroy(timer);
    g_regex_unref(ansi);
    return 0; // Timeout or EOF without match
}

gboolean
_create_dir(const char* name)
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
_mkdir_recursive(const char* dir)
{
    int i;
    gboolean result = TRUE;

    for (i = 1; i <= strlen(dir); i++) {
        if (dir[i] == '/' || dir[i] == '\0') {
            gchar* next_dir = g_strndup(dir, i);
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
    GString* profanity_dir = g_string_new(XDG_CONFIG_HOME);
    g_string_append(profanity_dir, "/profanity");

    if (!_mkdir_recursive(profanity_dir->str)) {
        assert_true(FALSE);
    }

    g_string_free(profanity_dir, TRUE);
}

void
_create_data_dir(void)
{
    GString* profanity_dir = g_string_new(XDG_DATA_HOME);
    g_string_append(profanity_dir, "/profanity");

    if (!_mkdir_recursive(profanity_dir->str)) {
        assert_true(FALSE);
    }

    g_string_free(profanity_dir, TRUE);
}

void
_create_chatlogs_dir(void)
{
    GString* chatlogs_dir = g_string_new(XDG_DATA_HOME);
    g_string_append(chatlogs_dir, "/profanity/chatlogs");

    if (!_mkdir_recursive(chatlogs_dir->str)) {
        assert_true(FALSE);
    }

    g_string_free(chatlogs_dir, TRUE);
}

void
_create_logs_dir(void)
{
    GString* logs_dir = g_string_new(XDG_DATA_HOME);
    g_string_append(logs_dir, "/profanity/logs");

    if (!_mkdir_recursive(logs_dir->str)) {
        assert_true(FALSE);
    }

    g_string_free(logs_dir, TRUE);
}

void
_cleanup_dirs(void)
{
    system("rm -rf ./tests/functionaltests/files");
}

void
prof_start(void)
{
    if (accumulated_output) {
        g_string_free(accumulated_output, TRUE);
        accumulated_output = NULL;
    }

    // helper script sets terminal columns, avoids assertions failing
    // based on the test runner terminal size
    int master;
    pid_t pid = forkpty(&master, NULL, NULL, NULL);
    if (pid == 0) {
        system("stty sane -echo");
        execlp("sh", "sh", "-c", "./tests/functionaltests/start_profanity.sh", (char *)0);
        _exit(1);
    }
    prof_pid = pid;
    fd = master;
}

int
init_prof_test(void** state)
{
    const char *cfg = getenv("XDG_CONFIG_HOME");
    const char *dat = getenv("XDG_DATA_HOME");
    config_orig = cfg ? g_strdup(cfg) : NULL;
    data_orig = dat ? g_strdup(dat) : NULL;

    setenv("XDG_CONFIG_HOME", XDG_CONFIG_HOME, 1);
    setenv("XDG_DATA_HOME", XDG_DATA_HOME, 1);

    usleep(1000 * 2000); // 2s delay between tests to let OS recover
    
    int res = -1;
    int port = 0;
    for (int i = 0; i < 5; i++) {
        port = current_port + (i * 10);
        res = stbbr_start(STBBR_LOGDEBUG, &port, 0);
        if (res == 0 && port > 0) {
            break;
        }
    }

    if (res != 0 || port == 0) {
        assert_true(FALSE);
        return -1;
    }
    current_port = port;

    _cleanup_dirs();

    _create_config_dir();
    _create_data_dir();
    _create_chatlogs_dir();
    _create_logs_dir();

    prof_start();
    usleep(1000 * 500); // Wait for profanity to start
    prof_expect_timeout = 30;
    assert_true(prof_output_regex("Profanity"));

    // set UI options to make expect assertions faster and more reliable
    prof_input("/inpblock timeout 5");
    prof_output_regex("Input blocking set to 5 milliseconds");
    prof_input("/inpblock dynamic off");
    prof_output_regex("Dynamic input blocking disabled");
    prof_input("/notify chat off");
    prof_output_regex("Chat notifications disabled");
    prof_input("/notify room off");
    prof_output_regex("Room notifications disabled");
    prof_input("/wrap off");
    prof_output_regex("Word wrap disabled");
    prof_input("/roster hide");
    prof_output_regex("Roster disabled");
    prof_input("/occupants default hide");
    prof_output_regex("Occupant list disabled");
    prof_input("/time console off");
    prof_output_regex("Console time display");
    prof_input("/time chat off");
    prof_output_regex("Chat time display");
    prof_input("/time muc off");
    prof_output_regex("MUC time display");
    prof_input("/time config off");
    prof_output_regex("Config time display");
    prof_input("/time private off");
    prof_output_regex("Private chat time display");
    prof_input("/time xml off");
    prof_output_regex("XML Console time display");

    prof_expect_timeout = 10;
    return 0;
}

int
close_prof_test(void** state)
{
    prof_input("/quit");
    
    // Give it some time to quit gracefully
    int retries = 0;
    int status;
    while (retries < 50) { // 5 seconds
        pid_t res = waitpid(prof_pid, &status, WNOHANG);
        if (res == prof_pid) {
            break;
        }
        usleep(1000 * 100);
        retries++;
    }

    if (retries == 50) {
        kill(prof_pid, SIGKILL);
        waitpid(prof_pid, &status, 0);
    }

    _cleanup_dirs();

    if (config_orig) {
        setenv("XDG_CONFIG_HOME", config_orig, 1);
        g_free(config_orig);
        config_orig = NULL;
    } else {
        unsetenv("XDG_CONFIG_HOME");
    }

    if (data_orig) {
        setenv("XDG_DATA_HOME", data_orig, 1);
        g_free(data_orig);
        data_orig = NULL;
    } else {
        unsetenv("XDG_DATA_HOME");
    }

    stbbr_stop();
    usleep(1000 * 500); // 500ms delay after stop
    return 0;
}

void
prof_input(const char* input)
{
    GString* inp_str = g_string_new(input);
    g_string_append(inp_str, "\n");
    write(fd, inp_str->str, inp_str->len);
    tcdrain(fd);
    g_string_free(inp_str, TRUE);
    usleep(1000 * 100); // 100ms delay
}

int
prof_output_exact(const char* text)
{
    // Use regex to skip potential timestamps [HH:MM:SS] or character noise
    gchar* escaped = g_regex_escape_string(text, -1);
    gchar* pattern = g_strdup_printf(".*%s", escaped);
    int res = prof_expect(fd, prof_expect_regexp, pattern, 1, prof_expect_end);
    g_free(pattern);
    g_free(escaped);
    return res;
}

int
prof_output_regex(const char* text)
{
    gchar* pattern = g_strdup_printf(".*%s", text);
    int res = prof_expect(fd, prof_expect_regexp, pattern, 1, prof_expect_end);
    g_free(pattern);
    return res;
}

void
prof_connect_with_roster(const char* roster)
{
    GString* roster_str = g_string_new(
        "<iq type='result' to='stabber@localhost/profanity'>"
        "<query xmlns='jabber:iq:roster' ver='362'>");
    g_string_append(roster_str, roster);
    g_string_append(roster_str,
                    "</query>"
                    "</iq>");

    stbbr_for_query("jabber:iq:roster", roster_str->str);
    g_string_free(roster_str, TRUE);

    stbbr_for_id("presence:*",
                 "<presence id='prof_presence_*' lang='en' to='stabber@localhost/profanity' from='stabber@localhost/profanity'>"
                 "<priority>0</priority>"
                 "<c hash='sha-1' xmlns='http://jabber.org/protocol/caps' node='http://profanity-im.github.io/' ver='f8mrtdyAmhnj8Ca+630bThSL718='/>"
                 "</presence>");

    int port = current_port;
    gchar* connect_cmd = g_strdup_printf("/connect stabber@localhost/profanity server 127.0.0.1 port %d tls disable auth legacy", port);
    prof_input(connect_cmd);
    g_free(connect_cmd);

    assert_true(prof_output_regex("Enter password:"));
    usleep(1000 * 200); // 200ms delay
    prof_input("password");
    usleep(1000 * 200); // 200ms delay

    // Allow time for profanity to connect
    prof_expect_timeout = 30;
    assert_true(prof_output_regex("successfully"));
    prof_expect_timeout = 10;
    stbbr_wait_for("presence:*");
    //usleep(1000 * 2000); // 2s delay
}

void
prof_timeout(int timeout)
{
    prof_expect_timeout = timeout;
}

void
prof_timeout_reset(void)
{
    prof_expect_timeout = 10;
}

void
prof_connect(void)
{
    prof_connect_with_roster(
        "<item jid='buddy1@localhost' subscription='both' name='Buddy1'/>"
        "<item jid='buddy2@localhost' subscription='both' name='Buddy2'/>");
}
