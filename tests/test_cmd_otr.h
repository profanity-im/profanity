#include "config.h"

#ifdef HAVE_LIBOTR
void cmd_otr_shows_usage_when_no_args(void **state);
void cmd_otr_shows_usage_when_invalid_subcommand(void **state);
void cmd_otr_log_shows_usage_when_no_args(void **state);
void cmd_otr_log_shows_usage_when_invalid_subcommand(void **state);
void cmd_otr_log_on_enables_logging(void **state);
void cmd_otr_log_off_disables_logging(void **state);
void cmd_otr_redact_redacts_logging(void **state);
void cmd_otr_log_on_shows_warning_when_chlog_disabled(void **state);
void cmd_otr_log_redact_shows_warning_when_chlog_disabled(void **state);
void cmd_otr_warn_shows_usage_when_no_args(void **state);
void cmd_otr_warn_shows_usage_when_invalid_arg(void **state);
void cmd_otr_warn_on_enables_unencrypted_warning(void **state);
void cmd_otr_warn_off_disables_unencrypted_warning(void **state);
#else
void cmd_otr_shows_message_when_otr_unsupported(void **state);
#endif
