#ifndef INCLUDE_MAIN_TU242_H
#define INCLUDE_MAIN_TU242_H

/* Functions of src/main/tu242.c that other files call. */

struct StateObject;

void func_00286228(struct StateObject *object, int value);
void msgSlotSetGeometry(struct StateObject *object, int first_value, int second_value, int third_value, int fourth_value);
void textFormatHanTag(char *dst, const char *src);
void textOutputReset(void);
void textSetOutputCursor(int value);
void timecodeClearFlag80(struct StateObject *object);

#endif /* INCLUDE_MAIN_TU242_H */
