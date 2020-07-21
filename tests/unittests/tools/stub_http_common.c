#ifndef TOOLS_HTTP_COMMON_H
#define TOOLS_HTTP_COMMON_H

typedef struct prof_win_t ProfWin;

char*
http_basename_from_url(const char* url)
{
    return "";
}

void http_print_transfer(ProfWin* window, char* url, const char* fmt, ...);
void http_print_transfer_update(ProfWin* window, char* url,
                                const char* fmt, ...);

#endif
