#include "log.h"
#include "windows.h"
#include "profanity.h"

int main(void)
{   
    int exit_status = LOGIN_FAIL;
    log_init();
    gui_init();

    while (exit_status == LOGIN_FAIL) {
        exit_status = profanity_start();
    }
    
    gui_close();
    log_close();

    return 0;
}
