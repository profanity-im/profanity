/*
 * cafile.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2022 Steffen Jaeckel <jaeckel-floss@eyet-services.de>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef CONFIG_CAFILE_H
#define CONFIG_CAFILE_H

#include <glib.h>
#include "tlscerts.h"

void cafile_add(const TLSCertificate* cert);
gchar* cafile_get_name(void);

#endif
