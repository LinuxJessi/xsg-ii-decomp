#include "common.h"
#include "main/pad.h"
#include "main/tu147.h"
#include "main/tu149.h"

extern PadRecord PadData[];

void func_0027CC58(void *);
void func_0027CD50(void *, void *, void *, void *, void *);
extern unsigned char msgWindow[];
extern int msgItemNames[];
extern int msgItemCost[];
extern int msgShownValue;
extern int msgTotalValue;
extern int msgItemCount[];
extern int msgModeFlag;
extern int msgBalance;

/* Bind the window to its item tables and to the shown-value word. */
void msgWindowBindTables(void) {
    func_0027CC58(msgWindow);
    func_0027CD50(msgWindow, msgItemNames, msgItemCost, msgItemCount, &msgShownValue);
    msgGameFlagSet();
}

/* Set the mode word that msgPurchaseUpdate tests before its sound call. */
void msgSetMode(int mode) {
    msgModeFlag = mode;
}

/* Set the name of item `index`; the name is a string pointer passed as an int. */
void msgSetItemName(int index, int name) {
    msgItemNames[index] = name;
}

/* Set the cost of item `index`; msgPurchaseUpdate subtracts it from the total. */
void msgSetItemCost(int index, int value) {
    msgItemCost[index] = value;
}

/* Set the remaining count of item `index`. */
void msgSetItemCount(int index, int value) {
    msgItemCount[index] = value;
}

/* Start the shown value and the total at the value in msgBalance. */
void msgResetValues(void) {
    msgShownValue = msgBalance;
    msgTotalValue = msgBalance;
}

/* Per frame: the shown value rolls down by 100 toward the total. UP and DOWN
 * move the selection. CROSS (0x40) resets the window. CIRCLE (0x20) buys the
 * selected item when its count is above zero: the cost comes off the total
 * (floored at 0) and the count drops by one. */
extern unsigned short textOptionFlags;
int msgWindowTestCurrentFlags(void *window, int mask);
void msgWindowRunAll(void *window);
int msgWindowPollCurrent(void *window);
void msgWindowMoveSelection(void *window, int step);
void func_002512B0(void);
void func_A9C7E8(int item);
void func_A9C848(int item);

/* Returns the shown value once the purchase window closes (it also becomes msgBalance), else -1. */
int msgPurchaseUpdate(void)
{
    int item;
    int total;

    if (msgTotalValue < msgShownValue) {
        msgShownValue -= 100;
        if ((textOptionFlags & 0x20) || msgShownValue < msgTotalValue) {
            msgShownValue = msgTotalValue;
        }
        return -1;
    }
    if (msgWindowTestCurrentFlags(msgWindow, 0x14)) {
        return -1;
    }
    if (msgWindowTestCurrentFlags(msgWindow, 8) == 0 && msgWindowTestCurrentFlags(msgWindow, 2) != 0) {
        func_002512B0();
        msgBalance = msgShownValue;
        return msgShownValue;
    }
    if (msgShownValue == 0) {
        msgWindowRunAll(msgWindow);
        return -1;
    }
    if (PadData[0].pressed & PAD_CROSS) {
        msgWindowRunAll(msgWindow);
    } else if (PadData[0].pressed & PAD_CIRCLE) {
        item = msgWindowPollCurrent(msgWindow);
        if (msgItemCount[item] > 0) {
            msgItemCount[item]--;
            total = msgTotalValue - msgItemCost[item];
            if (total < 0) {
                total = 0;
            }
            msgTotalValue = total;
            if (msgModeFlag == 0) {
                func_A9C7E8(item + 1);
            } else {
                func_A9C848(item + 1);
            }
        }
    }
    if (PadData[0].directionRepeated & PAD_UP) {
        msgWindowMoveSelection(msgWindow, -1);
    }
    if (PadData[0].directionRepeated & PAD_DOWN) {
        msgWindowMoveSelection(msgWindow, 1);
    }
    return -1;
}
