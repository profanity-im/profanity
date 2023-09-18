#include <strophe.h>

typedef int (*ProfIqCallback)(xmpp_stanza_t* const stanza, void* const userdata);
typedef void (*ProfIqFreeCallback)(void* userdata);

void
iq_send_stanza(xmpp_stanza_t* const stanza)
{
}

void
iq_id_handler_add(const char* const id, ProfIqCallback func, ProfIqFreeCallback free_func, void* userdata)
{
}