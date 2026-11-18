#ifndef INCLUDE_MAIN_TU100_H
#define INCLUDE_MAIN_TU100_H

#include "main/tu098.h"

/* Functions of src/main/tu100.c that other files call. */

void menuSceneReset(void);
void menuSkillModelReset(void);
void menuSkillModelTailReset(void);
void menuSkillModelReadDone(int key);
void menuUmnImageDecode(u8 sequenceIndex);
void cursorBufferTablesInit(void);
void cursorBuffersRewind(void);
void cursorBufferSetOrInit(CursorBuffer *buffer, int capacity, int unit_size, int mode_flags);

/* ov01 battle functions (src/ov01/tu023.c) that this file calls. */
int battleItemUse(int key, int value);
int battleSlotSetByte(int first, int index, int dispatch_value);
int battleSlotSetShort(int battle, int slot, int value);
int slotPairSet(int value);
int slotPairTest(int value);

#endif /* INCLUDE_MAIN_TU100_H */
