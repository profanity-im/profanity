#include "log.h"
#include "windows.h"
#include "app.h"

int main(void)
{   
    log_init();
    gui_init();

    start_profanity();
    
    gui_close();
    log_close();

    return 0;
}
