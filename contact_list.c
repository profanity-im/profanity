/* 
 * contact_list.c
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
 * 
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

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
    if (!_contact_list) {
        return 0;
    } else {
        struct _contact_t *curr = _contact_list;
        struct _contact_t *prev = NULL;
        
        while(curr) {
            if (strcmp(curr->contact, contact) == 0) {
                if (prev)
                    prev->next = curr->next;
                else
                    _contact_list = curr->next;
                
                free(curr->contact);
                free(curr);

                return 1;
            }

            prev = curr;
            curr = curr->next;
        }

        return 0;
    }
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

