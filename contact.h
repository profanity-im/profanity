#ifndef CONTACT_H
#define CONTACT_H

typedef struct p_contact_t *PContact;

PContact p_contact_new(const char * const name, const char * const show, 
    const char * const status);
void p_contact_free(PContact contact);
char * p_contact_name(PContact contact);
char * p_contact_show(PContact contact);
char * p_contact_status(PContact contact);

#endif
