/*******************************************************************************
 * Size: 14 px
 * Bpp: 1
 * Opts: --bpp 1 --size 14 --font /Users/mikas/SquareLine/assets/Black Ops One Regular.ttf -o /Users/mikas/SquareLine/assets/ui_font_Black_Ops_One_14.c --format lvgl -r 0x20-0x7f --symbols ° --no-compress --no-prefilter
 ******************************************************************************/

#include "../ui.h"

#ifndef UI_FONT_BLACK_OPS_ONE_14
#define UI_FONT_BLACK_OPS_ONE_14 1
#endif

#if UI_FONT_BLACK_OPS_ONE_14

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0xff, 0xc0, 0xe0,

    /* U+0022 "\"" */
    0xef, 0xdf, 0xbf, 0x70,

    /* U+0023 "#" */
    0x13, 0x19, 0x9e, 0xe6, 0x43, 0x21, 0x93, 0xee,
    0x6c, 0x26, 0x0,

    /* U+0024 "$" */
    0x1c, 0x71, 0xf8, 0x1c, 0xe, 0x3, 0xf8, 0xe,
    0x7, 0x1b, 0xbd, 0x86, 0x0,

    /* U+0025 "%" */
    0x0, 0xc1, 0xe7, 0xf, 0xd8, 0x3f, 0x60, 0xff,
    0x1, 0xed, 0xe0, 0x7f, 0xc1, 0xbf, 0x6, 0xfc,
    0x31, 0xe0, 0xc0, 0x7, 0x0,

    /* U+0026 "&" */
    0x3f, 0x1c, 0xe7, 0x80, 0xe2, 0x5d, 0xbb, 0xfe,
    0xfb, 0x9c, 0x7b, 0x80,

    /* U+0027 "'" */
    0xff, 0xf0,

    /* U+0028 "(" */
    0x7e, 0xee, 0xee, 0xee, 0x70,

    /* U+0029 ")" */
    0xe7, 0x77, 0x77, 0x77, 0xe0,

    /* U+002A "*" */
    0x18, 0x33, 0xfb, 0xf3, 0xcf, 0x89, 0x0,

    /* U+002B "+" */
    0x18, 0x30, 0x67, 0xf1, 0x83, 0x6, 0x0,

    /* U+002C "," */
    0xef, 0x0,

    /* U+002D "-" */
    0xfc,

    /* U+002E "." */
    0xe0,

    /* U+002F "/" */
    0x4, 0x30, 0xc6, 0x18, 0x43, 0xc, 0x61, 0x84,
    0x0,

    /* U+0030 "0" */
    0x7b, 0x71, 0xf8, 0xfc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xc7, 0x7f, 0x0,

    /* U+0031 "1" */
    0xf8, 0xe3, 0x8e, 0x38, 0xe3, 0x80, 0xfc,

    /* U+0032 "2" */
    0x7f, 0x71, 0xf8, 0xe0, 0x70, 0x3b, 0xfb, 0x81,
    0xc0, 0xef, 0x80,

    /* U+0033 "3" */
    0x7b, 0x71, 0xf8, 0xe0, 0x70, 0xf0, 0x1c, 0xf,
    0xc7, 0x7b, 0x0,

    /* U+0034 "4" */
    0x7, 0x7, 0x87, 0xc7, 0xe7, 0x77, 0x3b, 0xfe,
    0xe, 0x7, 0x0,

    /* U+0035 "5" */
    0xff, 0xf0, 0x38, 0x1c, 0xe, 0xf0, 0x1c, 0xf,
    0xc7, 0x7f, 0x0,

    /* U+0036 "6" */
    0x77, 0x71, 0xf8, 0x1c, 0xf, 0xf7, 0x1f, 0x8f,
    0xc7, 0x77, 0x0,

    /* U+0037 "7" */
    0xff, 0xe0, 0xe0, 0xe7, 0xe, 0xe, 0x1c, 0x1c,
    0x38,

    /* U+0038 "8" */
    0x77, 0x71, 0xf8, 0xfc, 0x77, 0x77, 0x1f, 0x8f,
    0xc7, 0x77, 0x0,

    /* U+0039 "9" */
    0x77, 0x71, 0xf8, 0xfc, 0x77, 0xb8, 0x1c, 0xf,
    0xc7, 0x77, 0x0,

    /* U+003A ":" */
    0xe0, 0x0, 0x38,

    /* U+003B ";" */
    0xe0, 0x0, 0x3b, 0xc0,

    /* U+003C "<" */
    0x3, 0x3f, 0xf0, 0x38, 0x5f, 0x3,

    /* U+003D "=" */
    0xfe, 0x0, 0x0, 0xf, 0xe0,

    /* U+003E ">" */
    0xc0, 0xfc, 0x19, 0x1f, 0xf8, 0x80,

    /* U+003F "?" */
    0x7f, 0x79, 0xc0, 0xe0, 0x71, 0xc0, 0x0, 0x0,
    0x0, 0x1c, 0x0,

    /* U+0040 "@" */
    0x37, 0x38, 0x7c, 0xf, 0x23, 0xc8, 0xf2, 0x3c,
    0x83, 0x90, 0x37, 0x0,

    /* U+0041 "A" */
    0xe, 0x7, 0x81, 0xf0, 0xdc, 0x3f, 0x1c, 0xe7,
    0x39, 0xff, 0xe1, 0xc0,

    /* U+0042 "B" */
    0xef, 0x71, 0xf8, 0xfc, 0x7e, 0xf7, 0x1f, 0x8f,
    0xc7, 0xef, 0x0,

    /* U+0043 "C" */
    0x6f, 0x71, 0xf8, 0xfc, 0xe, 0x7, 0x3, 0x8f,
    0xc7, 0x6f, 0x0,

    /* U+0044 "D" */
    0xef, 0x71, 0xf8, 0xfc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xc7, 0xef, 0x0,

    /* U+0045 "E" */
    0xff, 0xe7, 0xe0, 0xe0, 0xfc, 0xe0, 0xe0, 0xe7,
    0xff,

    /* U+0046 "F" */
    0xff, 0xe7, 0xe0, 0xe0, 0xe0, 0xfc, 0xe0, 0xe0,
    0xe0,

    /* U+0047 "G" */
    0x7f, 0x71, 0xf8, 0x1c, 0xe, 0x7, 0x3f, 0x8f,
    0xc7, 0x7f, 0x80,

    /* U+0048 "H" */
    0xe3, 0xf1, 0xf8, 0xfc, 0x7e, 0xff, 0x1f, 0x8f,
    0xc7, 0xe3, 0x80,

    /* U+0049 "I" */
    0xfb, 0x9c, 0xe7, 0x39, 0xce, 0xf8,

    /* U+004A "J" */
    0x7f, 0x7, 0x7, 0x7, 0x7, 0xe7, 0xe7, 0xe7,
    0x7e,

    /* U+004B "K" */
    0xe7, 0x73, 0xbb, 0x9f, 0x8f, 0xe7, 0xd3, 0xdd,
    0xee, 0xe7, 0x80,

    /* U+004C "L" */
    0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe0, 0xe7,
    0xff,

    /* U+004D "M" */
    0xf0, 0xee, 0x3e, 0xe7, 0xfd, 0xfd, 0xff, 0xfd,
    0xfb, 0xbf, 0x27, 0xe4, 0xe0,

    /* U+004E "N" */
    0xf3, 0xb9, 0xee, 0xff, 0xff, 0xff, 0xff, 0xbb,
    0xce, 0xe7, 0x80,

    /* U+004F "O" */
    0x77, 0x71, 0xf8, 0xfc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xc7, 0x77, 0x0,

    /* U+0050 "P" */
    0xef, 0x71, 0xf8, 0xfc, 0x7e, 0x3f, 0x7b, 0x81,
    0xc0, 0xe0, 0x0,

    /* U+0051 "Q" */
    0x77, 0x71, 0xf8, 0xfc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xc7, 0x7d, 0x3, 0x80, 0xc0,

    /* U+0052 "R" */
    0xef, 0x71, 0xf8, 0xfc, 0x7e, 0x3f, 0x7b, 0x85,
    0xc3, 0xe7, 0x80,

    /* U+0053 "S" */
    0x7f, 0x71, 0xf8, 0x1c, 0x7, 0xf0, 0x1c, 0xf,
    0xc7, 0x7f, 0x0,

    /* U+0054 "T" */
    0xff, 0x0, 0x0, 0x38, 0x38, 0x38, 0x38, 0x38,
    0x38,

    /* U+0055 "U" */
    0xe3, 0xf1, 0xf8, 0xfc, 0x7e, 0x3f, 0x1f, 0x8f,
    0xc7, 0x77, 0x0,

    /* U+0056 "V" */
    0xe3, 0xb8, 0xe7, 0x39, 0xdc, 0x77, 0xf, 0xc3,
    0xa0, 0x78, 0x1c, 0x0,

    /* U+0057 "W" */
    0xe7, 0x3f, 0x1d, 0xde, 0xee, 0xff, 0x67, 0xfb,
    0x3e, 0xf8, 0xf7, 0x87, 0xbc, 0x38, 0xc0,

    /* U+0058 "X" */
    0xf3, 0xbb, 0x8f, 0xc7, 0xc1, 0xc1, 0x70, 0xfc,
    0xee, 0xf3, 0x80,

    /* U+0059 "Y" */
    0xe3, 0xbb, 0x9d, 0xc7, 0xc3, 0xa0, 0xe0, 0x70,
    0x38, 0x1c, 0x0,

    /* U+005A "Z" */
    0xf3, 0x7, 0xe, 0x1c, 0x18, 0x38, 0x70, 0xe0,
    0xdf,

    /* U+005B "[" */
    0xfe, 0xee, 0xee, 0xee, 0xf0,

    /* U+005C "\\" */
    0x83, 0x4, 0x18, 0x60, 0x83, 0x4, 0x18, 0x60,
    0x80,

    /* U+005D "]" */
    0xf7, 0x77, 0x77, 0x77, 0xf0,

    /* U+005E "^" */
    0x10, 0x70, 0xe3, 0x46, 0xc9, 0xb1, 0x0,

    /* U+005F "_" */
    0xff, 0xc0,

    /* U+0060 "`" */
    0xe, 0x70,

    /* U+0061 "a" */
    0x7e, 0x7, 0x7, 0x7f, 0xe7, 0xe7, 0xf7,

    /* U+0062 "b" */
    0xe0, 0xe0, 0xe0, 0xee, 0xe7, 0xe7, 0xe7, 0xe7,
    0xe7, 0xee,

    /* U+0063 "c" */
    0x6e, 0xe7, 0xe0, 0xe0, 0xe0, 0xe7, 0x6e,

    /* U+0064 "d" */
    0x7, 0x7, 0x7, 0x7f, 0xe7, 0xe7, 0xe7, 0xe7,
    0xe7, 0x7f,

    /* U+0065 "e" */
    0x6e, 0xe7, 0xe7, 0xef, 0xe0, 0xe0, 0x6f,

    /* U+0066 "f" */
    0x3d, 0xc7, 0x3f, 0x71, 0xc7, 0x1c, 0x71, 0xc0,

    /* U+0067 "g" */
    0x6e, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0x77, 0x7,
    0x7e,

    /* U+0068 "h" */
    0xe0, 0xe0, 0xe0, 0xee, 0xe7, 0xe7, 0xe7, 0xe7,
    0xe7, 0xe7,

    /* U+0069 "i" */
    0xe0, 0x7f, 0xff, 0xfc,

    /* U+006A "j" */
    0x38, 0x0, 0x73, 0x9c, 0xe7, 0x39, 0xcf, 0xe0,

    /* U+006B "k" */
    0xe0, 0xe0, 0xe0, 0xe7, 0xee, 0xfc, 0xf8, 0xfe,
    0xfe, 0xe7,

    /* U+006C "l" */
    0xe7, 0x39, 0xce, 0x73, 0x9c, 0xe3, 0xc0,

    /* U+006D "m" */
    0xef, 0x77, 0x39, 0xf9, 0xcf, 0xce, 0x7e, 0x73,
    0xf3, 0x9f, 0x9c, 0xe0,

    /* U+006E "n" */
    0xee, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7,

    /* U+006F "o" */
    0x7e, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0x7e,

    /* U+0070 "p" */
    0xee, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xee, 0xe0,
    0xe0,

    /* U+0071 "q" */
    0x6e, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0x77, 0x7,
    0x7,

    /* U+0072 "r" */
    0xee, 0xe7, 0xe7, 0xe0, 0xe0, 0xe0, 0xe0,

    /* U+0073 "s" */
    0x6f, 0xe0, 0xe0, 0x7e, 0x7, 0xe7, 0x7e,

    /* U+0074 "t" */
    0x11, 0xcf, 0xdc, 0x71, 0xc7, 0x1c, 0x3c,

    /* U+0075 "u" */
    0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0xe7, 0x77,

    /* U+0076 "v" */
    0xe7, 0xe7, 0x76, 0x76, 0x3e, 0x3c, 0x38,

    /* U+0077 "w" */
    0xe7, 0x3f, 0x3b, 0x9e, 0xfc, 0xff, 0xc3, 0xfa,
    0x1e, 0xf0, 0xe7, 0x0,

    /* U+0078 "x" */
    0xef, 0x76, 0x3c, 0x38, 0x7c, 0x6e, 0xef,

    /* U+0079 "y" */
    0xe7, 0x67, 0x7e, 0x7e, 0x2e, 0x3c, 0x1c, 0x38,
    0x38,

    /* U+007A "z" */
    0xee, 0x1c, 0x71, 0xc7, 0x1c, 0x37, 0x80,

    /* U+007B "{" */
    0x3b, 0x9c, 0xee, 0x39, 0xce, 0x38,

    /* U+007C "|" */
    0xff, 0xff, 0xf0,

    /* U+007D "}" */
    0xe3, 0x9c, 0xe3, 0xb9, 0xce, 0xe0,

    /* U+007E "~" */
    0x78, 0x99, 0xe,

    /* U+00B0 "°" */
    0x7b, 0x3c, 0xf3, 0x78
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 58, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 82, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 5, .adv_w = 149, .box_w = 7, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 9, .adv_w = 186, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 20, .adv_w = 191, .box_w = 9, .box_h = 11, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 33, .adv_w = 262, .box_w = 14, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 54, .adv_w = 182, .box_w = 10, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 66, .adv_w = 85, .box_w = 3, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 68, .adv_w = 107, .box_w = 4, .box_h = 9, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 73, .adv_w = 107, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 78, .adv_w = 152, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 85, .adv_w = 157, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 92, .adv_w = 81, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 94, .adv_w = 137, .box_w = 6, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 95, .adv_w = 81, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 96, .adv_w = 82, .box_w = 6, .box_h = 11, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 105, .adv_w = 164, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 116, .adv_w = 128, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 134, .adv_w = 164, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 145, .adv_w = 152, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 156, .adv_w = 164, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 164, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 178, .adv_w = 142, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 187, .adv_w = 169, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 198, .adv_w = 164, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 81, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 212, .adv_w = 81, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 216, .adv_w = 157, .box_w = 8, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 222, .adv_w = 157, .box_w = 7, .box_h = 5, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 227, .adv_w = 157, .box_w = 8, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 233, .adv_w = 154, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 244, .adv_w = 173, .box_w = 10, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 256, .adv_w = 164, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 268, .adv_w = 164, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 279, .adv_w = 158, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 290, .adv_w = 163, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 301, .adv_w = 155, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 310, .adv_w = 145, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 319, .adv_w = 164, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 330, .adv_w = 166, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 341, .adv_w = 99, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 347, .adv_w = 150, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 356, .adv_w = 150, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 367, .adv_w = 139, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 376, .adv_w = 202, .box_w = 11, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 389, .adv_w = 165, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 400, .adv_w = 160, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 411, .adv_w = 159, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 422, .adv_w = 162, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 435, .adv_w = 163, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 446, .adv_w = 162, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 457, .adv_w = 137, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 466, .adv_w = 164, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 477, .adv_w = 150, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 489, .adv_w = 211, .box_w = 13, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 504, .adv_w = 147, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 515, .adv_w = 142, .box_w = 9, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 526, .adv_w = 139, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 535, .adv_w = 107, .box_w = 4, .box_h = 9, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 540, .adv_w = 82, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 549, .adv_w = 107, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 554, .adv_w = 141, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 561, .adv_w = 146, .box_w = 10, .box_h = 1, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 563, .adv_w = 120, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 8},
    {.bitmap_index = 565, .adv_w = 146, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 572, .adv_w = 147, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 582, .adv_w = 145, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 589, .adv_w = 149, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 599, .adv_w = 146, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 606, .adv_w = 99, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 614, .adv_w = 147, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 623, .adv_w = 148, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 633, .adv_w = 71, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 637, .adv_w = 71, .box_w = 5, .box_h = 12, .ofs_x = -1, .ofs_y = -2},
    {.bitmap_index = 645, .adv_w = 144, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 655, .adv_w = 82, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 662, .adv_w = 223, .box_w = 13, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 674, .adv_w = 148, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 681, .adv_w = 151, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 688, .adv_w = 148, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 697, .adv_w = 148, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 706, .adv_w = 140, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 713, .adv_w = 144, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 720, .adv_w = 100, .box_w = 6, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 727, .adv_w = 148, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 734, .adv_w = 130, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 741, .adv_w = 202, .box_w = 13, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 753, .adv_w = 125, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 760, .adv_w = 134, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 769, .adv_w = 129, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 776, .adv_w = 107, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 782, .adv_w = 77, .box_w = 2, .box_h = 10, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 785, .adv_w = 107, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 791, .adv_w = 163, .box_w = 8, .box_h = 3, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 794, .adv_w = 128, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 3}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 176, .range_length = 1, .glyph_id_start = 96,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Pair left and right glyphs for kerning*/
