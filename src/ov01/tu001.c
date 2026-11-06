/*
 * OV01 original TU 1: 0x00a801e0..0x00a80368 (5 functions)
 */
#include "common.h"
#include "ov01/tu001.h"

/*
 * One 0x1C-byte task record of the 32-entry table ov01TaskTable (ov01TaskTable). The table holds the tasks that
 * func_00A802F8 claims for the fades (fadeInStart, unitFadeIn), the screen flash and fill and the message
 * object (msgPutObject). Bit 0x1 of flags is the claim bit (ov01SlotClaim); update runs unless flags & 2
 * (update sweep ov01TaskRunUpdate, ov01TaskRunUpdate), draw runs unless flags & 4 (draw sweep ov01TaskRunDraw,
 * ov01TaskRunDraw). A finished task is ended by ov01TaskRelease (ov01TaskRelease).
 */
typedef struct TaskRecord {
    int flags;
    void (*update)(struct TaskRecord *);
    void (*draw)(struct TaskRecord *);
    unsigned char unmodeled_0C[0x1C - 0x0C];
} TaskRecord;

void ov01SlotClear(void *slot);
extern TaskRecord ov01TaskTable[32];

/* Clears all 32 task records (ov01TaskClearAll), called by ov01ClearTables. */
void ov01TaskClearAll(void)
{
    int i;

    for (i = 0; i < 32; i++) {
        ov01SlotClear(&ov01TaskTable[i]);
    }
}

/* Update sweep over the 32 task records (ov01TaskRunUpdate): runs each record's update unless flags & 2. */
void ov01TaskRunUpdate(void)
{
    void (*update)(TaskRecord *);
    TaskRecord *record;
    int i;

    record = ov01TaskTable;
    for (i = 0; i < 32; i++) {
        update = record->update;
        if (update != 0 && !(record->flags & 2)) {
            update(record);
        }
        record++;
    }
}

/* Draw sweep over the 32 task records (ov01TaskRunDraw): runs each record's draw unless flags & 4. */
void ov01TaskRunDraw(void)
{
    void (*draw)(TaskRecord *);
    TaskRecord *record;
    int i;

    record = ov01TaskTable;
    for (i = 0; i < 32; i++) {
        draw = record->draw;
        if (draw != 0 && !(record->flags & 4)) {
            draw(record);
        }
        record++;
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu001", func_00A802F8);

void ov01SlotRelease(void *slot);

/* Ends a task (ov01TaskRelease): the fade steps, screenFlashTick, screenFillEnd and msgObjRelease call it. */
void ov01TaskRelease(void *task)
{
    ov01SlotRelease(task);
}
