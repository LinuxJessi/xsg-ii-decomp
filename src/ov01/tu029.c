/*
 * OV01 original TU 29: 0x00a90368..0x00a90660 (8 functions)
 */
#include "common.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu029", func_00A90368);

/* Sub-record reached through Tu029Object.sub; fields are role-named from the code that touches them. */
typedef struct Tu029Sub {
    unsigned char unmodeled_00[18];
    signed char bias;              /* 0x12 */
    unsigned char unmodeled_13[0x62 - 0x13];
    short metric;                  /* 0x62 */
    short level;                   /* 0x64 */
    short value;                   /* 0x66 */
    short saved_value;             /* 0x68 */
    unsigned char unmodeled_6a[0x73 - 0x6A];
    unsigned char dirty;           /* 0x73 */
} Tu029Sub;

typedef struct Tu029Object {
    unsigned char unmodeled_00[12];
    unsigned int flags;            /* 0x0C */
    unsigned char unmodeled_10[0x144 - 0x10];
    Tu029Sub *sub;                 /* 0x144 */
} Tu029Object;

typedef signed char s8;

typedef int s32;

void calcStatTurn();
void unitValueReset(Tu029Object *obj);

/* Clears the dirty mark and the 0x208 flag bits, then recomputes the unit value. */
void unitValueRefresh(Tu029Object *obj)
{
    obj->sub->dirty = 0;
    obj->flags &= ~0x208;
    unitValueReset(obj);
    calcStatTurn(obj);
}

s32 unitValueCalc(Tu029Object *obj);

/* Resets the level to 0 and stores the calculated value as value and saved_value. */
void unitValueReset(Tu029Object *obj)
{
    s32 derived;
    Tu029Sub *sub;

    derived = unitValueCalc(obj);
    sub = obj->sub;
    sub->level = 0;
    sub->value = derived;
    sub->saved_value = derived;
}

s32 calcStatGet();

/*
 * Level-based value: (level - bias) * 9 + 180, then the status tests of
 * calcStatGet for (group 1, mask 1), (group 3, mask 0x40) and (group 3,
 * mask 0x80) add or subtract shares of it.
 */
s32 unitValueCalc(Tu029Object *obj)
{
    Tu029Sub *sub;
    s32 levelBase;
    s32 derived;
    s32 adjust;
    s32 penalty;

    adjust = 0;
    sub = obj->sub;
    levelBase = (sub->level - sub->bias) * 9;
    derived = levelBase + 180;
    if (calcStatGet(obj, 1, 1) != 0) {
        adjust = derived / 2;
    }
    if (calcStatGet(obj, 3, 0x40) != 0) {
        penalty = derived;
        if (derived < 0) {
            penalty = levelBase + 183;
        }
        adjust -= penalty >> 2;
    }
    if (calcStatGet(obj, 3, 0x80) != 0) {
        adjust -= derived / 2;
    }
    return derived + adjust;
}

/*
 * For a unit that can resCmdAct (unitCanAct), lowers sub->value by 1, and by 1
 * more when ov01RollPercent(30) rolls; never below 0. Returns the new value.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu029", func_00A90500);

typedef struct Tu029Record {
    unsigned char unmodeled_00[11];
    signed char state;             /* 0x0B */
} Tu029Record;

s32 unitHasEffectCode(s32, s32);
Tu029Record *func_00A9B3B8(s32);

/*
 * Returns the signed state byte of record index, or 0 when unit has effect code 0x13.
 * The caller (func_00A92E30) adds the result to the unit's sub->level (+0x64).
 */
s8 unitRecordStateGet(s32 unit, s32 index)
{
    s8 state;

    state = func_00A9B3B8(index)->state;
    if (unitHasEffectCode(unit, 0x13) != 0) {
        state = 0;
    }
    return state;
}

/*
 * Returns 1 when exactly one of the two units has metric (+0x62) below 16, the same
 * split tgtSideOf reads; 0 when both or neither are below it. When it returns 1,
 * func_00A97890 walks all units (team 2) instead of one target.
 */
s32 unitSidesDiffer(Tu029Object *unitA, Tu029Object *unitB)
{
    if (unitA->sub->metric < 0x10) {
        if (unitB->sub->metric < 0x10) {
            return 0;
        }
        return 1;
    }
    return unitB->sub->metric < 0x10;
}

/*
 * Returns 1 for a unit whose flags (+0x0C) lack 0x4 and for which the status
 * tests of calcStatGet for (group 1, 0x20), (group 2, 0x20) and (group 2, 0x1)
 * are all clear; 0 otherwise, and for a null unit.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu029", unitCanAct);
