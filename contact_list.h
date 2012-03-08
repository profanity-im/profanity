#ifndef CONTACT_LIST_H
#define CONTACT_LIST_H

struct contact_list {
    char **contacts;
    int size;
};

void contact_list_clear(void);
int contact_list_add(char *contact);
struct contact_list *get_contact_list(void);

#endif
