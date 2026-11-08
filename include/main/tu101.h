#ifndef INCLUDE_MAIN_TU101_H
#define INCLUDE_MAIN_TU101_H

/* Functions of src/main/tu101.c that other files call. */

void packetSubmitRequest(int command, int first, int second, int flags);
void packetSubmitRequestEx(int command, int first, int second, int fourth, int fifth, int flags);
void menuDrawKindIcon(int x, int y, int index);
void menuDrawOutlineFrame(int x, int y, int width, int height);
void countdownDraw(void);

#endif /* INCLUDE_MAIN_TU101_H */
