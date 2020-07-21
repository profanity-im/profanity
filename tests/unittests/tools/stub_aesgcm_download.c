#ifndef TOOLS_AESGCM_DOWNLOAD_H
#define TOOLS_AESGCM_DOWNLOAD_H

#include <pthread.h>

typedef struct prof_win_t ProfWin;
typedef struct http_download_t HTTPDownload;

typedef struct aesgcm_download_t
{
    char* url;
    char* filename;
    ProfWin* window;
    pthread_t worker;
    HTTPDownload* http_dl;
} AESGCMDownload;

void* aesgcm_file_get(void* userdata);

void aesgcm_download_cancel_processes(ProfWin* window);
void aesgcm_download_add_download(AESGCMDownload* download);

#endif
