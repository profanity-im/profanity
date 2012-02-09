#ifndef JABBER_H
#define JABBER_H

void jabber_connect(char *user, char *passwd);
void jabber_disconnect(void);
void jabber_roster_request(void);
void jabber_process_events(void);
void jabber_send(char *msg, char *recipient);

#endif
