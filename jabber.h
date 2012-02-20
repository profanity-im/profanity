#ifndef JABBER_H
#define JABBER_H

#define STARTED 0
#define CONNECTING 1
#define CONNECTED 2
#define DISCONNECTED 3

int jabber_connection_status(void);
int jabber_connect(char *user, char *passwd);
void jabber_disconnect(void);
void jabber_roster_request(void);
void jabber_process_events(void);
void jabber_send(char *msg, char *recipient);

#endif
