#ifndef INCLUDE_MAIN_TU245_H
#define INCLUDE_MAIN_TU245_H

/* Functions of src/main/tu245.c that other files call. */

struct MsgSlot;

#define TEXT_TAG_COUNT 20      /* entries of the text tag table */

void msgWindowDrawListIcons(struct MsgSlot *window);
void msgWindowLoadBotanXtx(void);
void msgWindowUnloadBotanXtx(void);
void textTagTableClear(void);
int msgWindowStepDirection(int step);
void textTagRemoveKey(int key);
/* Draws the glyphs of the text tags active at text (the revealed end of a text slot), yOffset pixels up. */
void textTagDrawActive(const char *text, int yOffset, struct MsgSlot *slot);

#endif /* INCLUDE_MAIN_TU245_H */
