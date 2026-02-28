#ifndef TESTS_TEST_CMD_OTR_H
#define TESTS_TEST_CMD_OTR_H

#include "config.h"

#ifdef HAVE_LIBOTR
void cmd_otr_log__shows__usage_when_no_args(void** state);
void cmd_otr_log__shows__usage_when_invalid_subcommand(void** state);
void cmd_otr_log__updates__enables_logging(void** state);
void cmd_otr_log__updates__disables_logging(void** state);
void cmd_otr_log__updates__redacts_logging(void** state);
void cmd_otr_log__shows__warning_when_chlog_disabled(void** state);
void cmd_otr_log__shows__redact_warning_when_chlog_disabled(void** state);
void cmd_otr_libver__shows__libotr_version(void** state);
void cmd_otr_gen__shows__message_when_not_connected(void** state);
void cmd_otr_gen__tests__generates_key_for_connected_account(void** state);
void cmd_otr_gen__shows__message_when_disconnected(void** state);
void cmd_otr_gen__shows__message_when_undefined(void** state);
void cmd_otr_gen__shows__message_when_connecting(void** state);
void cmd_otr_gen__shows__message_when_disconnecting(void** state);
void cmd_otr_myfp__shows__message_when_disconnected(void** state);
void cmd_otr_myfp__shows__message_when_undefined(void** state);
void cmd_otr_myfp__shows__message_when_connecting(void** state);
void cmd_otr_myfp__shows__message_when_disconnecting(void** state);
void cmd_otr_myfp__shows__message_when_no_key(void** state);
void cmd_otr_myfp__shows__my_fingerprint(void** state);
void cmd_otr_theirfp__shows__message_when_in_console(void** state);
void cmd_otr_theirfp__shows__message_when_in_muc(void** state);
void cmd_otr_theirfp__shows__message_when_in_private(void** state);
void cmd_otr_theirfp__shows__message_when_non_otr_chat_window(void** state);
void cmd_otr_theirfp__shows__fingerprint(void** state);
void cmd_otr_start__shows__message_when_in_console(void** state);
void cmd_otr_start__shows__message_when_in_muc(void** state);
void cmd_otr_start__shows__message_when_in_private(void** state);
void cmd_otr_start__shows__message_when_already_started(void** state);
void cmd_otr_start__shows__message_when_no_key(void** state);
void cmd_otr_start__tests__sends_otr_query_message_to_current_recipeint(void** state);
#else
void cmd_otr__shows__message_when_otr_unsupported(void** state);
#endif

#endif