static const uint8_t kern_pair_glyph_ids[] =
{
    34, 53,
    34, 55,
    34, 58,
    34, 87,
    34, 90,
    39, 13,
    39, 15,
    39, 34,
    39, 66,
    39, 70,
    39, 80,
    45, 53,
    45, 55,
    45, 58,
    49, 34,
    53, 13,
    53, 15,
    53, 34,
    53, 66,
    53, 70,
    53, 80,
    55, 34,
    55, 66,
    55, 70,
    55, 80,
    55, 83,
    55, 86,
    57, 70,
    57, 80,
    58, 34,
    58, 66,
    58, 69,
    58, 70,
    58, 80,
    71, 13,
    71, 15,
    72, 90,
    84, 90,
    90, 84
};

/* Kerning between the respective left and right glyphs
 * 4.4 format which needs to scaled with `kern_scale`*/
static const int8_t kern_pair_values[] =
{
    -15, -19, -19, -14, -10, -28, -28, -19,
    -9, -9, -9, -19, -19, -19, -10, -19,
    -19, -19, -10, -10, -10, -19, -14, -14,
    -14, -7, -7, -9, -9, -19, -19, -9,
    -14, -14, -14, -14, -5, -7, -5
};

/*Collect the kern pair's data in one place*/
static const lv_font_fmt_txt_kern_pair_t kern_pairs =
{
    .glyph_ids = kern_pair_glyph_ids,
    .values = kern_pair_values,
    .pair_cnt = 39,
    .glyph_ids_size = 0
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_pairs,
    .kern_scale = 16,
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};



/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t ui_font_Black_Ops_One_14 = {
#else
lv_font_t ui_font_Black_Ops_One_14 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 13,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if UI_FONT_BLACK_OPS_ONE_14*/

