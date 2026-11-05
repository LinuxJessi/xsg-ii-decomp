#include "common.h"
#include "shared.h"

char *caption_nextline(char *cursor);

INCLUDE_ASM("asm/main/nonmatchings/game_movie", func_001E7570);

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

INCLUDE_ASM("asm/main/nonmatchings/game_movie", func_001E7610);

INCLUDE_ASM("asm/main/nonmatchings/game_movie", func_001E7A08);

INCLUDE_ASM("asm/main/nonmatchings/game_movie", func_001E7B08);

INCLUDE_ASM("asm/main/nonmatchings/game_movie", func_001E7C08);

INCLUDE_ASM("asm/main/nonmatchings/game_movie", func_001E7D08);
