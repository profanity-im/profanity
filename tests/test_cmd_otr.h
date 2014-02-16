#include "config.h"

#ifdef HAVE_LIBOTR
void cmd_otr_shows_usage_when_no_args(void **state);
#else
void cmd_otr_shows_message_when_otr_unsupported(void **state);
#endif
