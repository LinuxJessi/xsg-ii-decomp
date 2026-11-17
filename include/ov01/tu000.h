#ifndef INCLUDE_OV01_TU000_H
#define INCLUDE_OV01_TU000_H

/* Functions of src/ov01/tu000.c that other files call. */

/* Bits of a slot record's flags word (CallbackSlot, TaskRecord, MonsSlot, UnitSideSlot, DataObjectSlot, ObjSlot):
 * the claim bit of ov01SlotClaim, the update sweep skips a record with SKIP_UPDATE, the draw sweep with SKIP_DRAW. */
#define OV01_SLOT_CLAIM 0x1
#define OV01_SLOT_SKIP_UPDATE 0x2
#define OV01_SLOT_SKIP_DRAW 0x4

/* Slot helpers: the record is any slot-shaped struct (flags, update and draw at +0, +4, +8). */
int ov01SlotClaim(int *flags);
void ov01SlotClear(void *slot);
/* Unprototyped on purpose: monsSlotReleaseRecord (tu003) and unitRelease (tu004) call it with no argument. */
void ov01SlotRelease();

/* Claims a callback slot (0x60 bytes). Returned as void *: tu005 uses it as its SaveStack, whose top word
 * is at the update callback's offset, so one struct would need a union that no reader asks for. */
void *ov01CallbackSlotClaim(void);
void ov01CallbackSlotsClear(void);
void ov01CallbackSlotsRunUpdate(void);

#endif /* INCLUDE_OV01_TU000_H */
