#include "common.h"
#include "debug_break.h"
#include "main/elevator.h"

/* The part of the actor body this file reads. */
struct XenoActor {
    unsigned char unmodeled_000[0x760];
    int status;                             /* 0x760: ground status (tu051.h) */
};

/* One 0x870-byte slot of uwamonoSlotTable (tu085.c). */
struct UwamonoSlot {
    struct XenoActor *actor;                /* +0x00 */
    struct UwamonoState *state;             /* +0x04 */
    unsigned char unmodeled_08[0x870 - 0x08];
};

struct ElevatorState {
    unsigned char unmodeled_00[0xD0];
    unsigned short flags;                   /* +0xD0: the camera flags word (CameraState.flags in tu051.c) */
};

#define CAMERA_FLAG_TRACK_PLAYER 0x20 /* src/main/tu051.c */

/* One elevator: position moves from start to target over duration frames. */
struct ElevatorRecord {
    float position[3];
    unsigned char unmodeled_0C[0x10 - 0x0C];
    float start[3];                         /* +0x10 */
    unsigned char unmodeled_1C[0x20 - 0x1C];
    float target[3];                        /* +0x20 */
    unsigned char unmodeled_2C[0x30 - 0x2C];
    float reference[3];                     /* +0x30 */
    unsigned char axis_moves[3];            /* +0x3C, set when elvMove gave the axis */
    unsigned char unmodeled_3F[0x44 - 0x3F];
    float elapsed;                          /* +0x44 */
    float duration;                         /* +0x48 */
    float progress;                         /* +0x4C */
};

/* One entry of elvSlopeTable (UwamonoSlope in tu051.c). */
struct ElevatorSlope {
    float x;
    float z;
    float push;
    float speed;
};

/* The resource record gameWork.active points at (MainActive in tu050.c). */
struct MainActive {
    unsigned char unmodeled_00[0x28];
    short offsetY;                          /* +0x28 */
    short offsetX;                          /* +0x2A */
};

struct ElevatorSample {
    float value[4];
};

extern int uwamonoCurrentIndex;
extern struct UwamonoSlot uwamonoSlotTable[];
extern struct ElevatorState gameWork;
extern struct ElevatorRecord elvRecords[];
extern struct MainActive *D_002B5DB8[]; /* gameWork.active (gameWork+0x28) */
extern unsigned char D_0069D1C0[];
extern unsigned char D_002BD478[];
extern unsigned char elvActiveMask;
extern unsigned char elvNoOutMask;
extern float I2F(int value);
extern float elvValueScale;
extern float elvAngleScale;
extern int elvKeepOffFlags[];
extern struct ElevatorSlope elvSlopeTable[];
extern int printf(const char *format, ...);
extern float xglSin(float angle);
extern float xglCos(float angle);
int mapFindBlockEntry(int id, int index);
int func_A9F0C0(int call, int index);
int func_A9F1C0(int call, int index);

extern const char D_006894A8[]; /* the assert format: expression, file, line */
extern const char D_00689508[]; /* "(no > 0) && (no < 9)" */
extern const char D_00689520[]; /* "../elevator.euc.c" */
extern const char D_00689538[]; /* "(no > 0) && (no < UNDUFLAGKEEPOFF_MAX)" */

#define UNDUFLAGKEEPOFF_MAX 32

/* The original assert: prints the expression, file and line, then traps.
 * A bare if, not do { } while (0): its loop notes change the schedule. */
#define ELV_ASSERT(condition, expression, line)                     \
    if (!(condition)) {                                             \
        printf(D_006894A8, expression, D_00689520, line);           \
        DEBUG_BREAK();                                              \
    }

int func_001EDCA0(void) {
    return 0x50;
}

int func_001EDCA8(void) {
    return 0x73;
}

/* Sets CAMERA_FLAG_TRACK_PLAYER in the camera flags word when the current actor
 * reports status 0x2nn with nn equal to this elevator's index (zero based), that is
 * while the player stands on this elevator. */
