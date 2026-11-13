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

#endif /* INCLUDE_MAIN_TU100_H */
