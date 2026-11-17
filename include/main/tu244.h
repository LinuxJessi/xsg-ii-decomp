#ifndef INCLUDE_MAIN_TU244_H
#define INCLUDE_MAIN_TU244_H

/* Functions of src/main/tu244.c that other files call. */

struct MsgSlot; /* tu242.h; not included here because tu237.c defines its own MsgWindow */

int sizedRecordCenterX(struct MsgSlot *record);
int sizedRecordCenterY(struct MsgSlot *record);
void sizedRecordSetValue(struct MsgSlot *record, int value);
void grControlPointBlend(int *out_x, int *out_y, float fraction);
void grControlPointSet(int index, int x, int y);
int textIsLineBreak(signed char *text);
signed char *textSkipLineBreak(signed char *text);

#endif /* INCLUDE_MAIN_TU244_H */
