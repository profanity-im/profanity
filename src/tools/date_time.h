/*
 * date_time.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2021 Stefan <stefan@devlug.de>
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
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
 */

#ifndef TOOLS_DATE_TIME_H
#define TOOLS_DATE_TIME_H

#include <glib.h>

/*!
 * \page xep0082 XEP-0082: XMPP Date and Time Profiles
 *
 * Date and Time implementation ( see also XEP-0082: XMPP Date and Time Profiles ) for profanity.
 *
 * Profanity will use format 1969-07-21T02:56:15Z in UTC.
 */


/*!
 * Profanity date and time now in UTC.
 *
 * \returns Date and Time in format like 2021-02-13T20:43:26Z
 */

gchar* prof_date_now();

/*!
 * Current date in timezone
 *
 *\retuns Date in format 2021-02-13 in timezone
 */

gchar* prof_date_today();

gchar* prof_date_time_fmt(GDateTime *datetime);

#endif // TOOLS_DATE_TIME_H
