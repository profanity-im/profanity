#include "ui/ui.h"
#include "xmpp/vcard.h"

ProfWin*
vcard_user_create_win()
{
    return NULL;
}

void
vcard_user_add_element(vcard_element_t* element)
{
}

void
vcard_user_remove_element(unsigned int index)
{
}

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
vcard_user_set_name_middle(char* middle)
{
}

void
vcard_user_set_name_prefix(char* prefix)
{
}

void
vcard_user_set_name_suffix(char* suffix)
{
}

vcard_element_t*
vcard_user_get_element_index(unsigned int index)
{
    return NULL;
}

void
vcard_user_free(void)
{
}
