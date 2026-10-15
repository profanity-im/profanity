/*
 * color.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 Aurelien Aptel <aurelien.aptel@gmail.com>
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <glib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#elif HAVE_CURSES_H
#include <curses.h>
#endif

#include "config/color.h"
#include "config/theme.h"
#include "log.h"

static struct color_pair_cache
{
    struct
    {
        int16_t fg, bg;
    }* pairs;
    int size;
    int capacity;
} cache = { 0 };

/*
 * xterm default 256 colors
 * XXX: there are many duplicates... (eg blue3)
 */

const struct color_def color_names[COLOR_NAME_SIZE] = {
    [0] = { 0, 0, 0, "black" },
    [1] = { 0, 100, 40, "red" },
    [2] = { 120, 100, 40, "green" },
    [3] = { 60, 100, 40, "yellow" },
    [4] = { 240, 100, 40, "blue" },
    [5] = { 300, 100, 40, "magenta" },
    [6] = { 180, 100, 40, "cyan" },
    [7] = { 0, 0, 90, "white" },
    [8] = { 0, 0, 50, "lightblack" },
    [9] = { 0, 100, 50, "lightred" },
    [10] = { 120, 100, 50, "lightgreen" },
    [11] = { 60, 100, 50, "lightyellow" },
    [12] = { 240, 100, 68, "lightblue" },
    [13] = { 300, 100, 50, "lightmagenta" },
    [14] = { 180, 100, 50, "lightcyan" },
    [15] = { 0, 0, 100, "lightwhite" },
    [16] = { 0, 0, 0, "grey0" },
    [17] = { 240, 100, 19, "navyblue" },
    [18] = { 240, 100, 26, "darkblue" },
    [19] = { 240, 100, 34, "blue3" },
    [20] = { 240, 100, 42, "blue3" },
    [21] = { 240, 100, 50, "blue1" },
    [22] = { 120, 100, 19, "darkgreen" },
    [23] = { 180, 100, 19, "deepskyblue4" },
    [24] = { 198, 100, 26, "deepskyblue4" },
    [25] = { 207, 100, 34, "deepskyblue4" },
    [26] = { 213, 100, 42, "dodgerblue3" },
    [27] = { 218, 100, 50, "dodgerblue2" },
    [28] = { 120, 100, 26, "green4" },
    [29] = { 162, 100, 26, "springgreen4" },
    [30] = { 180, 100, 26, "turquoise4" },
    [31] = { 194, 100, 34, "deepskyblue3" },
    [32] = { 202, 100, 42, "deepskyblue3" },
    [33] = { 208, 100, 50, "dodgerblue1" },
    [34] = { 120, 100, 34, "green3" },
    [35] = { 153, 100, 34, "springgreen3" },
    [36] = { 166, 100, 34, "darkcyan" },
    [37] = { 180, 100, 34, "lightseagreen" },
    [38] = { 191, 100, 42, "deepskyblue2" },
    [39] = { 199, 100, 50, "deepskyblue1" },
    [40] = { 120, 100, 42, "green3" },
    [41] = { 147, 100, 42, "springgreen3" },
    [42] = { 158, 100, 42, "springgreen2" },
    [43] = { 169, 100, 42, "cyan3" },
    [44] = { 180, 100, 42, "darkturquoise" },
    [45] = { 189, 100, 50, "turquoise2" },
    [46] = { 120, 100, 50, "green1" },
    [47] = { 142, 100, 50, "springgreen2" },
    [48] = { 152, 100, 50, "springgreen1" },
    [49] = { 161, 100, 50, "mediumspringgreen" },
    [50] = { 171, 100, 50, "cyan2" },
    [51] = { 180, 100, 50, "cyan1" },
    [52] = { 0, 100, 19, "darkred" },
    [53] = { 300, 100, 19, "deeppink4" },
    [54] = { 282, 100, 26, "purple4" },
    [55] = { 273, 100, 34, "purple4" },
    [56] = { 267, 100, 42, "purple3" },
    [57] = { 262, 100, 50, "blueviolet" },
    [58] = { 60, 100, 19, "orange4" },
    [59] = { 0, 0, 37, "grey37" },
    [60] = { 240, 17, 45, "mediumpurple4" },
    [61] = { 240, 33, 53, "slateblue3" },
    [62] = { 240, 60, 61, "slateblue3" },
    [63] = { 240, 100, 69, "royalblue1" },
    [64] = { 78, 100, 26, "chartreuse4" },
    [65] = { 120, 17, 45, "darkseagreen4" },
    [66] = { 180, 17, 45, "paleturquoise4" },
    [67] = { 210, 33, 53, "steelblue" },
    [68] = { 220, 60, 61, "steelblue3" },
    [69] = { 225, 100, 69, "cornflowerblue" },
    [70] = { 87, 100, 34, "chartreuse3" },
    [71] = { 120, 33, 53, "darkseagreen4" },
    [72] = { 150, 33, 53, "cadetblue" },
    [73] = { 180, 33, 53, "cadetblue" },
    [74] = { 200, 60, 61, "skyblue3" },
    [75] = { 210, 100, 69, "steelblue1" },
    [76] = { 93, 100, 42, "chartreuse3" },
    [77] = { 120, 60, 61, "palegreen3" },
    [78] = { 140, 60, 61, "seagreen3" },
    [79] = { 160, 60, 61, "aquamarine3" },
    [80] = { 180, 60, 61, "mediumturquoise" },
    [81] = { 195, 100, 69, "steelblue1" },
    [82] = { 98, 100, 50, "chartreuse2" },
    [83] = { 120, 100, 69, "seagreen2" },
    [84] = { 135, 100, 69, "seagreen1" },
    [85] = { 150, 100, 69, "seagreen1" },
    [86] = { 165, 100, 69, "aquamarine1" },
    [87] = { 180, 100, 69, "darkslategray2" },
    [88] = { 0, 100, 26, "darkred" },
    [89] = { 318, 100, 26, "deeppink4" },
    [90] = { 300, 100, 26, "darkmagenta" },
    [91] = { 286, 100, 34, "darkmagenta" },
    [92] = { 278, 100, 42, "darkviolet" },
    [93] = { 272, 100, 50, "purple" },
    [94] = { 42, 100, 26, "orange4" },
    [95] = { 0, 17, 45, "lightpink4" },
    [96] = { 300, 17, 45, "plum4" },
    [97] = { 270, 33, 53, "mediumpurple3" },
    [98] = { 260, 60, 61, "mediumpurple3" },
    [99] = { 255, 100, 69, "slateblue1" },
    [100] = { 60, 100, 26, "yellow4" },
    [101] = { 60, 17, 45, "wheat4" },
    [102] = { 0, 0, 53, "grey53" },
    [103] = { 240, 20, 61, "lightslategrey" },
    [104] = { 240, 50, 69, "mediumpurple" },
    [105] = { 240, 100, 76, "lightslateblue" },
    [106] = { 74, 100, 34, "yellow4" },
    [107] = { 90, 33, 53, "darkolivegreen3" },
    [108] = { 120, 20, 61, "darkseagreen" },
    [109] = { 180, 20, 61, "lightskyblue3" },
    [110] = { 210, 50, 69, "lightskyblue3" },
    [111] = { 220, 100, 76, "skyblue2" },
    [112] = { 82, 100, 42, "chartreuse2" },
    [113] = { 100, 60, 61, "darkolivegreen3" },
    [114] = { 120, 50, 69, "palegreen3" },
    [115] = { 150, 50, 69, "darkseagreen3" },
    [116] = { 180, 50, 69, "darkslategray3" },
    [117] = { 200, 100, 76, "skyblue1" },
    [118] = { 88, 100, 50, "chartreuse1" },
    [119] = { 105, 100, 69, "lightgreen" },
    [120] = { 120, 100, 76, "lightgreen" },
    [121] = { 140, 100, 76, "palegreen1" },
    [122] = { 160, 100, 76, "aquamarine1" },
    [123] = { 180, 100, 76, "darkslategray1" },
    [124] = { 0, 100, 34, "red3" },
    [125] = { 327, 100, 34, "deeppink4" },
    [126] = { 314, 100, 34, "mediumvioletred" },
    [127] = { 300, 100, 34, "magenta3" },
    [128] = { 289, 100, 42, "darkviolet" },
    [129] = { 281, 100, 50, "purple" },
    [130] = { 33, 100, 34, "darkorange3" },
    [131] = { 0, 33, 53, "indianred" },
    [132] = { 330, 33, 53, "hotpink3" },
    [133] = { 300, 33, 53, "mediumorchid3" },
    [134] = { 280, 60, 61, "mediumorchid" },
    [135] = { 270, 100, 69, "mediumpurple2" },
    [136] = { 46, 100, 34, "darkgoldenrod" },
    [137] = { 30, 33, 53, "lightsalmon3" },
    [138] = { 0, 20, 61, "rosybrown" },
    [139] = { 300, 20, 61, "grey63" },
    [140] = { 270, 50, 69, "mediumpurple2" },
    [141] = { 260, 100, 76, "mediumpurple1" },
    [142] = { 60, 100, 34, "gold3" },
    [143] = { 60, 33, 53, "darkkhaki" },
    [144] = { 60, 20, 61, "navajowhite3" },
    [145] = { 0, 0, 69, "grey69" },
    [146] = { 240, 33, 76, "lightsteelblue3" },
    [147] = { 240, 100, 84, "lightsteelblue" },
    [148] = { 71, 100, 42, "yellow3" },
    [149] = { 80, 60, 61, "darkolivegreen3" },
    [150] = { 90, 50, 69, "darkseagreen3" },
    [151] = { 120, 33, 76, "darkseagreen2" },
    [152] = { 180, 33, 76, "lightcyan3" },
    [153] = { 210, 100, 84, "lightskyblue1" },
    [154] = { 79, 100, 50, "greenyellow" },
    [155] = { 90, 100, 69, "darkolivegreen2" },
    [156] = { 100, 100, 76, "palegreen1" },
    [157] = { 120, 100, 84, "darkseagreen2" },
    [158] = { 150, 100, 84, "darkseagreen1" },
    [159] = { 180, 100, 84, "paleturquoise1" },
    [160] = { 0, 100, 42, "red3" },
    [161] = { 333, 100, 42, "deeppink3" },
    [162] = { 322, 100, 42, "deeppink3" },
    [163] = { 311, 100, 42, "magenta3" },
    [164] = { 300, 100, 42, "magenta3" },
    [165] = { 291, 100, 50, "magenta2" },
    [166] = { 27, 100, 42, "darkorange3" },
    [167] = { 0, 60, 61, "indianred" },
    [168] = { 340, 60, 61, "hotpink3" },
    [169] = { 320, 60, 61, "hotpink2" },
    [170] = { 300, 60, 61, "orchid" },
    [171] = { 285, 100, 69, "mediumorchid1" },
    [172] = { 38, 100, 42, "orange3" },
    [173] = { 20, 60, 61, "lightsalmon3" },
    [174] = { 0, 50, 69, "lightpink3" },
    [175] = { 330, 50, 69, "pink3" },
    [176] = { 300, 50, 69, "plum3" },
    [177] = { 280, 100, 76, "violet" },
    [178] = { 49, 100, 42, "gold3" },
    [179] = { 40, 60, 61, "lightgoldenrod3" },
    [180] = { 30, 50, 69, "tan" },
    [181] = { 0, 33, 76, "mistyrose3" },
    [182] = { 300, 33, 76, "thistle3" },
    [183] = { 270, 100, 84, "plum2" },
    [184] = { 60, 100, 42, "yellow3" },
    [185] = { 60, 60, 61, "khaki3" },
    [186] = { 60, 50, 69, "lightgoldenrod2" },
    [187] = { 60, 33, 76, "lightyellow3" },
    [188] = { 0, 0, 84, "grey84" },
    [189] = { 240, 100, 92, "lightsteelblue1" },
    [190] = { 69, 100, 50, "yellow2" },
    [191] = { 75, 100, 69, "darkolivegreen1" },
    [192] = { 80, 100, 76, "darkolivegreen1" },
    [193] = { 90, 100, 84, "darkseagreen1" },
    [194] = { 120, 100, 92, "honeydew2" },
    [195] = { 180, 100, 92, "lightcyan1" },
    [196] = { 0, 100, 50, "red1" },
    [197] = { 338, 100, 50, "deeppink2" },
    [198] = { 328, 100, 50, "deeppink1" },
    [199] = { 319, 100, 50, "deeppink1" },
    [200] = { 309, 100, 50, "magenta2" },
    [201] = { 300, 100, 50, "magenta1" },
    [202] = { 22, 100, 50, "orangered1" },
    [203] = { 0, 100, 69, "indianred1" },
    [204] = { 345, 100, 69, "indianred1" },
    [205] = { 330, 100, 69, "hotpink" },
    [206] = { 315, 100, 69, "hotpink" },
    [207] = { 300, 100, 69, "mediumorchid1" },
    [208] = { 32, 100, 50, "darkorange" },
    [209] = { 15, 100, 69, "salmon1" },
    [210] = { 0, 100, 76, "lightcoral" },
    [211] = { 340, 100, 76, "palevioletred1" },
    [212] = { 320, 100, 76, "orchid2" },
    [213] = { 300, 100, 76, "orchid1" },
    [214] = { 41, 100, 50, "orange1" },
    [215] = { 30, 100, 69, "sandybrown" },
    [216] = { 20, 100, 76, "lightsalmon1" },
    [217] = { 0, 100, 84, "lightpink1" },
    [218] = { 330, 100, 84, "pink1" },
    [219] = { 300, 100, 84, "plum1" },
    [220] = { 51, 100, 50, "gold1" },
    [221] = { 45, 100, 69, "lightgoldenrod2" },
    [222] = { 40, 100, 76, "lightgoldenrod2" },
    [223] = { 30, 100, 84, "navajowhite1" },
    [224] = { 0, 100, 92, "mistyrose1" },
    [225] = { 300, 100, 92, "thistle1" },
    [226] = { 60, 100, 50, "yellow1" },
    [227] = { 60, 100, 69, "lightgoldenrod1" },
    [228] = { 60, 100, 76, "khaki1" },
    [229] = { 60, 100, 84, "wheat1" },
    [230] = { 60, 100, 92, "cornsilk1" },
    [231] = { 0, 0, 100, "grey100" },
    [232] = { 0, 0, 3, "grey3" },
    [233] = { 0, 0, 7, "grey7" },
    [234] = { 0, 0, 11, "grey11" },
    [235] = { 0, 0, 15, "grey15" },
    [236] = { 0, 0, 19, "grey19" },
    [237] = { 0, 0, 23, "grey23" },
    [238] = { 0, 0, 27, "grey27" },
    [239] = { 0, 0, 31, "grey30" },
    [240] = { 0, 0, 35, "grey35" },
    [241] = { 0, 0, 38, "grey39" },
    [242] = { 0, 0, 42, "grey42" },
    [243] = { 0, 0, 46, "grey46" },
    [244] = { 0, 0, 50, "grey50" },
    [245] = { 0, 0, 54, "grey54" },
    [246] = { 0, 0, 58, "grey58" },
    [247] = { 0, 0, 62, "grey62" },
    [248] = { 0, 0, 66, "grey66" },
    [249] = { 0, 0, 70, "grey70" },
    [250] = { 0, 0, 74, "grey74" },
    [251] = { 0, 0, 78, "grey78" },
    [252] = { 0, 0, 82, "grey82" },
    [253] = { 0, 0, 85, "grey85" },
    [254] = { 0, 0, 89, "grey89" },
    [255] = { 0, 0, 93, "grey93" },
};

