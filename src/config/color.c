/*
 * color.c
 *
 * Copyright (C) 2019 Aurelien Aptel <aurelien.aptel@gmail.com>
 *
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <glib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "config/color.h"
#include "log.h"

static
struct color_pair_cache
{
    struct { int16_t fg, bg; } *pairs;
    int size;
    int capacity;
} cache = {0};

/*
 * xterm default 256 colors
 * XXX: there are many duplicates... (eg blue3)
 */
const char *color_names[COLOR_NAME_SIZE] = {
    [0]  = "black",
    [1]  = "red",
    [2]  = "green",
    [3]  = "yellow",
    [4]  = "blue",
    [5]  = "magenta",
    [6]  = "cyan",
    [7]  = "white",
    [8]  = "lightblack",
    [9]  = "lightred",
    [10] = "lightgreen",
    [11] = "lightyellow",
    [12] = "lightblue",
    [13] = "lightmagenta",
    [14] = "lightcyan",
    [15] = "lightwhite",
    [16] = "grey0",
    [17] = "navyblue",
    [18] = "darkblue",
    [19] = "blue3",
    [20] = "blue3",
    [21] = "blue1",
    [22] = "darkgreen",
    [23] = "deepskyblue4",
    [24] = "deepskyblue4",
    [25] = "deepskyblue4",
    [26] = "dodgerblue3",
    [27] = "dodgerblue2",
    [28] = "green4",
    [29] = "springgreen4",
    [30] = "turquoise4",
    [31] = "deepskyblue3",
    [32] = "deepskyblue3",
    [33] = "dodgerblue1",
    [34] = "green3",
    [35] = "springgreen3",
    [36] = "darkcyan",
    [37] = "lightseagreen",
    [38] = "deepskyblue2",
    [39] = "deepskyblue1",
    [40] = "green3",
    [41] = "springgreen3",
    [42] = "springgreen2",
    [43] = "cyan3",
    [44] = "darkturquoise",
    [45] = "turquoise2",
    [46] = "green1",
    [47] = "springgreen2",
    [48] = "springgreen1",
    [49] = "mediumspringgreen",
    [50] = "cyan2",
    [51] = "cyan1",
    [52] = "darkred",
    [53] = "deeppink4",
    [54] = "purple4",
    [55] = "purple4",
    [56] = "purple3",
    [57] = "blueviolet",
    [58] = "orange4",
    [59] = "grey37",
    [60] = "mediumpurple4",
    [61] = "slateblue3",
    [62] = "slateblue3",
    [63] = "royalblue1",
    [64] = "chartreuse4",
    [65] = "darkseagreen4",
    [66] = "paleturquoise4",
    [67] = "steelblue",
    [68] = "steelblue3",
    [69] = "cornflowerblue",
    [70] = "chartreuse3",
    [71] = "darkseagreen4",
    [72] = "cadetblue",
    [73] = "cadetblue",
    [74] = "skyblue3",
    [75] = "steelblue1",
    [76] = "chartreuse3",
    [77] = "palegreen3",
    [78] = "seagreen3",
    [79] = "aquamarine3",
    [80] = "mediumturquoise",
    [81] = "steelblue1",
    [82] = "chartreuse2",
    [83] = "seagreen2",
    [84] = "seagreen1",
    [85] = "seagreen1",
    [86] = "aquamarine1",
    [87] = "darkslategray2",
    [88] = "darkred",
    [89] = "deeppink4",
    [90] = "darkmagenta",
    [91] = "darkmagenta",
    [92] = "darkviolet",
    [93] = "purple",
    [94] = "orange4",
    [95] = "lightpink4",
    [96] = "plum4",
    [97] = "mediumpurple3",
    [98] = "mediumpurple3",
    [99] = "slateblue1",
    [100] = "yellow4",
    [101] = "wheat4",
    [102] = "grey53",
    [103] = "lightslategrey",
    [104] = "mediumpurple",
    [105] = "lightslateblue",
    [106] = "yellow4",
    [107] = "darkolivegreen3",
    [108] = "darkseagreen",
    [109] = "lightskyblue3",
    [110] = "lightskyblue3",
    [111] = "skyblue2",
    [112] = "chartreuse2",
    [113] = "darkolivegreen3",
    [114] = "palegreen3",
    [115] = "darkseagreen3",
    [116] = "darkslategray3",
    [117] = "skyblue1",
    [118] = "chartreuse1",
    [119] = "lightgreen",
    [120] = "lightgreen",
    [121] = "palegreen1",
    [122] = "aquamarine1",
    [123] = "darkslategray1",
    [124] = "red3",
    [125] = "deeppink4",
    [126] = "mediumvioletred",
    [127] = "magenta3",
    [128] = "darkviolet",
    [129] = "purple",
    [130] = "darkorange3",
    [131] = "indianred",
    [132] = "hotpink3",
    [133] = "mediumorchid3",
    [134] = "mediumorchid",
    [135] = "mediumpurple2",
    [136] = "darkgoldenrod",
    [137] = "lightsalmon3",
    [138] = "rosybrown",
    [139] = "grey63",
    [140] = "mediumpurple2",
    [141] = "mediumpurple1",
    [142] = "gold3",
    [143] = "darkkhaki",
    [144] = "navajowhite3",
    [145] = "grey69",
    [146] = "lightsteelblue3",
    [147] = "lightsteelblue",
    [148] = "yellow3",
    [149] = "darkolivegreen3",
    [150] = "darkseagreen3",
    [151] = "darkseagreen2",
    [152] = "lightcyan3",
    [153] = "lightskyblue1",
    [154] = "greenyellow",
    [155] = "darkolivegreen2",
    [156] = "palegreen1",
    [157] = "darkseagreen2",
    [158] = "darkseagreen1",
    [159] = "paleturquoise1",
    [160] = "red3",
    [161] = "deeppink3",
    [162] = "deeppink3",
    [163] = "magenta3",
    [164] = "magenta3",
    [165] = "magenta2",
    [166] = "darkorange3",
    [167] = "indianred",
    [168] = "hotpink3",
    [169] = "hotpink2",
    [170] = "orchid",
    [171] = "mediumorchid1",
    [172] = "orange3",
    [173] = "lightsalmon3",
    [174] = "lightpink3",
    [175] = "pink3",
    [176] = "plum3",
    [177] = "violet",
    [178] = "gold3",
    [179] = "lightgoldenrod3",
    [180] = "tan",
    [181] = "mistyrose3",
    [182] = "thistle3",
    [183] = "plum2",
    [184] = "yellow3",
    [185] = "khaki3",
    [186] = "lightgoldenrod2",
    [187] = "lightyellow3",
    [188] = "grey84",
    [189] = "lightsteelblue1",
    [190] = "yellow2",
    [191] = "darkolivegreen1",
    [192] = "darkolivegreen1",
    [193] = "darkseagreen1",
    [194] = "honeydew2",
    [195] = "lightcyan1",
    [196] = "red1",
    [197] = "deeppink2",
    [198] = "deeppink1",
    [199] = "deeppink1",
    [200] = "magenta2",
    [201] = "magenta1",
    [202] = "orangered1",
    [203] = "indianred1",
    [204] = "indianred1",
    [205] = "hotpink",
    [206] = "hotpink",
    [207] = "mediumorchid1",
    [208] = "darkorange",
    [209] = "salmon1",
    [210] = "lightcoral",
    [211] = "palevioletred1",
    [212] = "orchid2",
    [213] = "orchid1",
    [214] = "orange1",
    [215] = "sandybrown",
    [216] = "lightsalmon1",
    [217] = "lightpink1",
    [218] = "pink1",
    [219] = "plum1",
    [220] = "gold1",
    [221] = "lightgoldenrod2",
    [222] = "lightgoldenrod2",
    [223] = "navajowhite1",
    [224] = "mistyrose1",
    [225] = "thistle1",
    [226] = "yellow1",
    [227] = "lightgoldenrod1",
    [228] = "khaki1",
    [229] = "wheat1",
    [230] = "cornsilk1",
    [231] = "grey100",
    [232] = "grey3",
    [233] = "grey7",
    [234] = "grey11",
    [235] = "grey15",
    [236] = "grey19",
    [237] = "grey23",
    [238] = "grey27",
    [239] = "grey30",
    [240] = "grey35",
    [241] = "grey39",
    [242] = "grey42",
    [243] = "grey46",
    [244] = "grey50",
    [245] = "grey54",
    [246] = "grey58",
    [247] = "grey62",
    [248] = "grey66",
    [249] = "grey70",
    [250] = "grey74",
    [251] = "grey78",
    [252] = "grey82",
    [253] = "grey85",
    [254] = "grey89",
    [255] = "grey93",
};

