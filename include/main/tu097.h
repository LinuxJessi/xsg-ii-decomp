#ifndef INCLUDE_MAIN_TU097_H
#define INCLUDE_MAIN_TU097_H

/* Functions of src/main/tu097.c that other files call. */

struct FontObject;

void fontObjAdvanceTracks(struct FontObject *object);
int * fontObjClearRunLists(void);
void fontObjClearSlots(void);
void fontObjResetAndBuild(void);
void fontObjResetColor(void);
void fontObjResetOpacity(void);
void fontObjSetColor(unsigned long long colour);
void fontObjSetDepth(int depth);
void fontObjSetOpacity(int opacity);
int fontObjSetPosition(void *obj, int x, int y);
void fontObjStartClose(struct FontObject *object);

#endif /* INCLUDE_MAIN_TU097_H */
