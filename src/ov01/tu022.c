#include "common.h"
#include "shared.h"
#include "ov01/tu005.h"
#include "ov01/tu022.h"
#include "ov01/tu057.h"
#include "ov01/tu059.h"
#include "ov01/tu060.h"
#include "ov01/tu061.h"

extern int padMode;
extern short padData;

#define VIRTUAL_PAD_BIT_COUNT 14

extern unsigned short padConvTbl[VIRTUAL_PAD_BIT_COUNT];

/* Global block dataState (proposed dataState): the DataCursor fields (0xA4-0xAC) and the mode word (0xB4)
 * share it; the block is 0xBC bytes, cleared by dataStateReset. */
struct DataState {
    /* 0x00 */ unsigned char unmodeled_00[0xB4];
    /* 0xB4 */ unsigned int mode;
    /* 0xB8 */ unsigned char unmodeled_B8[4];
};

/* Object made by dataObjCreate (global dataObject, proposed dataObject). Seen in the code: 0x04 update
 * callback (dataObjUpdate), 0x08 draw callback (dataObjDraw), 0x0C flags (0x2 hidden, 0x1 set by
 * dataTargetMatchKey), 0x14 lookup block, 0x150 position (DataVec3), 0x1D0 id. */
struct DataObject {
    /* 0x00 */ unsigned char unmodeled_00[0xC];
    /* 0x0C */ int flags;
    /* 0x10 */ unsigned char unmodeled_10[0x1C0];
    /* 0x1D0 */ int id;
};

struct DataVec3 {
    f32 x;
    f32 y;
    f32 z;
};

extern struct DataState dataState;
extern struct DataObject *dataObject;
extern int D_00AB596C;
void menuWepSpPanelReset(void);
void *dataObjCreate(f32, f32, f32);
void func_00A89CB0(struct DataObject *obj, u32 *packet);
void menuTexFlush(void);
void grGpInit(void *packet);
void grPacketSend(void *packet);
int sefActorMulLocalVector(void *actor, void *input, void *output, int apply);
void *memset(void *dst, int value, unsigned int size);

/* Virtual pad reset (proposed dataVPadReset): pad mode and pad data, then the two counter pairs. */
void dataVPadReset(void) {
    dataVPadModeSet(0);
    dataCountersClear();
    menuWepSpPanelReset();
}

void func_00A88E98(void) {
    dataCountersClear();
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A88EB0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89038);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A891C0);

extern int D_00AC1078;
extern int D_00AC1098;

/* Clears the counter pair D_00AC1098 / D_00AC1078 (proposed dataCountersClear). */
void dataCountersClear(void) {
    D_00AC1098 = 0;
    D_00AC1078 = 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89280);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89340);

void dataVPadModeSet(int mode)
{
    padMode = mode;
    padData = 0;
}

void dataVPadSet(int virtualPad)
{
    unsigned short result = 0;
    int shift = 0;
    const unsigned short *entry = padConvTbl;
    virtualPad &= 0xFFFF;
    /* Original entry at 0x00A1D2A0 enters the body before testing the
     * loop count; keep the table pointer advance and 16-bit mask. */
    do {
        if ((virtualPad >> shift) & 1) {
            result |= *entry;
        }
        shift++;
        entry++;
    } while (shift < VIRTUAL_PAD_BIT_COUNT);
    padData = result;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", dataPadRead);

extern int menuWepSpPanelCursor;
extern int menuWepSpPanelRowCount;

/* Clears the pair menuWepSpPanelCursor / menuWepSpPanelRowCount. Their meaning is still open. */
void menuWepSpPanelReset(void) {
    menuWepSpPanelCursor = 0;
    menuWepSpPanelRowCount = 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89538);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89580);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89710);

int menuWepSpPanelCursorGet(void) {
    return menuWepSpPanelCursor;
}

int menuWepSpPanelRowCountGet(void) {
    return menuWepSpPanelRowCount;
}

extern int menuTargetListCursor;

void menuTargetListCursorReset(void) {
    menuTargetListCursor = 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A898B8);

int menuTargetListCursorGet(void) {
    return menuTargetListCursor;
}

/* Clears the DataState block and the object pointer (proposed dataStateReset). */
void dataStateReset(void) {
    memset(&dataState, 0, sizeof(dataState));
    dataObject = 0;
}

