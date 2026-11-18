#ifndef INCLUDE_OV01_TU002_H
#define INCLUDE_OV01_TU002_H

/* Functions of src/ov01/tu002.c that other files call. */

/*
 * One 0xA0-byte MonsSlot record of the two-entry monsSlotTable (tu002.c). The flags word uses OV01_SLOT_* (tu000.h): the claim and skip flags, the update and draw callbacks at +0x4 and +0x8, the
 * slot's actor at +0x90. monsSlotNew (tu019) stores the empty callback monsSlotEmptyUpdate in the update word and
 * clears the draw word, so each of those words has two views (a union only where the two files both touch it).
 */
typedef struct MonsSlot {
    int flags;
    union {
        int (*update)(struct MonsSlot *slot); /* +0x4: update sweep (monsSlotRunUpdate) */
        void (*callback)(void);               /* +0x4: monsSlotNew stores monsSlotEmptyUpdate (tu019) */
    };
    union {
        int (*draw)(struct MonsSlot *slot);   /* +0x8: draw sweep (monsSlotRunDraw) */
        unsigned int initWord08;              /* +0x8: monsSlotNew clears it (tu019) */
    };
    unsigned int initWord0C; /* +0xC: monsSlotNew clears it (tu019) */
    unsigned char unmodeled_10[0x80];
    struct XenoActor *actor; /* +0x90: the actor the record drives (tu019, tu020) */
    unsigned char unmodeled_94[0xC];
} MonsSlot;

void monsSlotRunDraw(void);
void monsSlotRunUpdate(void);
void monsSlotClearAll(void);
MonsSlot *monsSlotClaim(void);

#endif /* INCLUDE_OV01_TU002_H */
