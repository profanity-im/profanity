#include <signal/signal_protocol.h>

#include "config/account.h"

void
omemo_init(ProfAccount *account)
{
	signal_context *global_context;
	signal_context_create(&global_context, NULL);
}
