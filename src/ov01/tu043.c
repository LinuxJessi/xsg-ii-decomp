#include "common.h"
#include "shared.h"
#include "ov01/tu043.h"
/* Category values as X1 names them. This overlay's dataBoxPtrGet answers only 1 to 4 (NULL for 0 and 10), so these names are not confirmed here. */
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
/* X1 box table names. This overlay's code reaches the tables through dataBoxPtrGet and uses none of these. */
extern u16 accBox[DATA_BOX_SLOTS];
extern u16 bltBox[DATA_BOX_SLOTS];
extern u16 evtBox[DATA_BOX_SLOTS];
extern u16 itmBox[DATA_BOX_SLOTS];
extern u16 wpnBox[DATA_BOX_SLOTS];
u16 *dataBoxPtrGet(int category);
extern void *memset(void *destination, int value, unsigned int count);
extern u16 D_61C800[DATA_BOX_SLOTS];
extern u16 D_61CA00[DATA_BOX_SLOTS];
extern u16 D_61CC00[DATA_BOX_SLOTS];
extern u16 D_61CE00[DATA_BOX_SLOTS];

/* Zeroes the four quantity tables. ov01InitAll (ov01 tu036) calls it among its setup calls. */
void dataBoxClearAll(void) {
    memset(D_61C800, 0, sizeof D_61C800);
    memset(D_61CA00, 0, sizeof D_61CA00);
    memset(D_61CC00, 0, sizeof D_61CC00);
    memset(D_61CE00, 0, sizeof D_61CE00);
}

/* Quantity table of a category: D_61C800 for 1, D_61CA00 for 2, D_61CC00 for 3, D_61CE00 for 4; NULL for any other value. */
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

/* Per-category wrappers for categories 1 to 4: Chk, Inc and Dec act on the quantity of one id.
 * The original tail-calls each one and returns its value. tgtRecordCanTarget (ov01 tu030) reads the value of dataBoxChkCategory1. */
int dataBoxChkCategory1(int id) {
    return dataBoxChk(1, id);
}

void dataBoxIncCategory1(int id) {
    dataBoxInc(1, id);
}

void dataBoxDecCategory1(int id) {
    dataBoxDec(1, id);
}

void dataBoxChkCategory2(int id) {
    dataBoxChk(2, id);
}

void dataBoxIncCategory2(int id) {
    dataBoxInc(2, id);
}

void dataBoxDecCategory2(int id) {
    dataBoxDec(2, id);
}

void dataBoxChkCategory3(int id) {
    dataBoxChk(3, id);
}

void dataBoxIncCategory3(int id) {
    dataBoxInc(3, id);
}

void dataBoxDecCategory3(int id) {
    dataBoxDec(3, id);
}

void dataBoxChkCategory4(int id) {
    dataBoxChk(4, id);
}

void dataBoxIncCategory4(int id) {
    dataBoxInc(4, id);
}

void dataBoxDecCategory4(int id) {
    dataBoxDec(4, id);
}
