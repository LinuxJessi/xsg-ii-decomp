#include "common.h"
#include "shared.h"

/* Inventory categories: the first argument of dataBoxInc/dataBoxChk/dataBoxDec. */
#define ITEM_CATEGORY_ITEM 0

#define ITEM_CATEGORY_WEAPON 1
#define ITEM_CATEGORY_BULLET 2
#define ITEM_CATEGORY_ACCESSORY 3
#define ITEM_CATEGORY_EVENT 10 /* event items: names come from EvtItemTbl */

/* Each category keeps DATA_BOX_SLOTS quantities of at most DATA_BOX_QUANTITY_MAX. */
#define DATA_BOX_SLOTS 255

#define DATA_BOX_QUANTITY_MAX 99

extern int dataBoxInc(int category, int id);
extern int dataBoxChk(int category, int id);

int dataBoxDec(int category, int id);

extern u16 *dataBoxPtrGet(int category);
extern u16 accBox[DATA_BOX_SLOTS];
extern u16 bltBox[DATA_BOX_SLOTS];
extern u16 evtBox[DATA_BOX_SLOTS];
extern u16 itmBox[DATA_BOX_SLOTS];
extern u16 wpnBox[DATA_BOX_SLOTS];

u16 *dataBoxPtrGet(int category);

INCLUDE_ASM("asm/nonmatchings/ov01/tu043", func_00A9C610);

INCLUDE_ASM("asm/nonmatchings/ov01/tu043", dataBoxPtrGet);

int dataBoxChk(int category, int id)
{
    return dataBoxPtrGet(category)[id - 1];
}

int dataBoxInc(int category, int id)
{
    unsigned short *quantity;

    quantity = dataBoxPtrGet(category) + id - 1;
    if (*quantity < DATA_BOX_QUANTITY_MAX) {
        *quantity += 1;
        return 1;
    }
    return 0;
}

int dataBoxDec(int category, int id)
{
    unsigned short *quantity;

    quantity = dataBoxPtrGet(category) + id - 1;
    if (*quantity != 0) {
        *quantity -= 1;
        return 1;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu043", func_00A9C7A8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu043", func_00A9C7C8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu043", func_00A9C7E8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu043", func_00A9C808);

INCLUDE_ASM("asm/nonmatchings/ov01/tu043", func_00A9C828);

INCLUDE_ASM("asm/nonmatchings/ov01/tu043", func_00A9C848);

INCLUDE_ASM("asm/nonmatchings/ov01/tu043", func_00A9C868);

INCLUDE_ASM("asm/nonmatchings/ov01/tu043", func_00A9C888);

INCLUDE_ASM("asm/nonmatchings/ov01/tu043", func_00A9C8A8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu043", func_00A9C8C8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu043", func_00A9C8E8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu043", func_00A9C908);
