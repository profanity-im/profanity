#include "glib.h"

void load_preferences(void** state);
void close_preferences(void** state);

void init_chat_sessions(void** state);
void close_chat_sessions(void** state);

int utf8_pos_to_col(char* str, int utf8_pos);

void glist_set_cmp(GCompareFunc func);
int glist_contents_equal(const void* actual, const void* expected);