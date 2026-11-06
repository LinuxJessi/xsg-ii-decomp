#include "common.h"
#include "main/elevator.h"

struct ElevatorObject {
    unsigned char unmodeled_00[0x760];
    int status;
};

struct ElevatorSlot {
    struct ElevatorObject *object;
    unsigned char unmodeled_04[0x86C];
};

struct ElevatorState {
    unsigned char unmodeled_00[0xD0];
    unsigned short flags;
};

struct ElevatorRecord {
    float position[3];
    unsigned char unmodeled_0C[0x24];
    float reference[3];
    unsigned char unmodeled_3C[0x0C];
    int state;
    unsigned char unmodeled_4C[0x04];
};

struct ElevatorDevice {
    unsigned char unmodeled_00[0x28];
    short first;
    short second;
};

struct ElevatorSample {
    float value[4];
};

extern int uwamonoCurrentIndex;
extern struct ElevatorSlot uwamonoSlotTable[];
extern struct ElevatorState gameWork;
extern struct ElevatorRecord elvRecords[];
extern struct ElevatorDevice *D_002B5DB8[];
extern unsigned char D_0069D1C0[];
extern unsigned char D_002BD478[];
extern float I2F(int value);
extern float elvValueScale;

int func_001EDCA0(void) {
    return 0x50;
}

int func_001EDCA8(void) {
    return 0x73;
}

/* Sets the 0x20 arrival flag when the current object reports status 0x2nn
 * with nn equal to this elevator's index (zero based). */
void elvSetStatusFlag(int elevator_index)
{
    int status;

    status = uwamonoSlotTable[uwamonoCurrentIndex].object->status;
    if ((status & ~0xFF) == 0x200 && (status & 0xF) == elevator_index) {
        gameWork.flags |= 0x20;
    }
}

/* Per-frame motion of one elevator: interpolates the record position toward
 * its target by elapsed/duration (VU0 lerp), and sets the arrival flag at the
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

/* Script native id.elvActive: sets or clears bit (elevator - 1) of
 * elvActiveMask from the second argument. Asserts elevator < 8. */
INCLUDE_ASM("asm/main/nonmatchings/elevator", elvSetActive);

/* Script native id.elvNoOut: the same bit operation on elvNoOutMask. */
INCLUDE_ASM("asm/main/nonmatchings/elevator", elvSetNoOut);

/* Script native id.elvMove: sets the target and motion of one record. Asserts
 * elevator < 8. */
INCLUDE_ASM("asm/main/nonmatchings/elevator", elvMove);

/* Script native id.slopeSet: stores four floats {-sin, -cos, height, sin} of
 * the angle (second argument times 0.000174533) in elvSlopeTable. */
INCLUDE_ASM("asm/main/nonmatchings/elevator", elvSlopeSet);

/* Script native id.keepoff: writes elvKeepOffFlags[no] from mapFindBlockEntry and the
 * low bit of the argument. Asserts no < 0x1F. */
INCLUDE_ASM("asm/main/nonmatchings/elevator", elvSetKeepOff);

int func_A9F0C0(int index, int selector);
int func_A9F1C0(int index, int selector);
void func_001E3630(int first, int second, float *values);
int func_A9CDB0();

int preren(int index)
{
    int first;
    int second;

    first = func_A9F0C0(index, 1);
    second = func_A9F0C0(index, 2);
    if (D_002B5DB8[0] != 0) {
        D_002B5DB8[0]->first = (short) first;
        D_002B5DB8[0]->second = (short) second;
    }
    return 1;
}

int preload(int index)
{
    struct ElevatorSample sample;
    int first;
    int second;

    if (func_A9F1C0(index, 1) == 0) {
        first = func_A9F0C0(index, 1);
    } else {
        first = -1;
    }
    if (func_A9F1C0(index, 2) == 0) {
        second = func_A9F0C0(index, 2);
    } else {
        second = -1;
    }
    sample.value[0] = elvValueToFloat(func_A9F0C0(index, 3));
    sample.value[1] = elvValueToFloat(func_A9F0C0(index, 4));
    sample.value[2] = elvValueToFloat(func_A9F0C0(index, 5));
    sample.value[3] = elvValueToFloat(func_A9F0C0(index, 6));
    func_001E3630(first, second, sample.value);
    return 1;
}

/* Resets the eight elevator records: runs the table setup, then clears the
 * word at +0x48 of every record (the move duration, see elvMove). */
void elvReset(void)
{
    struct ElevatorRecord *records;
    int *state;
    int i;

    func_A9CDB0(D_0069D1C0, D_002BD478);
    records = elvRecords;
    state = &records[7].state;
    for (i = 0; i < 8; i++) {
        *state = 0;
        state -= sizeof(struct ElevatorRecord) / sizeof(int);
    }
}
