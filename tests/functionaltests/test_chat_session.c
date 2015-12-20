#include <glib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include <stabber.h>
#include <expect.h>

#include "proftest.h"

void
sends_message_to_barejid_when_contact_offline(void **state)
{
    prof_connect();

    prof_input("/msg buddy1@localhost Hi there");

    assert_true(stbbr_received(
        "<message id=\"*\" to=\"buddy1@localhost\" type=\"chat\">"
            "<body>Hi there</body>"
        "</message>"
    ));
}

void
sends_message_to_barejid_when_contact_online(void **state)
{
    prof_connect();

    stbbr_send(
        "<presence to=\"stabber@localhost/profanity\" from=\"buddy1@localhost/mobile\">"
            "<priority>10</priority>"
        "</presence>"
    );
    assert_true(prof_output_exact("Buddy1 (mobile) is online"));

    prof_input("/msg buddy1@localhost Hi there");

    assert_true(stbbr_received(
        "<message id=\"*\" to=\"buddy1@localhost\" type=\"chat\">"
            "<body>Hi there</body>"
        "</message>"
    ));
}

void
sends_message_to_fulljid_when_received_from_fulljid(void **state)
{
    prof_connect();

    stbbr_send(
        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\">"
            "<priority>10</priority>"
        "</presence>"
    );
    assert_true(prof_output_exact("Buddy1 (mobile) is online"));

    stbbr_send(
        "<message id=\"message1\" to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\" type=\"chat\">"
            "<body>First message</body>"
        "</message>"
    );
    assert_true(prof_output_exact("<< chat message: Buddy1/mobile (win 2)"));

    prof_input("/msg buddy1@localhost Hi there");

    assert_true(stbbr_received(
        "<message id=\"*\" to=\"buddy1@localhost/mobile\" type=\"chat\">"
            "<body>Hi there</body>"
        "</message>"
    ));
}

void
sends_subsequent_messages_to_fulljid(void **state)
{
    prof_connect();

    stbbr_send(
        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\">"
            "<priority>10</priority>"
        "</presence>"
    );
    assert_true(prof_output_exact("Buddy1 (mobile) is online"));

    stbbr_send(
        "<message id=\"message1\" to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\" type=\"chat\">"
            "<body>First message</body>"
        "</message>"
    );
    assert_true(prof_output_exact("<< chat message: Buddy1/mobile (win 2)"));

    prof_input("/msg buddy1@localhost Outgoing 1");
    assert_true(stbbr_received(
        "<message id=\"*\" to=\"buddy1@localhost/mobile\" type=\"chat\">"
            "<body>Outgoing 1</body>"
        "</message>"
    ));

    prof_input("/msg buddy1@localhost Outgoing 2");
    assert_true(stbbr_received(
        "<message id=\"*\" to=\"buddy1@localhost/mobile\" type=\"chat\">"
            "<body>Outgoing 2</body>"
        "</message>"
    ));

    prof_input("/msg buddy1@localhost Outgoing 3");
    assert_true(stbbr_received(
        "<message id=\"*\" to=\"buddy1@localhost/mobile\" type=\"chat\">"
            "<body>Outgoing 3</body>"
        "</message>"
    ));
}

void
resets_to_barejid_after_presence_received(void **state)
{
    prof_connect();

    stbbr_send(
        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\">"
            "<priority>10</priority>"
        "</presence>"
    );
    assert_true(prof_output_exact("Buddy1 (mobile) is online"));

    stbbr_send(
        "<message id=\"message1\" to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\" type=\"chat\">"
            "<body>First message</body>"
        "</message>"
    );
    assert_true(prof_output_exact("<< chat message: Buddy1/mobile (win 2)"));

    prof_input("/msg buddy1@localhost Outgoing 1");
    assert_true(stbbr_received(
        "<message id=\"*\" to=\"buddy1@localhost/mobile\" type=\"chat\">"
            "<body>Outgoing 1</body>"
        "</message>"
    ));

    stbbr_send(
        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/laptop\">"
            "<priority>5</priority>"
            "<show>dnd</show>"
        "</presence>"
    );
    assert_true(prof_output_exact("Buddy1 (laptop) is dnd"));

    prof_input("/msg buddy1@localhost Outgoing 2");
    assert_true(stbbr_received(
        "<message id=\"*\" to=\"buddy1@localhost\" type=\"chat\">"
            "<body>Outgoing 2</body>"
        "</message>"
    ));
}

void
new_session_when_message_received_from_different_fulljid(void **state)
{
    prof_connect();

    stbbr_send(
        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\">"
            "<priority>10</priority>"
        "</presence>"
    );
    assert_true(prof_output_exact("Buddy1 (mobile) is online"));

    stbbr_send(
        "<presence to=\"stabber@localhost\" from=\"buddy1@localhost/laptop\">"
            "<priority>8</priority>"
            "<show>away</show>"
        "</presence>"
    );
    assert_true(prof_output_exact("Buddy1 (laptop) is away"));

    stbbr_send(
        "<message id=\"message1\" to=\"stabber@localhost\" from=\"buddy1@localhost/mobile\" type=\"chat\">"
            "<body>From first resource</body>"
        "</message>"
    );
    assert_true(prof_output_exact("<< chat message: Buddy1/mobile (win 2)"));

    prof_input("/msg buddy1@localhost Outgoing 1");
    assert_true(stbbr_received(
        "<message id=\"*\" to=\"buddy1@localhost/mobile\" type=\"chat\">"
            "<body>Outgoing 1</body>"
        "</message>"
    ));

    stbbr_send(
        "<message id=\"message1\" to=\"stabber@localhost\" from=\"buddy1@localhost/laptop\" type=\"chat\">"
            "<body>From second resource</body>"
        "</message>"
    );
    assert_true(prof_output_regex("Buddy1/laptop:.+From second resource"));

    prof_input("/msg buddy1@localhost Outgoing 2");
    assert_true(stbbr_received(
        "<message id=\"*\" to=\"buddy1@localhost/laptop\" type=\"chat\">"
            "<body>Outgoing 2</body>"
        "</message>"
    ));
}
