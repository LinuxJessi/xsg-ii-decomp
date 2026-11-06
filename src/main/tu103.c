#include "common.h"
#include "main/tu104.h"

/* Decimal digits at (x, y), 9-pixel pitch, glyph kind 0x1D, digits taken from the value by repeated division by 10. */
INCLUDE_ASM("asm/main/nonmatchings/tu103", textDrawDigits);

/* Digit run of the same family; the loop differs from textDrawDigits (unnamed). */
INCLUDE_ASM("asm/main/nonmatchings/tu103", func_0021B8C8);

/* Digit run from textDrawDigits followed by one glyph of kind 0x63 at x + 9. */
INCLUDE_ASM("asm/main/nonmatchings/tu103", textDrawDigitsPair);

/* Digit run in glyph kind 0x27B with a 12-pixel pitch. */
INCLUDE_ASM("asm/main/nonmatchings/tu103", textDrawDigitsWide);

/* Digit run in glyph kind 0x20C. */
INCLUDE_ASM("asm/main/nonmatchings/tu103", func_0021BB10);

INCLUDE_ASM("asm/main/nonmatchings/tu103", func_0021BBE8);

/* Nine-piece frame (corners, edges, centre) from the record at a0; centre kind 0x109. */
INCLUDE_ASM("asm/main/nonmatchings/tu103", panelDrawNineSlice);

/* Nine-piece frame, sprite bank not yet identified. */
INCLUDE_ASM("asm/main/nonmatchings/tu103", func_0021BFA0);

/* Nine-piece frame with centre kind 0x109 and an extra kind 0xA11. */
INCLUDE_ASM("asm/main/nonmatchings/tu103", func_0021C228);

/* Nine-piece frame, sprite bank 0x108. */
INCLUDE_ASM("asm/main/nonmatchings/tu103", func_0021C498);

/* Record group of eleven submits, sprite kinds 0x116 to 0x11F. */
INCLUDE_ASM("asm/main/nonmatchings/tu103", func_0021C6F8);

/* Record group, sprite kinds 0x135 to 0x137 and 0x11A to 0x11E. */
INCLUDE_ASM("asm/main/nonmatchings/tu103", func_0021CA40);

/* Record group of ten submits, sprite kinds 0x100 to 0x10A. */
INCLUDE_ASM("asm/main/nonmatchings/tu103", func_0021CC08);


/* Calls the tu104 routine menuDrawOutlineFrame and nothing else: the callers' four arguments pass straight through. */
void func_0021CEE0(int x, int y, int width, int height) {
    menuDrawOutlineFrame(x, y, width, height);
}

/* Fixed set of records (kinds 0x52 to 0x5A and 0x118) after func_0021E338. */
INCLUDE_ASM("asm/main/nonmatchings/tu103", func_0021CEF8);

/* func_0021CEF8 with a colour state set first, then restored. */
INCLUDE_ASM("asm/main/nonmatchings/tu103", func_0021D048);

/* func_0021D048 with a second colour word. */
INCLUDE_ASM("asm/main/nonmatchings/tu103", func_0021D1B8);

/* Group of eleven records (kinds 0x226 to 0x231, 0x254) placed from (x, y). */
INCLUDE_ASM("asm/main/nonmatchings/tu103", func_0021D328);

/* Three-record group (kinds 0x13A to 0x13C) at (x, y, width). */
INCLUDE_ASM("asm/main/nonmatchings/tu103", func_0021D550);