void elvSetStatusFlag(int elevator_index)
{
    int status;

    status = uwamonoSlotTable[uwamonoCurrentIndex].actor->status;
    if ((status & ~0xFF) == 0x200 && (status & 0xF) == elevator_index) {
        gameWork.flags |= CAMERA_FLAG_TRACK_PLAYER;
    }
}

/* Per-frame motion of one elevator: interpolates the record position toward
 * its target by elapsed/duration (VU0 lerp), and calls elvSetStatusFlag at the
 * end. Called by DispMapObject with the object that holds the elevator index. */
INCLUDE_ASM("asm/main/nonmatchings/elevator", elvUpdateMotion);

/* Copies elevator position_out[] and, when asked, the offset of the position
 * from the reference point to delta_out[]. Either pointer may be null. */
void elvGetPosition(int elevator_index, float *position_out, float *delta_out)
{
    struct ElevatorRecord *record;

    record = &elvRecords[elevator_index];
    if (position_out != 0) {
        position_out[0] = record->position[0];
        position_out[1] = record->position[1];
        position_out[2] = record->position[2];
    }
    if (delta_out != 0) {
        delta_out[0] = record->position[0] - record->reference[0];
        delta_out[1] = record->position[1] - record->reference[1];
        delta_out[2] = record->position[2] - record->reference[2];
    }
}

/* Converts a script value to a float with the scale elvValueScale. */
float elvValueToFloat(int value)
{
    return I2F(value) * elvValueScale;
}

/* Script native id.elvActive: sets or clears bit (no - 1) of elvActiveMask
 * from the second argument. Asserts 0 < no < 9. The variable no is reused
 * for the bit value: one register serves both roles in the original. */
int elvSetActive(int call)
{
    int no;
    int active;
    int mask;

    no = func_A9F0C0(call, 1);
    active = func_A9F0C0(call, 2);
    ELV_ASSERT((no > 0) && (no < 9), D_00689508, 161);
    no = 1 << (no - 1);
    mask = elvActiveMask & ~no;
    if (active) {
        mask |= no;
    }
    elvActiveMask = mask;
    return 1;
}

/* Script native id.elvNoOut: the same bit operation on elvNoOutMask. */
int elvSetNoOut(int call)
{
    int no;
    int no_out;
    int mask;

    no = func_A9F0C0(call, 1);
    no_out = func_A9F0C0(call, 2);
    ELV_ASSERT((no > 0) && (no < 9), D_00689508, 186);
    no = 1 << (no - 1);
    mask = elvNoOutMask & ~no;
    if (no_out) {
        mask |= no;
    }
    elvNoOutMask = mask;
    return 1;
}

/* Script native id.elvMove: elevator no moves to the given x, y, z (an axis
 * left out is marked off in axis_moves and gets 0) over the given number of frames,
 * or jumps there at once when the frame count is not positive. */
int elvMove(int call)
{
    struct ElevatorRecord *record;
    int no;
    int frames;
    float x;
    float y;
    float z;

    no = func_A9F0C0(call, 1);
    ELV_ASSERT((no > 0) && (no < 9), D_00689508, 214);
    record = &elvRecords[no - 1];
    if (func_A9F1C0(call, 2) != 0) {
        x = 0.0f;
        record->axis_moves[0] = 0;
    } else {
        record->axis_moves[0] = 1;
        x = elvValueToFloat(func_A9F0C0(call, 2));
    }
    if (func_A9F1C0(call, 3) != 0) {
        y = 0.0f;
        record->axis_moves[1] = 0;
    } else {
        record->axis_moves[1] = 1;
        y = elvValueToFloat(func_A9F0C0(call, 3));
    }
    if (func_A9F1C0(call, 4) != 0) {
        z = 0.0f;
        record->axis_moves[2] = 0;
    } else {
        record->axis_moves[2] = 1;
        z = elvValueToFloat(func_A9F0C0(call, 4));
    }
    frames = func_A9F0C0(call, 5);
    if (frames > 0) {
        record->target[0] = x;
        record->target[1] = y;
        record->target[2] = z;
        record->elapsed = 0.0f;
        record->duration = frames;
        record->progress = 0.0f;
    } else {
        record->position[0] = x;
        record->position[1] = y;
        record->position[2] = z;
        record->duration = 0.0f;
        elvSetStatusFlag(no - 1);
    }
    return 1;
}

