#ifndef INCLUDE_MAIN_TU234_H
#define INCLUDE_MAIN_TU234_H

/* Functions of src/main/tu234.c that other files call. */

#include "main/tu242.h"

struct FontScissorCtx;

/*
 * SDK string functions the sell/talk text builders call; no TU of the recovered code defines them.
 * (The libgcc soft-float helpers are emitted by the compiler from plain arithmetic and are not declared.)
 */
char *strcat(char *dst, const char *src);              /* appends src to dst (strcat); callers use the result only in formatAcquiredText */
char *strncat(char *dst, const char *src, int count);   /* appends at most count characters of src (strncat) */

void drawAcquiredText(MsgSlot *block);
void drawShadowedText(int x, int y, int z, unsigned int *text);
int sellCountFilledSlots(int *words);
void flushFontBatch(void);
int textLastRowY(const MsgSlot *slot);
void fontPrintIntRow(int x, int y, int color, char *fmt, int *values);
/* xglFontPrintExtFunc callback: sets the SCISSOR_1 rows of the talk box; the box argument 1 means the full screen. */
void talkBoxScissorCallback(struct FontScissorCtx *ctx, MsgSlot *box);

int textLineCount(MsgSlot *entry);
char *textSkipLines(char *text, int lines);
int textCountLinesBefore(char *text, char *end);
void textBoxMeasure(int *width, int *height, signed char *text, int *font, signed char *end);
char *textSkipBracketTag(char *text);
char *textTagBody(char *text);
int textMaxLineWidth(MsgSlot *entry);
int textCountLinesOf(int text);
int sellCountFilled(int *words, int count);
int valueDigit(long value, int pos);
void talkBoxDrawContents(MsgSlot *box);
void drawSellPanel(MsgSlot *widget);
void drawLineList(MsgSlot *box);
int acquiredTextWidth(char *name_text, int count);

/* Frame remainder (30 per second) to hundredths of a second, as double: the original multiplies by 3.333. */
#define HUNDREDTHS_PER_FRAME 3.333

#endif /* INCLUDE_MAIN_TU234_H */
