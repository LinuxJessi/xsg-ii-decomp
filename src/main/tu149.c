#include "common.h"
#include "main/tu147.h"

void func_0027CC58(void *);
void func_0027CD50(void *, void *, void *, void *, void *);
extern unsigned char msgWindow[];
extern int msgItemNames[];
extern int msgItemCost[];
extern int D_008886E0;
extern int D_008886E4;
extern int msgItemCount[];
extern int msgModeFlag;
extern int msgBalance;

/* Bind the window to its item tables and to the shown-value word. */
void msgWindowBindTables(void) {
    func_0027CC58(msgWindow);
    func_0027CD50(msgWindow, msgItemNames, msgItemCost, msgItemCount, &D_008886E0);
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
    D_008886E0 = msgBalance;
    D_008886E4 = msgBalance;
}

/* Per frame: the shown value rolls down by 100 toward the total. UP and DOWN
 * move the selection. CROSS (0x40) resets the window. CIRCLE (0x20) buys the
 * selected item when its count is above zero: the cost comes off the total
 * (floored at 0) and the count drops by one. */
INCLUDE_ASM("asm/main/nonmatchings/tu149", msgPurchaseUpdate);
