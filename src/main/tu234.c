#include "common.h"
#include "main/tu234.h"
#include "main/tu242.h"
#include "main/xgl_font.h"

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_00281CA0);

INCLUDE_ASM("asm/main/nonmatchings/tu234", drawRemainingDebt);

typedef signed char s8;

typedef unsigned char u8;

typedef int s32;

/* Partial view of the object the draw helpers take: only the members these
 * functions read are named. */
typedef struct LabelWidgetState {
    unsigned char unmodeled_00[0x54];
    int **value_ref;
    int flag;
} LabelWidgetState;

typedef struct LabelWidget {
    unsigned char unmodeled_00[8];
    int x;
    int y;
    unsigned char unmodeled_10[0x18];
    LabelWidgetState state;
} LabelWidget;

u8 *fontColorTag(s32 color);
u8 *fontZoomTag(s32 zoom);
int sprintf();
extern unsigned char fontStylePrefix[];
extern unsigned char fmtSevenStrings[];
extern unsigned char fontShadowTagA[];
extern unsigned char fontShadowTagB[];
extern unsigned char fontPropTag[];

/* Draws the string text with a colour tag, a zoom tag and two shadow tags. */
void drawShadowedText(int x, int y, int z, unsigned int *text) {
    int text_y;
    char *color_tag;
    char *zoom_tag;

    text_y = y + 10;
    color_tag = fontColorTag(0x73960A);
    zoom_tag = fontZoomTag(0x12);
    sprintf((char *) 0x70000000, fmtSevenStrings, fontStylePrefix, color_tag, zoom_tag, fontPropTag, fontShadowTagA, text, fontShadowTagB);
    xglFontPrint(x, text_y, z, (char *) 0x70000000);
}

INCLUDE_ASM("asm/main/nonmatchings/tu234", fontPrintIntRow);

INCLUDE_ASM("asm/main/nonmatchings/tu234", textLineCount);

INCLUDE_ASM("asm/main/nonmatchings/tu234", textSkipLines);

INCLUDE_ASM("asm/main/nonmatchings/tu234", textCountLinesBefore);

INCLUDE_ASM("asm/main/nonmatchings/tu234", textBoxMeasure);

INCLUDE_ASM("asm/main/nonmatchings/tu234", textSkipBracketTag);

INCLUDE_ASM("asm/main/nonmatchings/tu234", textTagBody);

INCLUDE_ASM("asm/main/nonmatchings/tu234", drawLineList);

int func_00281CA0();
void drawRemainingDebt();
void drawSellItemList();
void drawSellCloseButtons(LabelWidget *widget);
struct TextPosition {
    /* 0x0 */ unsigned char unmodeled_00[4];
    /* 0x4 */ s32 x;
    /* 0x8 */ s32 y;
    /* 0xC */ s32 color;
};

void drawEmptyNotice(struct TextPosition *pos);
void xglFontFlush();
void xglFontPrintExtFunc();
extern unsigned char func_00283D20[];

/* Draws one sell slot: the action labels, the remaining debt, then the item grid when flag is set or the empty notice. */
void drawSellPanel(LabelWidget *widget) {
    LabelWidgetState *state;
    int **value_ref;
    int debt_x;

    state = &widget->state;
    value_ref = state->value_ref;
    xglFontPrintExtFunc(0, func_00283D20, 1);
    if (state->flag != 0) {
        drawSellCloseButtons(widget);
        if (value_ref != 0) {
            debt_x = func_00281CA0(widget, *value_ref);
            drawRemainingDebt(debt_x, widget->x + 0x25, widget->y + 7, *value_ref);
        }
        xglFontFlush();
        drawSellItemList(widget);
        return;
    }
    drawSellCloseButtons(widget);
    if (value_ref != 0) {
        debt_x = func_00281CA0(widget, *value_ref);
        drawRemainingDebt(debt_x, widget->x + 0x57, widget->y + 7, *value_ref);
    }
    drawEmptyNotice(widget);
    xglFontFlush();
}

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_002825E8);