/* Creates the object at the origin and keeps it in dataObject (proposed dataObjCreateAtOrigin). */
void dataObjCreateAtOrigin(void) {
    void *result;

    result = dataObjCreate(0.0f, 0.0f, 0.0f);
    dataObject = result;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", dataObjCreate);

/* Frees the object and clears its pointer when present (proposed dataObjRemove). */
void dataObjRemove(void) {
    if (dataObject != 0) {
        dataObjRelease(dataObject);
        dataObject = 0;
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", dataObjUpdate);

/* Draw callback (+0x08) of dataObjCreate, proposed dataObjDraw: sends a GS packet unless the object is hidden (flag 0x2). */
void dataObjDraw(struct DataObject *obj) {
    u32 packet[4];

    if (!(obj->flags & 2)) {
        grGpInit(packet);
        menuTexFlush();
        func_00A89CB0(obj, packet);
        grPacketSend(packet);
    }
}

/* Reads the object's position from its id (proposed dataObjGetPosition). */
void dataObjGetPosition(struct DataObject *obj, struct DataVec3 *out) {
    struct DataVec3 vec;
    f32 x;
    f32 y;
    f32 z;

    sefActorMulLocalVector(obj->id, 2, &vec, 1);
    x = vec.x;
    y = vec.y;
    z = vec.z;
    out->z = z;
    out->y = y;
    out->x = x;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89CB0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89D68);

typedef struct DataTarget DataTarget;

struct DataTarget {
    unsigned char unmodeled_00[0x0C];
    int flags;                /* +0x0C */
    int applied;              /* +0x10 */
    unsigned char lookup[1];  /* +0x14, trailing block; only its address is used */
};

typedef struct DataCursor DataCursor;

struct DataCursor {
    unsigned char unmodeled_00[0x24];
    int entry[32];            /* +0x24 */
    int count;                /* +0xA4 */
    int index;                /* +0xA8 */
    int prev_index;           /* +0xAC */
};

int dataPadRead(int port);
void seVoiceStartDefault(int id);
int *dataLookupAppend(void *lookup, int key);

/* Partial layouts: only the path dataTargetHide follows from tgtUnitGet(). */
typedef struct TgtUnitStatus {
    unsigned char unmodeled_00[0xEC];
    int word_EC;
} TgtUnitStatus;

typedef struct TgtUnit {
    unsigned char unmodeled_000[0x144];
    TgtUnitStatus *status;
} TgtUnit;

void func_00A89F08(DataTarget *target, DataCursor *cursor);
void *tgtUnitGet(void);

void dataTargetHide(void)
{
    TgtUnit *unit;

    dataObject->flags |= 2;
    func_00A89F08(dataObject, &dataState);
    menuTgtStatusHide();
    menuUnitPanelArmTimer();
    menuTimedSeqArmTick();
    unit = tgtUnitGet();
    if ((unit == 0) || (unit->status->word_EC <= 0)) {
        menuIconListClose();
    }
}

/* Clears the hidden flag 0x2 (proposed dataObjShow). */
void dataObjShow(void) {
    dataObject->flags &= ~2;
}

/* Sets the hidden flag 0x2 (proposed dataObjHide). */
void dataObjHide(void) {
    dataObject->flags |= 2;
}

/* Sets flag 0x1 on the target when the lookup for key is nonzero (proposed dataTargetMatchKey). */
void dataTargetMatchKey(DataTarget *target, int key)
{
    dataLookupReset(target->lookup);
    if (*dataLookupAppend(target->lookup, key) != 0) {
        target->flags |= 1;
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A89F08);

/* Cursor move (proposed dataCursorMove): next or prev pad bits step the index, wrapping at count, with
 * sound 0x01000001; the target is re-applied when its entry changed. Returns the new index. */
int dataCursorMove(DataTarget *target, DataCursor *cursor, int next_mask, int prev_mask)
{
    int forward_index;
    int backward_index;

    cursor->prev_index = cursor->index;
    if (dataPadRead(1) & next_mask) {
        seVoiceStartDefault(0x01000001);
        forward_index = cursor->index + 1;
        cursor->index = forward_index;
        if (forward_index >= cursor->count) {
            cursor->index = 0;
        }
    } else if (dataPadRead(1) & prev_mask) {
        seVoiceStartDefault(0x01000001);
        backward_index = cursor->index - 1;
        cursor->index = backward_index;
        if (backward_index < 0) {
            cursor->index = cursor->count - 1;
        }
    }
    if (target->applied != cursor->entry[cursor->index]) {
        func_00A89F08(target, cursor);
    }
    return cursor->index;
}

/* Sets the mode word (proposed dataStateSetMode): values below 2 are stored as given, otherwise the
 * flags choose 0 or 1. */
void dataStateSetMode(u32 value, int flags)
{
    struct DataState *state = &dataState;

    if (value < 2U) {
        state->mode = value;
        return;
    }
    if (!(flags & 4)) {
        if (flags & 1) {
            state->mode = 0U;
            return;
        }
        state->mode = 1U;
        return;
    }
    state->mode = 0U;
}

int func_00A8A150(void) {
    return D_00AB596C;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A160);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A1E0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A2E8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A3B0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A490);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A538);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A730);

INCLUDE_ASM("asm/nonmatchings/ov01/tu022", func_00A8A878);

void dataDummySet(void)
{
}
