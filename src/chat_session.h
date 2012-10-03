/* 
 * chat_session.h
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

#ifndef CHAT_SESSION_H
#define CHAT_SESSION_H

typedef struct chat_session_t *ChatSession;

typedef enum {
    ACTIVE,
    COMPOSING,
    PAUSED
} chat_state_t;

void chat_session_init(void);
int chat_session_size(void);
ChatSession chat_session_new(char *recipient);
ChatSession chat_session_get(char *recipient);
chat_state_t chat_session_get_state(ChatSession session);
char * chat_session_get_recipient(ChatSession session);

#endif
