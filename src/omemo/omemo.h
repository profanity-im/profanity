#include <glib.h>

#include "config/account.h"

typedef struct omemo_context_t omemo_context;

void omemo_init(void);
void omemo_generate_crypto_materials(ProfAccount *account);

uint32_t omemo_device_id(void);
void omemo_identity_key(unsigned char **output, size_t *length);
void omemo_signed_prekey(unsigned char **output, size_t *length);
void omemo_signed_prekey_signature(unsigned char **output, size_t *length);
void omemo_prekeys(GList ** const prekeys, GList ** const ids, GList ** const lengths);
void omemo_set_device_list(const char *const jid, GList * const device_list);

void omemo_start_session(ProfAccount *account, char *barejid);
gboolean omemo_loaded(void);
