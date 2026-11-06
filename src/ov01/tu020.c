/*
 * OV01 original TU 20: 0x00a88c60..0x00a88cb0 (4 functions)
 */
#include "common.h"
#include "ov01/tu020.h"

typedef struct MonsSlot MonsSlot;

typedef struct ActorView ActorView;

/*
 * Partial views: this TU reads only the pointer at +0x90 of a MonsSlot record
 * (the record monsSlotCreate creates, see tu019) and the flags word at +0xBC of
 * the actor it points to. No other byte is claimed. tu019 types the same word
 * as int; a shared header must pick one.
 */
struct MonsSlot {
    unsigned char unmodeled_00[0x90];
    ActorView *actor; /* +0x90 */
};

struct ActorView {
    unsigned char unmodeled_00[0xBC];
    unsigned int stateFlags; /* +0xBC: bit 0x4 is cleared by monsSlotShow and
                              * set by monsSlotHide; func_00A88AC0 sets it at
                              * creation and the fade-out end (tu018) sets it. */
};

/*
 * monsSlotShow (proposed monsSlotShow): clears bit 0x4 of the record's actor,
 * which makes it visible.
 */
void monsSlotShow(MonsSlot *slot) {
    slot->actor->stateFlags &= ~4;
}

/*
 * monsSlotHide (proposed monsSlotHide): sets bit 0x4 of the record's actor.
 */
void monsSlotHide(MonsSlot *slot) {
    slot->actor->stateFlags |= 4;
}

/*
 * monsTblValue (proposed monsTblValue): the value of the monster table entry
 * that monsTblSet selected (its +0x4 word, passed through monsTblValueSet).
 * monsTblValueGet returns it to ov01StepOpenHandles (tu038).
 */
extern int monsTblValue;

/* monsTblValueSet (proposed monsTblValueSet): stores the selected entry value. */
void monsTblValueSet(int value) {
    monsTblValue = value;
}

/* monsTblValueGet (proposed monsTblValueGet): returns the stored entry value. */
int monsTblValueGet(void) {
    return monsTblValue;
}
