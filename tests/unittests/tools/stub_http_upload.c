#ifndef TOOLS_HTTP_UPLOAD_H
#define TOOLS_HTTP_UPLOAD_H

#include <curl/curl.h>
#include <pthread.h>

// forward -> ui/win_types.h
typedef struct prof_win_t ProfWin;

typedef struct http_upload_t {
    char *filename;
    off_t filesize;
    curl_off_t bytes_sent;
    char *mime_type;
    char *get_url;
    char *put_url;
    ProfWin *window;
    pthread_t worker;
    int cancel;
} HTTPUpload;

//GSList *upload_processes;

void* http_file_put(void *userdata) { return NULL; }

char* file_mime_type(const char* const file_name) { return NULL; }
off_t file_size(const char* const file_name) { return 0; }

#endif
