#include "common.h"
#include "shared.h"

char *caption_nextline(char *cursor);
void fadeStartOut(int);

/* Calls fadeStartOut(1); the callee is not named yet, so this wrapper is left as GameMovieCaptionFadeOut. */
void GameMovieCaptionFadeOut(void) {
    fadeStartOut(1);
}

char *caption_nextline(char *cursor)
{
    char character;

    do {
        if (*cursor != '\r' && *cursor != '\n') {
            if (*cursor != ';') {
                break;
            }
            cursor++;
        }
        for (;;) {
            character = *cursor++;
            if (character == '\r' || character == '\n') {
                if (*cursor != '\r' && *cursor != '\n' && *cursor != ';') {
                    break;
                }
            }
        }
    } while (0);
    return cursor;
}

/* Proposed caption_getparam(char *cursor, int *time, char *out): parses a time with colons, then the text
 * segments with their width halves; "[base/ruby]" tags expand to the ruby control strings below. */
INCLUDE_ASM("asm/main/nonmatchings/game_movie", caption_getparam);

/* Proposed caption_convert(char *text): the caption control string, then one (time, text) record per line,
 * until a negative time. Same body as X1's caption_convert. */
INCLUDE_ASM("asm/main/nonmatchings/game_movie", caption_convert);

/* Two copies of one sRender-based image transfer packet (func_001E7B08 and func_001E7C08) that differ only in
 * which of sRender +0x10 / +0x12 goes to each half of the register pair. Direction not decided yet, so unnamed. */
INCLUDE_ASM("asm/main/nonmatchings/game_movie", func_001E7B08);

INCLUDE_ASM("asm/main/nonmatchings/game_movie", func_001E7C08);

/* Proposed GameMovieCaptionPlay: reads a caption script with xglCdReadFile, lays the lines out with
 * caption_convert, prints them with xglFontPrintDirect and waits on the pad. Called by the mpeg2 playback. */
INCLUDE_ASM("asm/main/nonmatchings/game_movie", GameMovieCaptionPlay);
