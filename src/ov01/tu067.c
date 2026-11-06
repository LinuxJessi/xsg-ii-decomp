/*
 * OV01 original TU 67: 0x00aa8b70..0x00aa8c10 (3 functions)
 */
#include "common.h"
#include "ov01/tu001.h"
#include "ov01/tu067.h"

typedef struct ScreenFlashTask ScreenFlashTask;

/* Task returned by func_00A802F8. Only the fields these three functions touch are named.
 * +0x04 is called with the task on each update pass (update); +0x08 is the draw callback.
 * The first and second handler slots of the tu001 record have the same offsets. */
struct ScreenFlashTask {
    unsigned char unmodeled_00[4];
    void (*update)(ScreenFlashTask *task);
    void (*draw)(void);
    int framesLeft;                             /* +0x0C, set to 10 when started */
};

ScreenFlashTask *func_00A802F8(void);
void screenFlashTick(ScreenFlashTask *task);
void grPacketScreenFill(int, int, int);

/* Starts a 10-frame grey screen flash. */
void screenFlashStart(void)
{
    ScreenFlashTask *task;

    task = func_00A802F8();
    if (task != 0) {
        task->update = screenFlashTick;
        task->draw = screenFillGray;
        task->framesLeft = 0xA;
    }
}

/* Per-frame tick: counts framesLeft down to zero, then removes the task. */
void screenFlashTick(ScreenFlashTask *task)
{
    if (task->framesLeft > 0) {
        task->framesLeft = task->framesLeft - 1;
        return;
    }
    ov01TaskRelease(task);
}

/* Draw callback of the flash: a grey (0xC0) screen fill. */
void screenFillGray(void)
{
    grPacketScreenFill(0xC0, 0xC0, 0xC0);
}
