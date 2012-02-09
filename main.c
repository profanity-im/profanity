#include "log.h"
#include "windows.h"
#include "profanity.h"

int main(void)
{   
    log_init();
    gui_init();

    profanity_start();
    
    gui_close();
    log_close();

    return 0;
}
