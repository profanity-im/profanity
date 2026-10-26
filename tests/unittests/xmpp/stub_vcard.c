#include "ui/ui.h"
#include "xmpp/vcard.h"

void
vcard_print(xmpp_ctx_t* ctx, ProfWin* window, char* jid)
{
}

void
vcard_photo(xmpp_ctx_t* ctx, char* jid, char* filename, int index, gboolean open)
{
}

void
vcard_user_refresh(void)
{
}

void
vcard_user_save(void)
{
}

void
vcard_user_set_fullname(char* fullname)
{
}

void
vcard_user_set_name_family(char* family)
{
}

void
vcard_user_set_name_given(char* given)
{
}

void
vcard_user_free(void)
{
}

DataForm*
vcard_create_form(vCard* vcard)
{
    return NULL;
}

DataForm*
vcard_user_create_form(void)
{
    return NULL;
}

void
vcard_user_apply_form(DataForm* form)
{
}
