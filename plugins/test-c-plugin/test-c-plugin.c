#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <profapi.h>

void
cmd_c(char **args)
{
    if (args[0] != NULL) {
        char *start = "c-test: /c command called, arg = ";
        char buf[strlen(start) + strlen(args[0]) + 1];
        sprintf(buf, "%s%s", start, args[0]);
        prof_cons_show(buf);
    } else {
        prof_cons_show("c-test: /c command called with no arg");
    }
    prof_cons_alert();
    prof_notify("c-test: notify", 2000, "Plugins");
    prof_send_line("/about");
    prof_cons_show("c-test: sent \"/about\" command");
}

void
timer_test(void)
{
    prof_cons_show("c-test: timer fired.");
    char *recipient = prof_get_current_recipient();
    if (recipient != NULL) {
        char *start = "  current recipient = ";
        char buf[strlen(start) + strlen(recipient) + 1];
        sprintf(buf, "%s%s", start, recipient);
        prof_cons_show(buf);
    }
    prof_cons_alert();
}

void
prof_init(const char * const version, const char * const status)
{
    char *start = "c-test: init. ";
    char buf[strlen(start) + strlen(version) + 2 + strlen(status) + 1];
    sprintf(buf, "%s%s, %s", start, version, status);
    prof_cons_show(buf);
    prof_register_command("/c", 0, 1, "/c", "c test", "c test", cmd_c);
    prof_register_timed(timer_test, 10);
}

void
prof_on_start(void)
{
    prof_cons_show("c-test: on_start");
}

void
prof_on_connect(const char * const account_name, const char * const fulljid)
{
    char *start = "c-test: on_connect, ";
    char buf[strlen(start) + strlen(account_name) + 2 + strlen(fulljid) + 1];
    sprintf(buf, "%s%s, %s", start, account_name, fulljid);
    prof_cons_show(buf);
}

char *
prof_on_message_received(const char * const jid, const char *message)
{
    char *start = "c-test: on_message_received, ";
    char buf[strlen(start) + strlen(jid) + 2 + strlen(message) + 1];
    sprintf(buf, "%s%s, %s", start, jid, message);
    prof_cons_show(buf);
    prof_cons_alert();
    char *result = malloc(strlen(message) + 4);
    sprintf(result, "%s%s", message, "[C]");

    return result;
}

char *
prof_on_message_send(const char * const jid, const char *message)
{
    char *start = "c-test: on_message_send, ";
    char buf[strlen(start) + strlen(jid) + 2 + strlen(message) + 1];
    sprintf(buf, "%s%s, %s", start, jid, message);
    prof_cons_show(buf);
    prof_cons_alert();
    char *result = malloc(strlen(message) + 4);
    sprintf(result, "%s%s", message, "[C]");

    return result;
}