/* -1 is valid curses color */
#define COL_ERR -2

static inline int
color_distance(const struct color_def* a, const struct color_def* b)
{
    int h = MIN((a->h - b->h) % 360, (b->h - a->h) % 360);
    int s = (int)a->s - b->s;
    int l = (int)a->l - b->l;
    return h * h + s * s + l * l;
}

static int
find_closest_col(int h, int s, int l)
{
    struct color_def a = { h, s, l, NULL };
    int min = 0;
    int dmin = color_distance(&a, &color_names[0]);

    for (int i = 1; i < COLOR_NAME_SIZE; i++) {
        int d = color_distance(&a, &color_names[i]);
        if (d < dmin) {
            dmin = d;
            min = i;
        }
    }
    return min;
}

static int
find_col(const char* col_name, int n)
{
    char name[32] = { 0 };

    /*
     * make a null terminated version of col_name. we don't want to
     * use strNcasecmp because we could end up matching blue3 with
     * blue.
     */

    if (n >= (int)sizeof(name)) {
        /* truncate */
        log_error("Color: <%s,%d> bigger than %zu", col_name, n, sizeof(name));
        n = sizeof(name) - 1;
    }
    memcpy(name, col_name, n);

    if (g_ascii_strcasecmp(name, "default") == 0) {
        return -1;
    }

    if (g_ascii_strncasecmp(name, "color", 5) == 0) {
        char* endptr = NULL;
        long num = strtol(name + 5, &endptr, 10);
        if (*endptr == '\0' && num >= 0 && num < COLOR_NAME_SIZE) {
            return (int)num;
        }
    }

    for (int i = 0; i < COLOR_NAME_SIZE; i++) {
        if (g_ascii_strcasecmp(name, color_names[i].name) == 0) {
            return i;
        }
    }

    return COL_ERR;
}

