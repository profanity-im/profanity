#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <curl/curl.h>
#include <gio/gio.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>

#include "profanity.h"
#include "event/client_events.h"
#include "tools/http_common.h"
#include "tools/aesgcm_upload.h"
#include "omemo/omemo.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "common.h"

void* aesgcm_file_put(HTTPUploader* uploader) {
    // Open a file handle for reading the cleartext.
    FILE* fh = NULL;
    if (!(fh = fopen(uploader->filename, "rb"))) {
        http_print_transfer_update(uploader->window, uploader->url,
                                   "Uploading '%s' failed: Could not open file for reading.",
                                   uploader->url);
        return NULL;
    }

    // Create temporary file for writing the ciphertext.
    gchar* tmpname = NULL;
    gint tmpfd;
    if ((tmpfd = g_file_open_tmp("profanity.XXXXXX", &tmpname, NULL)) == -1) {
        http_print_transfer_update(uploader->window, uploader->url,
                                   "Uploading '%s' failed: Unable to create "
                                   "temporary file for encrypted transfer.",
                                   uploader->url);
        return NULL;
    }


    // Open a file handle for writing the ciphertext.
    FILE* tmpfh = fopen(tmpname, "rb");
    if (tmpfh == NULL) {
        http_print_transfer_update(uploader->window, uploader->url,
                                   "Uploading '%s' failed: Unable to open "
                                   "temporary file at '%s' for reading (%s).",
                                   uploader->url, tmpname,
                                   g_strerror(errno));
        return NULL;
    }

    // Encrypt the file and store the result in the temporary file previously
    // created.
    int crypt_res;
    char* fragment;
    fragment = omemo_encrypt_file(fh, tmpfh, file_size(uploader->filename), &crypt_res);
    if (crypt_res != 0) {
        http_print_transfer_update(uploader->window, uploader->url,
                                   "Uploading '%s' failed: Failed to encrypt "
                                   "file (%s).",
                                   https_url, gcry_strerror(crypt_res));
        goto out;
    }

    // Force flush as the upload will read from the same file.
    fflush(tmpfh);
    fclose(tmpfh);

    uploader->filename = strdup(tmpname); // Set the temporary ciphertext as the file to upload.
    http_uploader_put(uploader);

out:
    close(tmpfd); // Also closes the stream.
    remove(tmpname); // Remove the temporary ciphertext.
    g_free(tmpname);
}
