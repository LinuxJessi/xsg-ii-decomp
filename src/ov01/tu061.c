/*
 * OV01 original TU 61: 0x00aa6bf8..0x00aa70e8 (6 functions)
 */
#include "common.h"
#include "ov01/tu061.h"

/*
 * menuTimedSeqNew (proposed menuTimedSeqNew): constructor of the timed sequence
 * panel. It allocates the panel with objSlotAlloc, sets its step menuTimedSeqStep
 * and its draw menuTimedPanelDraw (menuTimedPanelDraw), keeps it in menuTimedSeqPanel and
 * zeroes its state, owner, slot, value and tick fields. Called from the battle
 * setup ov01StepNextEnemy.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu061", menuTimedSeqNew);

typedef short s16;

typedef unsigned short u16;

typedef float f32;

/* The origin block at +0x10 of a timed sequence: the callees take its address; this body reads only y, the row. */
typedef struct SequenceOrigin {
    unsigned char unmodeled_00[4];
    f32 y;
} SequenceOrigin;

typedef struct TimedSequence {
    unsigned char unmodeled_000[0x10];
    SequenceOrigin origin;      /* +0x10: x and y of the panel; y is the row given by menuPanelRowAlloc */
    unsigned char unmodeled_018[0x78];
    int stage;                  /* +0x90: 1 while the panel slides in, else 0 */
    int owner;                  /* +0x94: the unit the sequence shows; the asm stores its pointer and the C tests it for zero */
    int slot;                   /* +0x98: index of the set bit of the unit's word at +0xF0 (menuTimedPanelDraw picks its title with it) */
    int value;                  /* +0x9C: the number menuTimedPanelDraw prints (the unit's word at +0xF4) */
    s16 tickEnabled;            /* +0xA0: set by menuTimedSeqArmTick, lets the ticks count */
    u16 tickCount;              /* +0xA2: counts ticks; 16 ends the sequence */
} TimedSequence;

int objStdMove(void *);
int slideSetStep(void *, int);
int menuPanelRowFree(int);

/*
 * menuTimedSeqStep (proposed menuTimedSeqStep): the step of the timed sequence.
 * While an owner is set, stage 1 slides the origin with slideSetStep until
 * objStdMove reports arrival. Otherwise, once the tick is enabled, each call
 * counts one tick; the sixteenth tick ends the sequence and frees its row.
 */
void menuTimedSeqStep(TimedSequence *sequence)
{
    int stage;
    void *origin;

    if (sequence->owner == 0) {
        return;
    }
    stage = sequence->stage;
    if (stage != 0) {
        if (stage == 1) {
            origin = &sequence->origin;
            slideSetStep(origin, 2);
            if (objStdMove(origin) != 0) {
                sequence->stage = 0;
            }
            return;
        }
    }
    if (sequence->tickEnabled == 1) {
        s16 ticks;

        ticks = sequence->tickCount + 1;
        sequence->tickCount = (u16)(sequence->tickCount + 1);
        if (ticks >= 0x10) {
            sequence->owner = 0;
            sequence->stage = 0;
            sequence->slot = 0;
            sequence->value = 0;
            menuPanelRowFree((int)sequence->origin.y);
        }
    }
}

/*
 * menuTimedPanelDraw (proposed menuTimedPanelDraw): draws a timed panel while it has
 * an owner: the title row from the table D_00AB6608 at the slot (+0x98), then
 * the number at +0x9C with menuNumDrawMode. The same draw is the one of the
 * menuTimedPanelNew objects of ov01/tu062.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu061", menuTimedPanelDraw);

/*
 * menuTimedSeqStart (proposed menuTimedSeqStart, argument unit): for a unit whose
 * word at +0xF4 is below 2, ends the sequence when the unit owns it (tick armed,
 * tick count 16) and does nothing otherwise. For a unit of 2 and up: when the
 * unit already owns the sequence only the value at +0x9C is set; otherwise the
 * sequence (or, when another unit owns it, a new menuTimedPanelNew panel of
 * ov01/tu062) takes the unit with state 1, the slot of the lowest set bit of the
 * unit's word at +0xF0, the value from +0xF4 and origin (-152, row).
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu061", menuTimedSeqStart);

extern TimedSequence *menuTimedSeqPanel;

/* Proposed menuTimedSeqArmTick: enables the tick count of the timed sequence. */
void menuTimedSeqArmTick(void) {
    menuTimedSeqPanel->tickEnabled = 1;
}

/*
 * func_00AA6FE0 (proposed menuTimedSeqEnd, low: the second branch is only partly
 * evidenced): for a unit below kind 2 that owns the sequence, arms the tick and
 * sets the tick count to 16 so the next tick ends it; otherwise it frees the
 * row and records the unit's value.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu061", func_00AA6FE0);
