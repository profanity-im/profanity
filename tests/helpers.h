#include "glib.h"

void init_preferences(void **state);
void close_preferences(void **state);

void glist_set_cmp(GCompareFunc func);
int glist_contents_equal(const void *actual, const void *expected);
