#ifndef INCLUDE_MAIN_TU045_H
#define INCLUDE_MAIN_TU045_H

#include "shared.h"

/* One 16-byte entry of the direct queue (a GIF packet of A+D pairs): data and register address, or four words (the tag,
 * then the header words, are written through the word view). */
typedef union XglDirectQueueEntry {
    struct {
        u64 data;
        u64 address;
    };
    u32 word[4];
} XglDirectQueueEntry;

extern XglDirectQueueEntry *xglDirectQueueBuffer;
extern int xglDirectQueueCount;

/* Functions of src/main/tu045.c that other files call. */

void pxySetRotateFreeze(int value);
void func_001CD134(void);
void func_001CD160(void);
void func_001CD1AC(void);
void pxyResetSubmitCount(void);
int pxyGetSubmitCount(void);
void pxyClearFlushDone(void);
void pxySetFlushDone(void);
void xglPacketAddFlushEIrq(void);
void xglPacketModeReset(void);
void xglPacketSetMode3Flag(int value);
void xglPacketSetMode4Flag(int value);

#endif /* INCLUDE_MAIN_TU045_H */
