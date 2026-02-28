#ifndef TESTS_TEST_CMD_PGP_H
#define TESTS_TEST_CMD_PGP_H

#include "config.h"

#ifdef HAVE_LIBGPGME
void cmd_pgp__shows__usage_when_no_args(void** state);
void cmd_pgp_start__shows__message_when_disconnected(void** state);
void cmd_pgp_start__shows__message_when_disconnecting(void** state);
void cmd_pgp_start__shows__message_when_connecting(void** state);
void cmd_pgp_start__shows__message_when_undefined(void** state);
void cmd_pgp_start__shows__message_when_no_arg_in_console(void** state);
void cmd_pgp_start__shows__message_when_no_arg_in_muc(void** state);
void cmd_pgp_start__shows__message_when_no_arg_in_conf(void** state);
void cmd_pgp_start__shows__message_when_no_arg_in_private(void** state);
void cmd_pgp_start__shows__message_when_no_arg_in_xmlconsole(void** state);
#else
void cmd_pgp__shows__message_when_pgp_unsupported(void** state);
#endif

#endif
