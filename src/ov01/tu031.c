#include "common.h"
#include "shared.h"
#include "ov01/tu025.h"
#include "ov01/tu031.h"

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

INCLUDE_ASM("asm/nonmatchings/ov01/tu031", tgtFindFirst);

ObjectTask *tgtFindNext(void)
{
    if (tgtIdx >= tgtNum) {
        return 0;
    }
    return tgtTbl[tgtIdx++];
}

int func_00A90660(int first, int second, int value);

/*
 * Words of one task record. tgtCmdAccepts reads the words at 0x00 and 0x08
 * and passes them to func_00A90660; calcActStockSet sets bits in the word at
 * 0x0C. The gap at 0x04 is not evidenced.
 */
struct TaskRecord {
    int target; /* word 0x00: the target the command aims at (tgtFindFirst compares it). */
    u8 unmodeled_04[4];
    int tec_id; /* word 0x08: the technique id whose flags byte func_00A9B3B8 returns. */
    int flags;
};

/* Whether the candidate unit value is an acceptable target for the command record. */
int tgtCmdAccepts(struct TaskRecord *record, int value)
{
    return func_00A90660(record->target, record->tec_id, value) != 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu031", func_00A91708);

INCLUDE_ASM("asm/nonmatchings/ov01/tu031", func_00A91730);

INCLUDE_ASM("asm/nonmatchings/ov01/tu031", func_00A91750);

INCLUDE_ASM("asm/nonmatchings/ov01/tu031", calcBoostOk);

void *actStockGet(void);

/*
 * Marks a task record with 0x200 and stores it in the slot table returned by
 * actStockGet. The slots begin 0x10 bytes into the table; slot 1 first
 * gets boostAddStock with -1.
 */
void calcActStockSet(struct TaskRecord *task, int slot)
{
    void **slots;

    if (slot == 1) {
        boostAddStock(task, -1);
    }
    task->flags |= 0x200;
    slots = actStockGet();
    slots[slot + 4] = task;
}
