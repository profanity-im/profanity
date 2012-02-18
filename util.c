#include <time.h>
#include <string.h>
#include <ctype.h>

void get_time(char *thetime)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(thetime, 80, "%H:%M", timeinfo);
}

char *trim(char *str)
{
    char *end;

    while (isspace(*str)) 
        str++;

    if (*str == 0)
      return str;

    end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) 
        end--;

    *(end+1) = 0;

    return str;
}