INCLUDE_ASM("asm/main/nonmatchings/tu234", drawSellItemList);

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_002827C0);

int func_002AA7F0();
char *strchr(char *s, int c);
extern int D_0069DCA0;
extern unsigned char fmtString[];
extern unsigned char sellActionSell[];
extern unsigned char sellActionClose[];

/* Draws the Sell and Close action labels at y 0x15B; the Sell colour is bright when flag is set. */
void drawSellCloseButtons(LabelWidget *widget) {
    LabelWidgetState *state;
    int line_y;
    char *zoom_tag;
    char *color_tag;
    char *text_end;

    state = &widget->state;
    line_y = widget->y + 7;
    sprintf((char *) 0x70000000, fmtString, fontStylePrefix);
    zoom_tag = fontZoomTag(0xE);
    func_002AA7F0((char *) 0x70000000, zoom_tag);
    func_002AA7F0((char *) 0x70000000, fontPropTag);
    text_end = strchr((char *) 0x70000000, 0);
    if (state->flag != 0) {
        color_tag = fontColorTag(0x828282);
        func_002AA7F0((char *) 0x70000000, color_tag);
    } else {
        color_tag = fontColorTag(0x4B4B4B);
        func_002AA7F0((char *) 0x70000000, color_tag);
    }
    if (D_0069DCA0 == 0) {
        func_002AA7F0((char *) 0x70000000, sellActionSell);
    }
    xglFontPrint(0xB9, 0x15B, line_y, (char *) 0x70000000);
    *text_end = '\0';
    color_tag = fontColorTag(0x828282);
    func_002AA7F0((char *) 0x70000000, color_tag);
    if (D_0069DCA0 == 0) {
        func_002AA7F0((char *) 0x70000000, sellActionClose);
    }
    xglFontPrint(0x11D, 0x15B, line_y, (char *) 0x70000000);
}

int textRunLength(int used);
void *memcpy(void *destination, const void *source, unsigned int size);

/* Appends the colour tag for `color` and the run of `src` up to its break to dst; returns the rest of src. */
const char *appendColoredRun(char *dst, const char *src, s32 color) {
    s32 run_len;
    char *end;
    u8 *color_tag;

    run_len = textRunLength(src);
    color_tag = fontColorTag(color);
    sprintf(dst, fmtString, color_tag);
    end = strchr(dst, '\0');
    memcpy(end, src, run_len);
    end[run_len] = '\0';
    return src + run_len;
}

extern unsigned char fmtThreeStrings[];

/* Writes the style prefix, the $prop1 tag and the colour tag for `color` to dest, then the run of `text` up to its break. */
char *appendStyledRun(char *dest, char *text, int color) {
    int run_len;
    char *color_tag;
    char *end;

    run_len = textRunLength(text);
    color_tag = fontColorTag(color);
    sprintf(dest, fmtThreeStrings, fontStylePrefix, fontPropTag, color_tag);
    end = strchr(dest, 0);
    memcpy(end, text, run_len);
    end[run_len] = '\0';
    return text + run_len;
}

INCLUDE_ASM("asm/main/nonmatchings/tu234", sellFormatRow);

int func_00282C00(void);

int textRunLength(int used)
{
    return func_00282C00() - used;
}

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_00282C00);

extern unsigned char sellNoItemsText[];

/* Draws the empty-list notice "No items to sell." at the position in pos (only when flag is zero). */
void drawEmptyNotice(struct TextPosition *pos) {
    u8 *color_tag;

    color_tag = fontColorTag(0x50509C);
    sprintf((u8 *) 0x70000000, fmtThreeStrings, fontStylePrefix, fontPropTag, color_tag);
    if (D_0069DCA0 == 0) {
        func_002AA7F0((u8 *) 0x70000000, sellNoItemsText);
    }
    xglFontPrint(pos->x + 0x7A, pos->y + 0x3B, pos->color, (u8 *) 0x70000000);
}

INCLUDE_ASM("asm/main/nonmatchings/tu234", textMaxLineWidth);

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_00282D50);