static int
color_hash(const char* str, color_profile profile)
{
    GChecksum* cs = NULL;
    guint8 buf[256] = { 0 };
    gsize len = 256;
    int rc = -1; /* default ncurse color */

    cs = g_checksum_new(G_CHECKSUM_SHA1);
    if (!cs)
        goto out;

    g_checksum_update(cs, (guint8*)str, strlen(str));
    g_checksum_get_digest(cs, buf, &len);

    // sha1 should be 20 bytes
    if (len != 20)
        goto out;

    double h = ((buf[1] << 8) | buf[0]) / 65536. * 360.;

    switch (profile) {
    case COLOR_PROFILE_REDGREEN_BLINDNESS:
        // red/green blindness correction
        h = fmod(fmod(h + 90., 180) - 90., 360.);
        break;
    case COLOR_PROFILE_BLUE_BLINDNESS:
        // blue blindness correction
        h = fmod(h, 180.);
    default:
        break;
    }

    rc = find_closest_col((int)h, 100, 50);

out:
    g_checksum_free(cs);
    return rc;
}

void
color_pair_cache_free(void)
{
    if (cache.pairs) {
        free(cache.pairs);
        memset(&cache, 0, sizeof(cache));
    }
}

void
color_pair_cache_reset(void)
{
    color_pair_cache_free();
    /*
     * COLOR_PAIRS is actually not a macro and is thus not a
     * compile-time constant
     */
    cache.capacity = COLOR_PAIRS;

    /* when we run unit tests COLOR_PAIRS will be -1 */
    if (cache.capacity < 0)
        cache.capacity = 8;

    cache.pairs = g_malloc0(sizeof(*cache.pairs) * cache.capacity);
    if (cache.pairs) {
        /* default_default */
        cache.pairs[0].fg = -1;
        cache.pairs[0].bg = -1;
        cache.size = 1;
    } else {
        log_error("Color: unable to allocate memory");
    }
}

