#include <glib.h>

void omemo_devicelist_subscribe(void);
void omemo_devicelist_publish(GList *device_list);
void omemo_devicelist_request(const char * const jid);
void omemo_bundle_publish(void);