typedef unsigned int u32;

s32 sellCountFilledSlots(s32 *words) {
    s32 *end;
    s32 nonzero;
    s32 count;

    nonzero = 0;
    end = words + 39;
    count = 0;
    if (words != 0) {
        if (end >= words) {
            do {
                if (*words != 0) {
                    nonzero = nonzero + 1;
                }
                words++;
            } while (end >= words);
        }
        count = nonzero;
    }
    return count;
}

s32 sellCountFilled(s32 *words, s32 count) {
    s32 *end;
    s32 nonzero;

    end = words + count;
    nonzero = 0;
    if (words < end) {
        do {
            if (*words != 0) {
                nonzero = nonzero + 1;
            }
            words++;
        } while (words < end);
    }
    return nonzero;
}

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_00282E30);

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_00282EE0);

void drawPlainText(s32 x, s32 y, s32 color, const char *text);
void drawAccentText(s32 x, s32 y, s32 color, const char *text);

/* Draws one decimal digit; variant selects the accent colour (drawAccentText) over the plain draw (drawPlainText). */
void drawDigit(s32 x, s32 y, s32 color, s32 digit, s32 variant) {
    char digit_str[2];
    char text_buf[16];

    digit_str[0] = (char) (digit + 0x30);
    digit_str[1] = '\0';
    textFormatHanTag(&text_buf[0], &digit_str[0]);
    if (variant != 0) {
        drawAccentText(x, y, color, &text_buf[0]);
        return;
    }
    drawPlainText(x, y, color, &text_buf[0]);
}

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_00283108);

s32 formatEntryValue(s32 *buffer, void *entry);
s32 func_002838F8(s32 x, s32 y, s32 color, s32 *buffer, s32 length);
s32 func_00283AC8(void *entry);
extern unsigned char func_00283C78[];

struct TextEntry {
    /* 0x0 */ unsigned char unmodeled_00[0xC];
    /* 0xC */ s32 index;
};

/* Draws the entry's formatted value as digit cells at (x, y). */
void drawEntryDigits(struct TextEntry *entry, s32 x, s32 y) {
    s32 digits[4];
    s32 digit_count;

    formatEntryValue(&digits[0], entry);
    digit_count = func_00283AC8(entry);
    xglFontPrintExtFunc(entry->index + 1, func_00283C78, 1);
    func_002838F8(x, y, 0x0FFFFFFF, &digits[0], digit_count);
}

INCLUDE_ASM("asm/main/nonmatchings/tu234", formatEntryValue);

INCLUDE_ASM("asm/main/nonmatchings/tu234", formatEntryDigits);

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_002833F0);

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_00283648);

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_002838F8);

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_00283AC8);

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_00283BC8);

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_00283C30);

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_00283C78);

INCLUDE_ASM("asm/main/nonmatchings/tu234", func_00283D20);

INCLUDE_ASM("asm/main/nonmatchings/tu234", valueDigit);

s32 formatAcquiredText(s32 *buffer, s32 name_text, s32 count);
void drawTextFront(int x, int y, int text);

struct TextBlock {
    /* 0x0 */ unsigned char unmodeled_00[4];
    /* 0x4 */ s32 x;
    /* 0x8 */ s32 y;
    /* 0xC */ s32 index;
    /* 0x10 */ unsigned char unmodeled_10[0x28];
    /* 0x38 */ s32 name_text;
    /* 0x3C */ unsigned char unmodeled_3c[8];
    /* 0x44 */ s32 count;
};

/* Draws the acquired-item message for block: a background packet, the text, then the background off. */
void drawAcquiredText(struct TextBlock *block) {
    s32 acquired_text[0x1C];

    formatAcquiredText(&acquired_text[0], block->name_text, block->count);
    xglFontPrintExtFunc(block->index, func_00283C78, block);
    drawTextFront(block->x + 0x20, block->y + 0x20, &acquired_text[0]);
    xglFontFlush();
    xglFontPrintExtFunc(0, func_00283C78, 1);
}

