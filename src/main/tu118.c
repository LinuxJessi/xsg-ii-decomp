#include "common.h"
#include "main/tu092.h"

INCLUDE_ASM("asm/main/nonmatchings/tu118", func_00231F40);

/* One 0x10-byte row of ysegfStateTable (the current scene state is ysegfState): a one-time start hook,
   a handle queued with xglCallbackQueueAdd on every run, and a finish hook called last. */
typedef struct YsegfStateRecord {
    int initialized;            /* +0x00: 0 until the start hook has run */
    void (*start)(void);        /* +0x04 */
    int handle;                 /* +0x08 */
    void (*finish)(void);       /* +0x0C */
} YsegfStateRecord;


/* Runs one frame of a scene state: start hook once, then the queued handle, then the finish hook. */
void ysegfRunState(YsegfStateRecord *record)
{
    int handle;

    if (record->initialized == 0) {
        record->start();
        record->initialized = 1;
    }
    handle = record->handle;
    if (handle != 0) {
        xglCallbackQueueAdd(handle);
    }
    record->finish();
}

INCLUDE_ASM("asm/main/nonmatchings/tu118", func_00231FD8);
