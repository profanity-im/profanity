#ifndef _COLOR_H_
#define _COLOR_H_

/* to access color names */
#define COLOR_NAME_SIZE 256
extern const char *color_names[];

/* to add or clear cache */
int color_pair_cache_get(const char *pair_name);
void color_pair_cache_reset(void);

#endif