INCLUDE_ASM("asm/main/nonmatchings/tu234", formatAcquiredText);


/* Pixel width of the acquired-item message for (name_text, count). */
int acquiredTextWidth(int name_text, int count)
{
    char acquired_text[0x70];

    formatAcquiredText(acquired_text, name_text, count);
    return xglFontGetStringWidth(acquired_text);
}

/* Copies `count` bytes from src to dest and NUL-terminates dest. */
void textCopyChars(unsigned char *dest, const unsigned char *src, int count)
{
    unsigned char *out;
    const unsigned char *in;

    out = dest;
    in = src;
    if (count > 0) {
        do {
            *out = *in;
            in++;
            count--;
            out++;
        } while (count > 0);
    }
    *out = 0;
}

/* Flushes the font batch. */
void flushFontBatch(void)
{
    xglFontFlush();
}

/* Partial view: only the members the leaf accessors in this file read. */
typedef struct TextLayout {
    unsigned char unmodeled_00[0x08];
    int base;                          /* +0x08 */
    unsigned char unmodeled_0C[0x28];
    int first_edge;                    /* +0x34 */
    unsigned char unmodeled_38[0x08];
    int last_edge;                     /* +0x40 */
} TextLayout;

int textLastRowY(const TextLayout *layout)
{
    return layout->base - layout->last_edge + 0x20;
}

void *memset(void *s, s32 c, u32 n);
extern unsigned char fmtXAdvance[];

/* Appends an x-advance tag "$ax<advance_x>;" to dst. */
void appendXAdvanceTag(s32 dst, s32 advance_x) {
    u8 tag[0x20];

    memset(&tag[0], 0, sizeof(tag));
    sprintf(&tag[0], fmtXAdvance, advance_x);
    func_002AA7F0(dst, &tag[0]);
}

extern unsigned char fmtDecimal[];

/* Appends the decimal `number` to dst. */
void appendNumberText(s32 dst, s32 number) {
    u8 number_text[0x20];

    memset(&number_text[0], 0, sizeof(number_text));
    sprintf(&number_text[0], fmtDecimal, number);
    func_002AA7F0(dst, &number_text[0]);
}

int textFirstRowY(const TextLayout *layout)
{
    return layout->base - layout->first_edge + 0x20;
}

int sellItemFirstRowY(const TextLayout *layout)
{
    return layout->base - layout->first_edge + 0x48;
}

extern unsigned char fontXOffsetTag[];

void drawPlainText(s32 x, s32 y, s32 color, const char *text) {
    sprintf((u8 *) 0x70000000, fmtThreeStrings, fontStylePrefix, fontXOffsetTag, text);
    xglFontPrint(x, y, color, (u8 *) 0x70000000);
}

extern unsigned char fmtFourStrings[];

void drawAccentText(s32 x, s32 y, s32 color, const char *text) {
    u8 *color_tag;

    color_tag = fontColorTag(0x202050);
    sprintf((u8 *) 0x70000000, fmtFourStrings, fontStylePrefix, fontXOffsetTag, color_tag, text);
    xglFontPrint(x, y - 1, color, (u8 *) 0x70000000);
}

/* Draws text (declared int in the original) at (x, y + 8) with the largest z, in front of the rest. */
void drawTextFront(int x, int y, int text)
{
    xglFontPrint(x, y + 8, 0x7FFFFFFF, text);
}

extern unsigned char fmtColorTag[];
extern unsigned char D_00888798[];

/* Returns the colour tag "$col%06x;" for `color`, formatted into a static buffer. */
u8 *fontColorTag(s32 color) {
    sprintf(D_00888798, fmtColorTag, color);
    return D_00888798;
}

extern unsigned char fmtZoomTag[];
extern unsigned char D_008887B0[];

/* Returns the zoom tag "$zoom%d,%d;" for `zoom`, formatted into a static buffer. */
u8 *fontZoomTag(s32 zoom) {
    sprintf(D_008887B0, fmtZoomTag, zoom, zoom);
    return D_008887B0;
}
