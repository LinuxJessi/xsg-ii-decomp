#ifndef INCLUDE_MAIN_TU234_H
#define INCLUDE_MAIN_TU234_H

/* Functions of src/main/tu234.c that other files call. */

struct TextBlock;

void drawAcquiredText(struct TextBlock *block);
void drawShadowedText(int x, int y, int z, unsigned int *text);
int sellCountFilledSlots(int *words);

#endif /* INCLUDE_MAIN_TU234_H */
