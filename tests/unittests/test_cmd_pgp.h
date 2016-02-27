#include "prof_config.h"

#ifdef PROF_HAVE_LIBGPGME
void cmd_pgp_shows_usage_when_no_args(void **state);
void cmd_pgp_start_shows_message_when_disconnected(void **state);
void cmd_pgp_start_shows_message_when_disconnecting(void **state);
void cmd_pgp_start_shows_message_when_connecting(void **state);
void cmd_pgp_start_shows_message_when_undefined(void **state);
void cmd_pgp_start_shows_message_when_started(void **state);
void cmd_pgp_start_shows_message_when_no_arg_in_console(void **state);
void cmd_pgp_start_shows_message_when_no_arg_in_muc(void **state);
void cmd_pgp_start_shows_message_when_no_arg_in_mucconf(void **state);
void cmd_pgp_start_shows_message_when_no_arg_in_private(void **state);
void cmd_pgp_start_shows_message_when_no_arg_in_xmlconsole(void **state);
#else
void cmd_pgp_shows_message_when_pgp_unsupported(void **state);
#endif
