#ifndef INCLUDE_MAIN_XGL_FONT_H
#define INCLUDE_MAIN_XGL_FONT_H

/* Functions of src/main/xgl_font.c that other files call. */

int xglFontGetStringWidth(const char *text);
void xglFontPrint(int x, int y, unsigned int color, const char *text);
void xglFontPrintDirectOT(int ot, const char *text);
void xglFontDebugPrintf(int x, int y, const char *format, ...);
/* Copies one text unit (an inline command, a control code with its arguments, or a
 * one- or two-byte character) to word, or to the word buffer when word is 0; returns
 * the text after it and stores its length in *length when given. */
const char *xglFontCopyTextUnit(char *word, const char *text, int *length);

#endif /* INCLUDE_MAIN_XGL_FONT_H */
