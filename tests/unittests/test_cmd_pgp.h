#include "config.h"

#ifdef HAVE_LIBGPGME
void cmd_pgp_shows_usage_when_no_args(void **state);
#else
void cmd_pgp_shows_message_when_pgp_unsupported(void **state);
#endif