static int
_color_pair_cache_get(int fg, int bg)
{
    if (COLORS < 256) {
        if (fg > 7 || bg > 7) {
            log_error("Color: trying to load 256 colour theme without capable terminal");
            return -1;
        }
    }

    /* try to find pair in cache */
    for (int i = 0; i < cache.size; i++) {
        if (fg == cache.pairs[i].fg && bg == cache.pairs[i].bg) {
            return i;
        }
    }

    /* otherwise cache new pair */

    if (cache.size >= cache.capacity) {
        log_error("Color: reached ncurses color pair cache of %d (COLOR_PAIRS=%d)",
                  cache.capacity, COLOR_PAIRS);
        return -1;
    }

    int i = cache.size;
    cache.pairs[i].fg = fg;
    cache.pairs[i].bg = bg;
    /* (re-)define the new pair in curses */
    init_pair(i, fg, bg);

    cache.size++;

    return i;
}

/**
 * color_pair_cache_hash_str - hash string to a color pair curses id
 *
 * Implements XEP-0392 ("Consistent Color Generation") as best as
 * possible given a 256 colors terminal.
 *
 * hash a string into a color that will be used as fg
 * check for 'bkgnd' in theme file or use default color as bg
 */
int
color_pair_cache_hash_str(const char* str, color_profile profile)
{
    int fg = color_hash(str, profile);
    int bg = -1;

    auto_char char* bkgnd = theme_get_bkgnd();
    if (bkgnd) {
        bg = find_col(bkgnd, strlen(bkgnd));
    }

    return _color_pair_cache_get(fg, bg);
}

/**
 * color_pair_cache_get - parse color pair "fg_bg" and returns curses id
 *
 * if the pair doesn't exist it will allocate it in curses with init_pair
 * if the pair exists it returns its id
 */
int
color_pair_cache_get(const char* pair_name)
{
    const char* sep;
    int fg, bg;

    sep = strchr(pair_name, '_');
    if (!sep) {
        log_error("Color: color pair %s missing", pair_name);
        return -1;
    }

    fg = find_col(pair_name, sep - pair_name);
    bg = find_col(sep + 1, strlen(sep));
    if (fg == COL_ERR || bg == COL_ERR) {
        log_error("Color: bad color name %s", pair_name);
        return -1;
    }

    return _color_pair_cache_get(fg, bg);
}
