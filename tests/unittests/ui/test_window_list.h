#ifndef TESTS_TEST_WINDOW_LIST_H
#define TESTS_TEST_WINDOW_LIST_H

void wins_get_url__returns__null_when_buffer_empty(void** state);
void wins_get_url__returns__urls_from_newest_to_oldest(void** state);
void wins_get_url__returns__deduplicated_urls_favoring_newest(void** state);
void wins_get_url__returns__match_by_prefix(void** state);
void wins_get_url__returns__urls_in_console_window(void** state);
void wins_reset_url_ac__updates__clears_cached_urls(void** state);

#endif
