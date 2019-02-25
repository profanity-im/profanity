#include <glib.h>

#include "config/account.h"

typedef struct omemo_context_t omemo_context;

void omemo_init(void);
void omemo_generate_crypto_materials(ProfAccount *account);

uint32_t omemo_device_id(void);
void omemo_identity_key(unsigned char **output, size_t *length);
void omemo_signed_prekey(unsigned char **output, size_t *length);
void omemo_signed_prekey_signature(unsigned char **output, size_t *length);
void omemo_prekeys(GList **prekeys, GList **ids, GList **lengths);
void omemo_set_device_list(const char *const jid, GList * device_list);

void omemo_start_session(const char *const barejid);
void omemo_start_device_session(const char *const jid, uint32_t device_id, const unsigned char *const prekey, size_t prekey_len);

gboolean omemo_loaded(void);
