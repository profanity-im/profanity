#include <glib.h>
#include "prof_cmocka.h"
#include <stdlib.h>
#include <string.h>

#include <stabber.h>
#include <expect.h>

#include "proftest.h"

void
message_send(void** state)
{
    prof_connect();

    prof_input("/msg somejid@someserver.com Hi there");

    assert_true(stbbr_received(
        "<message id='*' to='somejid@someserver.com' type='chat'>"
        "<body>Hi there</body>"
        "</message>"));

    assert_true(prof_output_regex("me: .+Hi there"));
}

void
message_receive_console(void** state)
{
    prof_connect();

    stbbr_send(
        "<message id='message1' to='stabber@localhost' from='someuser@chatserv.org/laptop' type='chat'>"
        "<body>How are you?</body>"
        "</message>");

    assert_true(prof_output_exact("<< chat message: someuser@chatserv.org/laptop (win 2)"));
}

void
message_receive_chatwin(void** state)
{
    prof_connect();

    prof_input("/msg someuser@chatserv.org");
    assert_true(prof_output_exact("someuser@chatserv.org"));

    stbbr_send(
        "<message id='message1' to='stabber@localhost' from='someuser@chatserv.org/laptop' type='chat'>"
        "<body>How are you?</body>"
        "</message>");

    assert_true(prof_output_regex("someuser@chatserv.org/laptop: .+How are you?"));
}