/* -1 is valid curses color */
#define COL_ERR -2

static int find_col(const char *col_name, int n)
{
    int i;
    char name[32] = {0};

    /*
     * make a null terminated version of col_name. we don't want to
     * use strNcasecmp because we could end up matching blue3 with
     * blue.
     */

    if (n >= sizeof(name)) {
        /* truncate */
        log_error("Color: <%s,%d> bigger than %zu", col_name, n, sizeof(name));
        n = sizeof(name)-1;
    }
    memcpy(name, col_name, n);

    if (g_ascii_strcasecmp(name, "default") == 0) {
        return -1;
    }

    for (i = 0; i < COLOR_NAME_SIZE; i++) {
        if (g_ascii_strcasecmp(name, color_names[i]) == 0) {
            return i;
        }
    }

    return COL_ERR;
}

void color_pair_cache_reset(void)
{
    if (cache.pairs) {
        free(cache.pairs);
        memset(&cache, 0, sizeof(cache));
    }

    /*
     * COLOR_PAIRS is actually not a macro and is thus not a
     * compile-time constant
     */
    cache.capacity = COLOR_PAIRS;

    /* when we run unit tests COLOR_PAIRS will be -1 */
    if (cache.capacity < 0)
        cache.capacity = 8;

    cache.pairs = g_malloc0(sizeof(*cache.pairs)*cache.capacity);
    if (cache.pairs) {
        /* default_default */
        cache.pairs[0].fg = -1;
        cache.pairs[0].bg = -1;
        cache.size = 1;
    } else {
        log_error("Color: unable to allocate memory");
    }
}

