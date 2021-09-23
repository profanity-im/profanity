#ifndef TOOLS_AESGCM_UPLOAD_H
#define TOOLS_AESGCM_UPLOAD_H

#ifdef PLATFORM_CYGWIN
#define SOCKET int
#endif

#include <sys/select.h>
#include <curl/curl.h>
#include "tools/http_upload.h"

#include "ui/win_types.h"

typedef struct aesgcm_upload_t
{
    HTTPUploader* uploader;
} AESGCMUpload;

#endif
