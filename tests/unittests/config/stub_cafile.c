/*
 * stub_cafile.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2022 Steffen Jaeckel <jaeckel-floss@eyet-services.de>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include <fcntl.h>
#include <glib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#include "common.h"
#include "config/files.h"
#include "xmpp/xmpp.h"
#include "log.h"

void
cafile_add(const TLSCertificate* cert)
{
}

gchar*
cafile_get_name(void)
{
    return NULL;
}
