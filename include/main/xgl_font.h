#ifndef INCLUDE_MAIN_XGL_FONT_H
#define INCLUDE_MAIN_XGL_FONT_H

/* Functions of src/main/xgl_font.c that other files call. */

int xglFontGetStringWidth(const char *text);
void xglFontPrint(int x, int y, unsigned int color, const char *text);
void xglFontPrintDirectOT(int ot, const char *text);

#endif /* INCLUDE_MAIN_XGL_FONT_H */