/**
 * color_pair_cache_get - parse color pair "fg_bg" and returns curses id
 *
 * if the pair doesn't exist it will allocate it in curses with init_pair
 * if the pair exists it returns its id
 */
int color_pair_cache_get(const char *pair_name)
{
    int i;
    const char *sep;
    int fg, bg;

    sep = strchr(pair_name, '_');
    if (!sep) {
        log_error("Color: color pair %s missing", pair_name);
        return -1;
    }

    fg = find_col(pair_name, sep - pair_name);
    bg = find_col(sep+1, strlen(sep));
    if (fg == COL_ERR || bg == COL_ERR) {
        log_error("Color: bad color name %s", pair_name);
        return -1;
    }

    if (COLORS < 256) {
        if (fg > 7 || bg > 7) {
            log_error("Color: trying to load 256 colour theme without capable terminal");
            return -1;
        }
    }

    /* try to find pair in cache */
    for (i = 0; i < cache.size; i++) {
        if (fg == cache.pairs[i].fg && bg == cache.pairs[i].bg) {
            return i;
        }
    }

    /* otherwise cache new pair */

    if (cache.size >= cache.capacity) {
        log_error("Color: reached ncurses color pair cache of %d", COLOR_PAIRS);
        return -1;
    }

    i = cache.size;
    cache.pairs[i].fg = fg;
    cache.pairs[i].bg = bg;
    /* (re-)define the new pair in curses */
    init_pair(i, fg, bg);

    cache.size++;

    return i;
}
