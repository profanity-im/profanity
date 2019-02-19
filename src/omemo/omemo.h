#include "config/account.h"

typedef struct omemo_context_t omemo_context;

void omemo_init(void);
void omemo_generate_crypto_materials(ProfAccount *account);
