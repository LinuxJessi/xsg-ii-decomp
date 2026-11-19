#ifndef INCLUDE_OV01_TU068_H
#define INCLUDE_OV01_TU068_H

/* Functions of src/ov01/tu068.c that other files call. */

/* Draws a fill of the given RGB colour over the screen (the grey fill of tu067's flash uses it). */
void grPacketScreenFill(int r, int g, int b);
/* Starts and ends the grey screen fill (a ScreenTask claimed from ov01TaskTable). */
void screenFillStart(void);
void screenFillEnd(void);

#endif /* INCLUDE_OV01_TU068_H */
