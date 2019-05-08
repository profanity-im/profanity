#include <libotr/proto.h>
#include <libotr/message.h>
#include <glib.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "config/account.h"

#include "otr/otr.h"

OtrlUserState otr_userstate(void)
{
    return NULL;
}

OtrlMessageAppOps* otr_messageops(void)
{
    return NULL;
}

GHashTable* otr_smpinitators(void)
{
    return NULL;
}

void otr_init(void) {}
void otr_shutdown(void) {}

char* otr_libotr_version(void)
{
    return mock_ptr_type(char*);
}

char* otr_start_query(void)
{
    return mock_ptr_type(char*);
}

void otr_poll(void) {}
void otr_on_connect(ProfAccount *account) {}
char* otr_on_message_recv(const char * const barejid, const char * const resource, const char * const message, gboolean *was_decrypted)
{
    return NULL;
}
gboolean otr_on_message_send(ProfChatWin *chatwin, const char * const message, gboolean request_receipt)
{
    return FALSE;
}

void otr_keygen(ProfAccount *account)
{
    check_expected(account);
}

gboolean otr_key_loaded(void)
{
    return (gboolean)mock();
}

char* otr_tag_message(const char * const msg)
{
    return NULL;
}

gboolean otr_is_secure(const char * const recipient)
{
    return FALSE;
}

gboolean otr_is_trusted(const char * const recipient)
{
    return FALSE;
}

void otr_trust(const char * const recipient) {}
void otr_untrust(const char * const recipient) {}

void otr_smp_secret(const char * const recipient, const char *secret) {}
void otr_smp_question(const char * const recipient, const char *question, const char *answer) {}
void otr_smp_answer(const char * const recipient, const char *answer) {}

void otr_end_session(const char * const recipient) {}

char * otr_get_my_fingerprint(void)
{
    return mock_ptr_type(char *);
}

char * otr_get_their_fingerprint(const char * const recipient)
{
    check_expected(recipient);
    return mock_ptr_type(char *);
}

char * otr_encrypt_message(const char * const to, const char * const message)
{
    return NULL;
}

char * otr_decrypt_message(const char * const from, const char * const message,
    gboolean *was_decrypted)
{
    return NULL;
}

void otr_free_message(char *message) {}

prof_otrpolicy_t otr_get_policy(const char * const recipient)
{
    return PROF_OTRPOLICY_MANUAL;
}
