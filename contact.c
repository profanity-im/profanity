#include <stdlib.h>
#include <string.h>

#include "contact.h"

struct p_contact_t {
    char *name;
    char *show;
    char *status;
};

PContact p_contact_new(const char * const name, const char * const show, 
    const char * const status)
{
    PContact contact = malloc(sizeof(struct p_contact_t));
    contact->name = strdup(name);

    if (show == NULL || (strcmp(show, "") == 0))
        contact->show = strdup("online");
    else
        contact->show = strdup(show);

    if (status != NULL)
        contact->status = strdup(status);
    else
        contact->status = NULL;

    return contact;
}

void p_contact_free(PContact contact)
{
    free(contact->name);

    if (contact->show != NULL) {
        free(contact->show);
        contact->show = NULL;
    }
    if (contact->status != NULL) {
        free(contact->status);
        contact->status = NULL;
    }

    free(contact);
    contact = NULL;
}

char * p_contact_name(PContact contact)
{
    return contact->name;
}

char * p_contact_show(PContact contact)
{
    return contact->show;
}

char * p_contact_status(PContact contact)
{
    return contact->status;
}
