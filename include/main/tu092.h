#ifndef INCLUDE_MAIN_TU092_H
#define INCLUDE_MAIN_TU092_H

/* Functions of src/main/tu092.c that other files call. */

void xglCallbackQueueAdd(int entry);
void xglCdArenaClose(void);
void xglCdBufferClear(void);
int xglCdBufferRewind(void);
void xglCdPendingReadsClear(void);
void xglCdWaitIdle(void);

#endif /* INCLUDE_MAIN_TU092_H */
