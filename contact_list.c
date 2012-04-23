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

// internal contact list
static struct contact_node_t * _contact_list = NULL;

// state of current tab completion, currrent node 
static struct contact_node_t * _last_found = NULL;
// and current search pattern
static char * _search_str = NULL;

static char * _search_contact_list_from(struct contact_node_t * curr);
static struct contact_node_t * _make_contact_node(const char * const name, 
    const char * const show, const char * const status);
static struct contact_t * _new_contact(const char * const name, 
    const char * const show, const char * const status);
static void _destroy_contact(struct contact_t *contact);
static struct contact_node_t * _copy_contact_list(struct contact_node_t *node);
static void _insert_contact(struct contact_node_t *curr, 
    struct contact_node_t *prev, const char * const name, 
    const char * const show, const char * const status);

void contact_list_clear(void)
{
    struct contact_node_t *curr = _contact_list;
    
    if (curr) {
        while(curr) {
            struct contact_t *contact = curr->contact;
            _destroy_contact(contact);
            curr = curr->next;
        }

        free(_contact_list);
        _contact_list = NULL;
        
        reset_search_attempts();
    }
}

void destroy_list(struct contact_node_t *list)
{
    while(list) {
        struct contact_t *contact = list->contact;
        _destroy_contact(contact);
        list = list->next;
    }

    free(list);
    list = NULL;
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
    if (!_contact_list) {
        return 0;
    } else {
        struct contact_node_t *curr = _contact_list;
        struct contact_node_t *prev = NULL;
        
        while(curr) {
            struct contact_t *contact = curr->contact;
            if (strcmp(contact->name, name) == 0) {
                if (prev)
                    prev->next = curr->next;
                else
                    _contact_list = curr->next;

                // reset last found if it points at the node to be removed
                if (_last_found != NULL)
                    if (strcmp(_last_found->contact->name, contact->name) == 0)
                        _last_found = NULL;

                _destroy_contact(contact);
                free(curr);

                return 1;
            }

            prev = curr;
            curr = curr->next;
        }

        return 0;
    }
}

int contact_list_add(const char * const name, const char * const show, 
    const char * const status)
{

    if (!_contact_list) {
        _contact_list = _make_contact_node(name, show, status);
        
        return 1;
    } else {
        struct contact_node_t *curr = _contact_list;
        struct contact_node_t *prev = NULL;

        while(curr) {
            struct contact_t *curr_contact = curr->contact;

            // insert    
            if (strcmp(curr_contact->name, name) > 0) {
                _insert_contact(curr, prev, name, show, status);
                return 0;
            // update
            } else if (strcmp(curr_contact->name, name) == 0) {
                _destroy_contact(curr->contact);
                curr->contact = _new_contact(name, show, status);
                return 0;
            }

            // move on
            prev = curr;
            curr = curr->next;
        }

        curr = _make_contact_node(name, show, status);    
        
        if (prev)
            prev->next = curr;

        return 1;
    }
}

struct contact_node_t * get_contact_list(void)
{
    struct contact_node_t *copy = NULL;
    struct contact_node_t *curr = _contact_list;
    
    copy = _copy_contact_list(curr);

    return copy;
}

struct contact_node_t * _copy_contact_list(struct contact_node_t *node)
{
    if (node == NULL) {
        return NULL;
    } else {
        struct contact_t *curr_contact = node->contact;
        struct contact_node_t *copy = 
            _make_contact_node(curr_contact->name, 
                               curr_contact->show, 
                               curr_contact->status);

        copy->next = _copy_contact_list(node->next);

        return copy;
    }
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
        found = _search_contact_list_from(_last_found->next);
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

int get_size(struct contact_node_t *list)
{
    int size = 0;

    while(list) {
        size++;
        list = list->next;
    }

    return size;
}

static char * _search_contact_list_from(struct contact_node_t * curr)
{
    while(curr) {
        struct contact_t *curr_contact = curr->contact;
        
        // match found
        if (strncmp(curr_contact->name, _search_str, strlen(_search_str)) == 0) {
            char *result = 
                (char *) malloc((strlen(curr_contact->name) + 1) * sizeof(char));

            // set pointer to last found
            _last_found = curr;
            
            // return the contact, must be free'd by caller
            strcpy(result, curr_contact->name);
            return result;
        }

        curr = curr->next;
    }

    return NULL;
}

static struct contact_node_t * _make_contact_node(const char * const name, 
    const char * const show, const char * const status)
{
    struct contact_node_t *new = 
        (struct contact_node_t *) malloc(sizeof(struct contact_node_t));
    new->contact = _new_contact(name, show, status);
    new->next = NULL;

    return new;
}

static struct contact_t * _new_contact(const char * const name, 
    const char * const show, const char * const status)
{
    struct contact_t *new = 
        (struct contact_t *) malloc(sizeof(struct contact_t));
    new->name = (char *) malloc((strlen(name) + 1) * sizeof(char));
    strcpy(new->name, name);
    
    if (show == NULL || (strcmp(show, "") == 0)) {
        new->show = (char *) malloc((strlen("online") + 1) * sizeof(char));
        strcpy(new->show, "online");
    } else {
        new->show = (char *) malloc((strlen(show) + 1) * sizeof(char));
        strcpy(new->show, show);
    }

    if (status != NULL) {
        new->status = (char *) malloc((strlen(status) + 1) * sizeof(char));
        strcpy(new->status, status);
    } else {
        new->status = NULL;
    }

    return new;
}

static void _destroy_contact(struct contact_t *contact)
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

static void _insert_contact(struct contact_node_t *curr, 
    struct contact_node_t *prev, const char * const name,
    const char * const show, const char * const status)
{
    if (prev) {
        struct contact_node_t *new = _make_contact_node(name, show, status);
        new->next = curr;
        prev->next = new;
    } else {
        struct contact_node_t *new = _make_contact_node(name, show, status);
        new->next = _contact_list;
        _contact_list = new;
    }
}

