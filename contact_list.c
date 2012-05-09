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

#include <glib.h>

#include "contact.h"
#include "contact_list.h"

// internal contact list
static GSList * _contact_list = NULL;

// state of current tab completion, currrent node 
static GSList * _last_found = NULL;
// and current search pattern
static char * _search_str = NULL;

static char * _search_contact_list_from(GSList * curr);

void contact_list_clear(void)
{
    g_slist_free_full(_contact_list, (GDestroyNotify)p_contact_free);
    _contact_list = NULL;
        
    reset_search_attempts();
}

void reset_search_attempts(void)
{
    _last_found = NULL;
    if (_search_str != NULL) {
        free(_search_str);
        _search_str = NULL;
    }
}

int contact_list_remove(const char * const name)
{
    // reset last found if it points at the node to be removed
    if (_last_found != NULL)
        if (strcmp(p_contact_name(_last_found->data), name) == 0)
            _last_found = NULL;
    
    if (!_contact_list) {
        return 0;
    } else {
        GSList *curr = _contact_list;
        
        while(curr) {
            PContact contact = curr->data;
            if (strcmp(p_contact_name(contact), name) == 0) {
                _contact_list = g_slist_remove(_contact_list, contact);
                p_contact_free(contact);

                return 1;
            }

            curr = g_slist_next(curr);
        }

        return 0;
    }
}

int contact_list_add(const char * const name, const char * const show, 
    const char * const status)
{

    // empty list, create new
    if (!_contact_list) {
        _contact_list = g_slist_append(_contact_list, 
            p_contact_new(name, show, status));
        
        return 1;
    } else {
        GSList *curr = _contact_list;

        while(curr) {
            PContact curr_contact = curr->data;

            // insert    
            if (strcmp(p_contact_name(curr_contact), name) > 0) {
                _contact_list = g_slist_insert_before(_contact_list,
                    curr, p_contact_new(name, show, status));
                return 0;
            // update
            } else if (strcmp(p_contact_name(curr_contact), name) == 0) {
                p_contact_free(curr->data);
                curr->data = p_contact_new(name, show, status);
                return 0;
            }

            curr = g_slist_next(curr);
        }

        // hit end, append
        _contact_list = g_slist_append(_contact_list, 
            p_contact_new(name, show, status));
        
        return 1;
    }
}

GSList * get_contact_list(void)
{
    GSList *copy = NULL;
    GSList *curr = _contact_list;

    while(curr) {
        PContact curr_contact = curr->data;
        
        copy = g_slist_append(copy, 
            p_contact_new(p_contact_name(curr_contact),
                          p_contact_show(curr_contact),
                          p_contact_status(curr_contact)));

        curr = g_slist_next(curr);
    }

    return copy;
}

char * find_contact(char *search_str)
{
    char *found = NULL;

    // no contacts to search
    if (!_contact_list)
        return NULL;

    // first search attempt
    if (_last_found == NULL) {
        _search_str = (char *) malloc((strlen(search_str) + 1) * sizeof(char));
        strcpy(_search_str, search_str);

        found = _search_contact_list_from(_contact_list);
        return found;

    // subsequent search attempt
    } else {
        // search from here+1 to end 
        found = _search_contact_list_from(g_slist_next(_last_found));
        if (found != NULL)
            return found;

        // search from beginning
        found = _search_contact_list_from(_contact_list);
        if (found != NULL)
            return found;

        // we found nothing, reset search
        reset_search_attempts();
        return NULL;
    }
}

static char * _search_contact_list_from(GSList * curr)
{
    while(curr) {
        PContact curr_contact = curr->data;
        
        // match found
        if (strncmp(p_contact_name(curr_contact), 
                    _search_str, 
                    strlen(_search_str)) == 0) {
            char *result = 
                (char *) malloc((strlen(p_contact_name(curr_contact)) + 1) 
                    * sizeof(char));

            // set pointer to last found
            _last_found = curr;
            
            // return the contact, must be free'd by caller
            strcpy(result, p_contact_name(curr_contact));
            return result;
        }

        curr = g_slist_next(curr);
    }

    return NULL;
}
