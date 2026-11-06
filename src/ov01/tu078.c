/*
 * OV01 original TU 78: 0x00aadb58..0x00aae0a8 (3 functions)
 */
#include "common.h"
#include "main/tu197.h"

/*
 * The block at +0x20 of the record MEfAdvanceStepShort updates; its step counter sits
 * at +0x54 of the block (+0x74 of the record).
 */
typedef struct MEfShortStepBlock {
    unsigned char unmodeled_00[0x54];
    unsigned short step;                               /* +0x54 */
    unsigned char unmodeled_56[0xb4 - 0x56];
} MEfShortStepBlock;

/*
 * The record MEfAdvanceStepShort updates: a 19-entry history at +0xd4 that the
 * function shifts one entry toward its end.
 */
typedef struct MEfShortStepObject {
    unsigned char unmodeled_00[0x20];
    MEfShortStepBlock block;                                /* +0x20 */
    unsigned short history[19];                        /* +0xd4 */
} MEfShortStepObject;

int ov01LastValueGet(void);

/*
 * Returns unless ov01LastValueGet reports 0. Otherwise shifts history[0..17] up
 * by one entry, advances the step counter, and calls MSeObjRequestFree once the
 * incremented step reaches 0x31.
 */
void MEfAdvanceStepShort(MEfShortStepObject *record)
{
  int index;
  short next_step;
  MEfShortStepBlock *block;

  block = &record->block;
  if (ov01LastValueGet() != 0) {
    return;
  }
  index = 18;
  do {
    record->history[index] = record->history[index - 1];
    index -= 1;
  } while (index > 0);
  next_step = (short) (block->step + 1);
  block->step = (unsigned short) (block->step + 1);
  if (next_step >= 0x31) {
    MSeObjRequestFree(record);
  }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu078", MEfCreateSway);

INCLUDE_ASM("asm/nonmatchings/ov01/tu078", MEfUpdateSway);
