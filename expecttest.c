#include <string.h>
#include <assert.h>
#include <expect.h>
#include <stabber.h>

#define CONNECT_CMD "/connect stabber@localhost port 5230\r"
#define PASSWORD    "password\r"
#define QUIT_CMD    "/quit\r"

int main(void)
{
    stbbr_start(5230);

    int res = 0;
    int fd = exp_spawnl("./profanity");
    FILE *fp = fdopen(fd, "r+");
    setbuf(fp, (char *)0);

    res = exp_expectl(fd, exp_exact, "Profanity. Type /help for help information.", 10, exp_end);
    assert(res == 10);

    write(fd, CONNECT_CMD, strlen(CONNECT_CMD));
    res = exp_expectl(fd, exp_exact, "Enter password:", 11, exp_end);
    assert(res == 11);
    
    write(fd, PASSWORD, strlen(PASSWORD));
    res = exp_expectl(fd, exp_exact, "Connecting with account stabber@localhost", 12, exp_end);
    assert(res == 12);
    res = exp_expectl(fd, exp_exact, "stabber@localhost logged in successfully", 13, exp_end);
    assert(res == 13);
    
    write(fd, QUIT_CMD, strlen(QUIT_CMD));
    sleep(1);

    printf("\n");
    printf("\n");
    printf("PID: %d\n", exp_pid);

    stbbr_stop();
}
