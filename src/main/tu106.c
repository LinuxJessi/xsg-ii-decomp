#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu106", msgRangeSetText);

/* A message being typed out in a window. msgRangeSetText stores the escape-coded text and counts its
 * visible glyphs into glyph_count; msgRangeDrawRevealed advances reveal_pos (starts at -8, a short lead-in)
 * and drawn (the token count its draw loop reaches) toward glyph_count as well. */
typedef struct MsgRange {
    unsigned char *text;
    int glyph_count;
    int reveal_pos;
    int drawn;
} MsgRange;

void textDrawSegment(int, int, void *, int);
extern unsigned char msgStyleInit[];
extern unsigned char D_0068ABF0[];
extern unsigned char msgStyleReset[];

/* Clears the reveal so the next draw starts again from the lead-in. */
void msgRangeReset(MsgRange *range)
{
    if (range != 0) {
        range->drawn = 0;
        range->reveal_pos = -8;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu106", msgRangeDrawIcon);

INCLUDE_ASM("asm/main/nonmatchings/tu106", msgRangeDrawRevealed);

/* Returns 1 once every glyph is revealed, 0 while some are still hidden. */
int msgRangeIsRevealed(const MsgRange *range)
{
    if (range->reveal_pos < range->glyph_count) {
        return 0;
    }
    return 1;
}

/* Draws the whole message at (x, y) between the style reset strings. */
void msgRangeDraw(MsgRange *range, int x, int y)
{
    if (range->text != 0) {
        textDrawSegment(0, 0, msgStyleInit, 0);
        textDrawSegment(0, 0, D_0068ABF0, 0);
        textDrawSegment(x, y, range->text, 0);
        textDrawSegment(0, 0, msgStyleReset, 0);
    }
}
