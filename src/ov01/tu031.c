#include "common.h"
#include "shared.h"

typedef struct ObjectTask ObjectTask;
typedef struct XglTaskPrefix XglTaskPrefix;
typedef struct XglTaskScheduler XglTaskScheduler;

/*
 * Only the link/callback prefix is evidenced by this allocation.  The
 * complete task records are allocated with a 0x80-byte stride, and callers
 * write task-specific payload beyond this 0x10-byte prefix.  This declaration
 * must not be read as the complete task layout or allocation size.
 */
struct XglTaskPrefix {
    XglTaskScheduler *scheduler;
    XglTaskPrefix *next;
    XglTaskPrefix *previous;
    int (*callback)(XglTaskPrefix *task);
};

struct XglTaskScheduler {
    XglTaskPrefix *free_tasks;
    XglTaskPrefix *active_head;
    XglTaskPrefix *active_tail;
    XglTaskPrefix *next_to_visit;
};

struct ObjectTask {
    XglTaskPrefix task; /* the scheduler header xglTaskEntryNext maintains. */
    void *work; /* objRemove passes this slot to objWorkFree. */
};

extern XglTaskPrefix *xglTaskEntryNext(XglTaskScheduler *scheduler,
                                int (*callback)(XglTaskPrefix *task),
                                XglTaskPrefix *entry);

/*
 * tgtFindFirst's own candidate table (this TU, still asm): tgtNum entries
 * of tgtTbl are filled in target-search order, tgtIdx walks them out one at
 * a time. The gap between tgtTbl and tgtIdx (0x00a57e68..0x00a57e88, 0x20
 * bytes) is this array's only size evidence.
 */
extern ObjectTask *tgtTbl[8];

extern int tgtIdx;
extern int tgtNum;

INCLUDE_ASM("asm/nonmatchings/ov01/tu031", func_00A912D0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu031", func_00A913C8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu031", func_00A91418);

ObjectTask *tgtFindNext(void)
{
    if (tgtIdx >= tgtNum) {
        return 0;
    }
    return tgtTbl[tgtIdx++];
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu031", func_00A916E0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu031", func_00A91708);

INCLUDE_ASM("asm/nonmatchings/ov01/tu031", func_00A91730);

INCLUDE_ASM("asm/nonmatchings/ov01/tu031", func_00A91750);

INCLUDE_ASM("asm/nonmatchings/ov01/tu031", func_00A91790);

INCLUDE_ASM("asm/nonmatchings/ov01/tu031", func_00A918B0);
