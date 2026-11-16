#ifndef INCLUDE_MAIN_TU234_H
#define INCLUDE_MAIN_TU234_H

/* Functions of src/main/tu234.c that other files call. */

struct TextBlock;
struct TextLayout;
struct FontScissorCtx;
struct TalkBoxPlace;
struct TextLineSource;
struct LineListBox;
struct LabelWidget;

/*
 * Runtime helpers the compiler and the sell/talk text builders call; no TU of the recovered code
 * defines them (SDK strcat/strncat and libgcc soft-float). Proposal: move to include/shared.h.
 */
char *strcat(char *dst, const char *src);              /* appends src to dst (strcat); callers use the result only in formatAcquiredText */
char *func_002AAE40(char *dst, const char *src, int count);   /* appends at most count characters of src (strncat) */
double litodp(int value);                                     /* libgcc: int -> double */
double fptodp(float value);                                   /* libgcc: float -> double widen */
double dpadd(double a, double b);                             /* libgcc: double add */
double dpmul(double a, double b);                             /* libgcc: double multiply */
long long __divdi3(long long dividend, long long divisor);    /* libgcc: signed 64-bit divide */
float __floatdisf(long long value);                           /* libgcc: long long -> float */
/* The three truncating conversions have no libgcc name in main.externals.ld yet, hence the func_ names. */
char func_002A7618(double value);                             /* double -> int truncate; declared char: matches the original (no sibcall) */
long long func_002A6BB8(float value);                         /* float -> long long truncate */
long long func_002A6A18(double value);                        /* double -> long long truncate */

void drawAcquiredText(struct TextBlock *block);
void drawShadowedText(int x, int y, int z, unsigned int *text);
int sellCountFilledSlots(int *words);
void flushFontBatch(void);
int textLastRowY(const struct TextLayout *layout);
void fontPrintIntRow(int x, int y, int color, char *fmt, int *values);
/* xglFontPrintExtFunc callback: sets the SCISSOR_1 rows of the talk box; the box argument 1 means the full screen. */
void talkBoxScissorCallback(struct FontScissorCtx *ctx, struct TalkBoxPlace *box);

int textLineCount(struct TextLineSource *entry);
char *textSkipLines(char *text, int lines);
int textCountLinesBefore(char *text, char *end);
void textBoxMeasure(int *width, int *height, signed char *text, int *font, signed char *end);
char *textSkipBracketTag(char *text);
char *textTagBody(char *text);
int textMaxLineWidth(struct TextLineSource *entry);
int textCountLinesOf(int text);
int sellCountFilled(int *words, int count);
int valueDigit(long value, int pos);
void talkBoxDrawContents(struct TalkBoxPlace *box);
void drawSellPanel(struct LabelWidget *widget);
void drawLineList(struct LineListBox *box);
int acquiredTextWidth(int name_text, int count);

/* Frame remainder (30 per second) to hundredths of a second, as double: the original multiplies by 3.333. */
#define HUNDREDTHS_PER_FRAME 3.333

#endif /* INCLUDE_MAIN_TU234_H */
