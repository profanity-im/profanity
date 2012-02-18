#include "log.h"
#include "windows.h"
#include "profanity.h"

int main(void)
{   
    int exit_status;
    
    log_init();
    gui_init();

    do { 
        exit_status = profanity_start();
    } while (exit_status == LOGIN_FAIL);
    
    gui_close();
    log_close();

    return 0;
}
