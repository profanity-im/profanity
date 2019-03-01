#include <glib.h>

#include "ui/ui.h"
#include "config/account.h"

#define OMEMO_ERR_UNSUPPORTED_CRYPTO -10000

typedef struct omemo_context_t omemo_context;

typedef struct omemo_key {
    const unsigned char *data;
    size_t length;
    gboolean prekey;
    uint32_t device_id;
} omemo_key_t;

void omemo_init(void);
void omemo_on_connect(ProfAccount *account);
void omemo_generate_crypto_materials(ProfAccount *account);

uint32_t omemo_device_id(void);
void omemo_identity_key(unsigned char **output, size_t *length);
void omemo_signed_prekey(unsigned char **output, size_t *length);
void omemo_signed_prekey_signature(unsigned char **output, size_t *length);
void omemo_prekeys(GList **prekeys, GList **ids, GList **lengths);
void omemo_set_device_list(const char *const jid, GList * device_list);

void omemo_start_session(const char *const barejid);
void omemo_start_device_session(const char *const jid, uint32_t device_id, uint32_t prekey_id, const unsigned char *const prekey, size_t prekey_len, uint32_t signed_prekey_id, const unsigned char *const signed_prekey, size_t signed_prekey_len, const unsigned char *const signature, size_t signature_len, const unsigned char *const identity_key, size_t identity_key_len);

gboolean omemo_loaded(void);
gboolean omemo_on_message_send(ProfChatWin *chatwin, const char *const message, gboolean request_receipt);
char * omemo_on_message_recv(const char *const from, uint32_t sid, const unsigned char *const iv, size_t iv_len, GList *keys, const unsigned char *const payload, size_t payload_len);
