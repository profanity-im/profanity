#include <time.h>

void get_time(char *thetime)
{
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(thetime, 80, "%H:%M", timeinfo);
}

