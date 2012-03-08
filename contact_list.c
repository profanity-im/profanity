#include <stdlib.h>
#include <string.h>

#include "contact_list.h"

// internal contact list
struct _contact_t {
    char *contact;    
    struct _contact_t *next;
    int last;
};

// the contact list
static struct _contact_t *_contact_list = NULL;

void contact_list_clear(void)
{
    if (_contact_list == NULL) {
        return;
    }
    else {
        struct _contact_t *curr = _contact_list;
        while(!curr->last) {
            free(curr->contact);
            curr = curr->next;
        }
        free(_contact_list);
    }
    
    _contact_list = NULL;
}

int contact_list_add(char *contact)
{
    if (_contact_list == NULL) {
        // create the new one
        _contact_list = (struct _contact_t *) malloc(sizeof(struct _contact_t));
        _contact_list->contact = 
            (char *) malloc((strlen(contact)+1) * sizeof(char));
        strcpy(_contact_list->contact, contact);
        _contact_list->last = 1;

        // complete the circle
        _contact_list->next = _contact_list;

    } else {
        // go through entries
        struct _contact_t *curr = _contact_list;
        while (!curr->last) {
            // if already there, return without adding
            if (strcmp(curr->contact, contact) == 0)
                return 1;

           // move on  
           curr = curr->next;
        }

        // create the new one
        struct _contact_t *new = 
            (struct _contact_t *) malloc(sizeof(struct _contact_t));
        new->contact = (char *) malloc((strlen(contact)+1) * sizeof(char));
        strcpy(new->contact, contact);
        new->last = 1;

        // point the old one to it
        curr->next = new;
        curr->last = 0;
    }

    return 0;
}

struct contact_list *get_contact_list(void)
{
    struct contact_list *list = 
        (struct contact_list *) malloc(sizeof(struct contact_list));

    if (_contact_list == NULL) {
        list->contacts = NULL;
        list->size = 0;
        return list;
    } else {
        struct _contact_t *curr = _contact_list;
        list->contacts = (char **) malloc(sizeof(char **));
        int count = 0;
        while(1) {
            list->contacts[count] = 
                (char *) malloc((strlen(curr->contact)+1) * sizeof(char));
            strcpy(list->contacts[count], curr->contact);
            count++;
            
            if (curr->last)
                break;
            else {
                curr = curr->next;
            }
        }
    
        list->size = count;

        return list;
    }
}
