#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contact_list.h"

// contact list node
struct _contact_t {
    char *contact;    
    struct _contact_t *next;
};

// the contact list
static struct _contact_t *_contact_list = NULL;

static struct _contact_t * _make_contact(char *contact);

void contact_list_clear(void)
{
    struct _contact_t *curr = _contact_list;
    
    if (curr) {
        while(curr) {
            free(curr->contact);
            curr = curr->next;
        }

        free(_contact_list);
        _contact_list = NULL;
    }
}

int contact_list_remove(char *contact)
{
    return 0;
}

int contact_list_add(char *contact)
{
    if (!_contact_list) {
        _contact_list = _make_contact(contact);
        
        return 1;
    } else {
        struct _contact_t *curr = _contact_list;
        struct _contact_t *prev = NULL;

        while(curr) {
            if (strcmp(curr->contact, contact) == 0)
                return 0;

            prev = curr;
            curr = curr->next;
        }

        curr = _make_contact(contact);        
        
        if (prev)
            prev->next = curr;

        return 1;
    }
}

struct contact_list *get_contact_list(void)
{
    int count = 0;
    
    struct contact_list *list = 
        (struct contact_list *) malloc(sizeof(struct contact_list));

    struct _contact_t *curr = _contact_list;
    
    if (!curr) {
        list->contacts = NULL;
    } else {
        list->contacts = (char **) malloc(sizeof(char **));

        while(curr) {
            list->contacts[count] = 
                (char *) malloc((strlen(curr->contact) + 1) * sizeof(char));
            strcpy(list->contacts[count], curr->contact);
            count++;
            
            curr = curr->next;
        }
    }

    list->size = count;

    return list;
}

static struct _contact_t * _make_contact(char *contact)
{
    struct _contact_t *new = (struct _contact_t *) malloc(sizeof(struct _contact_t));
    new->contact = (char *) malloc((strlen(contact) + 1) * sizeof(char));
    strcpy(new->contact, contact);
    new->next = NULL;

    return new;
}

