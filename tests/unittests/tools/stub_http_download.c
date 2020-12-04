#ifndef TOOLS_HTTP_DOWNLOAD_H
#define TOOLS_HTTP_DOWNLOAD_H

#include <curl/curl.h>
#include <pthread.h>

typedef struct prof_win_t ProfWin;

typedef struct http_download_t
{
    char* url;
    char* filename;
    char* directory;
    FILE* filehandle;
    curl_off_t bytes_received;
    ProfWin* window;
    pthread_t worker;
    int cancel;
} HTTPDownload;

void*
http_file_get(void* userdata)
{
    return NULL;
}

void http_download_cancel_processes(){};
void http_download_add_download(){};

#endif
