/*
 * OV01 original TU 35: 0x00a97180..0x00a97d68 (16 functions)
 */
#include "common.h"
#include "ov01/tu012.h"

typedef struct TgtObject TgtObject;

typedef struct TgtSlot TgtSlot;

typedef struct TgtSelectState TgtSelectState;

/*
 * A battle candidate. Its flags word at +0x0c: 0x08 is set when tgtSelectBegin takes it, 0x10 is the
 * highlight (set by tgtSelectActivate, cleared by tgtSelectUnhighlight below), 0x20 gates the cursor stage,
 * 0x01 is set and 0x1000 cleared when tgtSelectConfirm picks it.
 */
struct TgtObject {
    int unmodeled_00[3];
    int flags;
};

/* Three-entry table set up by tgtSelectInit: index is the slot number, value starts at 180. */
struct TgtSlot {
    int unmodeled_00;
    int index;
    int value;
    int unmodeled_0c[2];
};

/*
 * The target selection state, one block at tgtSelectState. flags holds the stage bits 1 (begin),
 * 2 (activate), 4 (cursor) and 8 (execute) that tgtSelectStep dispatches on. The words at 0x04,
 * 0x30 and 0x54 also have scaffold labels (tgtSelectCurrent, tgtSelectStatus, tgtSelectMode).  Three functions
 * reach those addresses through their labels; the others reach them through this struct.
 */
struct TgtSelectState {
    int flags;
    TgtObject *current;
    TgtObject *last_object;
    int unmodeled_0c[4];
    int phase;
    int cleared_20;
    int cleared_24;
    int active;
    int unmodeled_2c;
    int status;
    int unmodeled_34[8];
    int mode;
    int unmodeled_58[3];
};

extern TgtSlot ov01PartyTable[];
extern TgtSelectState tgtSelectState;
extern TgtObject *tgtSelectCurrent;
extern int tgtSelectStatus;
extern int tgtSelectMode;
void *memset(void *s, int c, unsigned int n);
void func_00A9BCC8(int selector, void *object);
void thinkSendEnd(int value);
void tgtSelectActivate(void);
void tgtSelectBegin(void);
void tgtSelectCursor(int inputHandled);
void tgtSelectExecute(void);
void func_00A977A0(void);
int tgtSelectPadInput(void);

/* Clears the selection state, numbers the three slots and arms the begin stage (flag 1). */
void tgtSelectInit(void)
{
    int i;

    memset(&tgtSelectState, 0, sizeof(tgtSelectState));
    for (i = 0; i < 3; i++) {
        ov01PartyTable[i].index = i;
        ov01PartyTable[i].value = 180;
    }
    tgtSelectState.flags |= 1;
    tgtSelectState.phase = 0;
}

/* Runs the stage handlers whose bits are set in state->flags, then the status update. */
void tgtSelectStep(void)
{
    TgtSelectState *state = &tgtSelectState;
    int inputHandled;

    if (state->flags & 1) {
        tgtSelectBegin();
    }
    if (state->flags & 2) {
        tgtSelectActivate();
    }
    inputHandled = tgtSelectPadInput();
    if (state->flags & 4) {
        tgtSelectCursor(inputHandled);
    }
    if (state->flags & 8) {
        tgtSelectExecute();
    }
    func_00A977A0();
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu035", func_00A97288);

INCLUDE_ASM("asm/nonmatchings/ov01/tu035", tgtSelectBegin);

/* Stage 2: once unitQueueIsEmpty reports the current candidate ready, highlights it and moves to the cursor stage (flag 4). */
void tgtSelectActivate(void)
{
    TgtSelectState *state = &tgtSelectState;
    TgtObject *object;

    if (unitQueueIsEmpty(state->current) != 0) {
        object = state->current;
        object->flags |= 0x10;
        state->active = 1;
        state->mode = 0;
        state->flags = (state->flags & ~2) | 4;
        func_00A9BCC8(4, &object);
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu035", tgtSelectCursor);

/*
 * Confirms object: it becomes last_object and the execute stage (flag 8) is armed. If the current
 * candidate has lost its highlight, the selection is dropped and the cursor stage (flag 4) cleared.
 */
void tgtSelectConfirm(TgtObject *object)
{
    TgtSelectState *state = &tgtSelectState;
    int flags;

    object->flags = (object->flags | 1) & ~0x1000;
    state->last_object = object;
    flags = state->flags | 8;
    state->flags = flags;
    if (!(state->current->flags & 0x10)) {
        state->active = 0;
        state->current = 0;
        state->cleared_24 = 0;
        state->cleared_20 = 0;
        state->flags = flags & ~4;
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu035", tgtSelectExecute);

INCLUDE_ASM("asm/nonmatchings/ov01/tu035", func_00A977A0);

/* Returns the status word at state+0x30 (codes 1 to 5), which func_00A97288 switches on. */
int tgtSelectStatusGet(void)
{
    return tgtSelectStatus;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu035", func_00A97890);

INCLUDE_ASM("asm/nonmatchings/ov01/tu035", tgtSelectPadInput);

void tgtSelectUnhighlight(int value)
{
    tgtSelectCurrent->flags &= ~0x10;
    thinkSendEnd(value);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu035", func_00A97B90);

void tgtSelectModeEnd(void)
{
    tgtSelectMode = 2;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu035", func_00A97C78);
