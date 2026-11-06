#ifndef INCLUDE_MAIN_TU045_H
#define INCLUDE_MAIN_TU045_H

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
