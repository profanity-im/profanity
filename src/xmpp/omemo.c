#include "xmpp/connection.h"
#include "xmpp/iq.h"
#include "xmpp/stanza.h"

void
omemo_devicelist_publish(void)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    char *barejid = xmpp_jid_bare(ctx, session_get_account_name());
    xmpp_stanza_t *iq = stanza_create_omemo_devicelist_subscription(ctx, barejid);
    iq_send_stanza(iq);
    xmpp_stanza_release(iq);

    free(barejid);
}
