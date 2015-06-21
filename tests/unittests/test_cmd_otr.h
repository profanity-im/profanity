#include "prof_config.h"

#ifdef PROF_HAVE_LIBOTR
void cmd_otr_shows_usage_when_no_args(void **state);
void cmd_otr_shows_usage_when_invalid_subcommand(void **state);
void cmd_otr_log_shows_usage_when_no_args(void **state);
void cmd_otr_log_shows_usage_when_invalid_subcommand(void **state);
void cmd_otr_log_on_enables_logging(void **state);
void cmd_otr_log_off_disables_logging(void **state);
void cmd_otr_redact_redacts_logging(void **state);
void cmd_otr_log_on_shows_warning_when_chlog_disabled(void **state);
void cmd_otr_log_redact_shows_warning_when_chlog_disabled(void **state);
void cmd_otr_libver_shows_libotr_version(void **state);
void cmd_otr_gen_shows_message_when_not_connected(void **state);
void cmd_otr_gen_generates_key_for_connected_account(void **state);
void cmd_otr_gen_shows_message_when_disconnected(void **state);
void cmd_otr_gen_shows_message_when_undefined(void **state);
void cmd_otr_gen_shows_message_when_started(void **state);
void cmd_otr_gen_shows_message_when_connecting(void **state);
void cmd_otr_gen_shows_message_when_disconnecting(void **state);
void cmd_otr_myfp_shows_message_when_disconnected(void **state);
void cmd_otr_myfp_shows_message_when_undefined(void **state);
void cmd_otr_myfp_shows_message_when_started(void **state);
void cmd_otr_myfp_shows_message_when_connecting(void **state);
void cmd_otr_myfp_shows_message_when_disconnecting(void **state);
void cmd_otr_myfp_shows_message_when_no_key(void **state);
void cmd_otr_myfp_shows_my_fingerprint(void **state);
void cmd_otr_theirfp_shows_message_when_in_console(void **state);
void cmd_otr_theirfp_shows_message_when_in_muc(void **state);
void cmd_otr_theirfp_shows_message_when_in_private(void **state);
void cmd_otr_theirfp_shows_message_when_non_otr_chat_window(void **state);
void cmd_otr_theirfp_shows_fingerprint(void **state);
void cmd_otr_start_shows_message_when_in_console(void **state);
void cmd_otr_start_shows_message_when_in_muc(void **state);
void cmd_otr_start_shows_message_when_in_private(void **state);
void cmd_otr_start_shows_message_when_already_started(void **state);
void cmd_otr_start_shows_message_when_no_key(void **state);
void cmd_otr_start_sends_otr_query_message_to_current_recipeint(void **state);
#else
void cmd_otr_shows_message_when_otr_unsupported(void **state);
#endif