/* Script native id.slopeSet: elvSlopeTable[no - 1] = {-sin, -cos} of the
 * angle (second argument times elvAngleScale), then the third and fourth
 * arguments as push and speed. */
int elvSlopeSet(int call)
{
    struct ElevatorSlope *slope;
    int no;
    float angle;
    float push;
    float speed;

    no = func_A9F0C0(call, 1);
    ELV_ASSERT((no > 0) && (no < 9), D_00689508, 290);
    slope = &elvSlopeTable[no - 1];
    angle = I2F(func_A9F0C0(call, 2)) * elvAngleScale;
    push = elvValueToFloat(func_A9F0C0(call, 3));
    speed = elvValueToFloat(func_A9F0C0(call, 4));
    slope->x = -xglSin(angle);
    slope->z = -xglCos(angle);
    slope->push = push;
    slope->speed = speed;
    return 1;
}

/* Script native id.keepoff: elvKeepOffFlags[no] = 0 when the second argument
 * is zero, else mapFindBlockEntry(0xB00, no) with the argument's low bit. */
int elvSetKeepOff(int call)
{
    int no;
    int keep;

    no = func_A9F0C0(call, 1);
    keep = func_A9F0C0(call, 2);
    ELV_ASSERT((no > 0) && (no < UNDUFLAGKEEPOFF_MAX), D_00689538, 323);
    if (keep == 0) {
        elvKeepOffFlags[no] = 0;
    } else {
        elvKeepOffFlags[no] = mapFindBlockEntry(0xB00, no) | (keep & 1);
    }
    return 1;
}

void resJumpRecordSet(int index, int cfId, float *values);
/* ov01 sbNativeOwnerAdd: it returns 1, and elvReset's register choice needs the int result. */
int func_A9CDB0(void *module, void *nativeTable);

int preren(int call)
{
    int offsetY;
    int offsetX;

    offsetY = func_A9F0C0(call, 1);
    offsetX = func_A9F0C0(call, 2);
    if (D_002B5DB8[0] != 0) {
        D_002B5DB8[0]->offsetY = (short) offsetY;
        D_002B5DB8[0]->offsetX = (short) offsetX;
    }
    return 1;
}

int preload(int call)
{
    struct ElevatorSample sample;
    int index;
    int cfId;

    if (func_A9F1C0(call, 1) == 0) {
        index = func_A9F0C0(call, 1);
    } else {
        index = -1;
    }
    if (func_A9F1C0(call, 2) == 0) {
        cfId = func_A9F0C0(call, 2);
    } else {
        cfId = -1;
    }
    sample.value[0] = elvValueToFloat(func_A9F0C0(call, 3));
    sample.value[1] = elvValueToFloat(func_A9F0C0(call, 4));
    sample.value[2] = elvValueToFloat(func_A9F0C0(call, 5));
    sample.value[3] = elvValueToFloat(func_A9F0C0(call, 6));
    resJumpRecordSet(index, cfId, sample.value);
    return 1;
}

/* Registers the elevator script module "id" (elvActive, elvMove, elvNoOut, slopeSet,
 * keepoff, preren, preload), then clears the move duration of the eight elevator records. */
void elvReset(void)
{
    struct ElevatorRecord *records;
    float *duration;
    int i;

    func_A9CDB0(D_0069D1C0, D_002BD478);
    records = elvRecords;
    duration = &records[7].duration;
    for (i = 0; i < 8; i++) {
        *duration = 0.0f;
        duration -= sizeof(struct ElevatorRecord) / sizeof(float);
    }
}
