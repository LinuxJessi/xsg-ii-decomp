#include "common.h"
#include "main/tu116.h"
#include "main/tu148.h"

/* Script native seg.addrOn (table 0x3ec918): segment address number and a second argument; shows the message for number 1 or 2. */
INCLUDE_ASM("asm/main/nonmatchings/tu113", seg_0022D5A0);

#include "shared.h"

int segGetAddrOn(u32);
void func_A9CC70(int, int);
int func_A9F0A8(void);
int func_A9F0C0(int, int);
void questFlagSet(int, int);
int yg2PathGetCitizenFlag(u32);
int msgPurchaseUpdate(void);

/* Script native seg.getAddrOn (table 0x3ec920): argument 1 is the segment address number, 1-based. */
int seg_0022D6E8(int ctx)
{
    if (func_A9F0A8() != 1) {
        func_A9CC70(ctx, 2);
        return 0;
    }
    return segGetAddrOn(func_A9F0C0(ctx, 1) - 1);
}

/* Script native setQuestFlag (table 0x3ec8f0): flag number (1-based) and value; stored by questFlagSet. */
int setQuestFlag(int ctx)
{
    int flagNumber;
    int value;

    if (func_A9F0A8() != 2) {
        func_A9CC70(ctx, 2);
        return 0;
    }
    flagNumber = func_A9F0C0(ctx, 1);
    value = func_A9F0C0(ctx, 2);
    questFlagSet(flagNumber - 1, value);
    return 1;
}

/* Script native getQuestFlag (table 0x3ec8f8): flag number (1-based); the value comes from questFlagGet. */
int getQuestFlag(int ctx)
{
    if (func_A9F0A8() != 1) {
        func_A9CC70(ctx, 2);
        return 0;
    }
    return questFlagGet(func_A9F0C0(ctx, 1) - 1);
}

/* Script native setCitizenFlag (table 0x3ec900): citizen number (1-based) and value; stored by citizenFlagSet. */
int setCitizenFlag(int ctx)
{
    int citizenNumber;
    int value;

    if (func_A9F0A8() != 2) {
        func_A9CC70(ctx, 2);
        return 0;
    }
    citizenNumber = func_A9F0C0(ctx, 1);
    value = func_A9F0C0(ctx, 2);
    citizenFlagSet(citizenNumber - 1, value);
    return 1;
}

/* Script native getCitizenFlag (table 0x3ec908): citizen number (1-based); the value comes from yg2PathGetCitizenFlag. */
int getCitizenFlag(int ctx)
{
    if (func_A9F0A8() != 1) {
        func_A9CC70(ctx, 2);
        return 0;
    }
    return yg2PathGetCitizenFlag(func_A9F0C0(ctx, 1) - 1);
}

/* Script native menuAddItem (table 0x3ec8b8): a string argument is added to the menu. */
INCLUDE_ASM("asm/main/nonmatchings/tu113", menuAddItem);

/* Script native menuDeptQuest (table 0x3ec8c8): four arguments; up to 40 string entries are read from the script. */
INCLUDE_ASM("asm/main/nonmatchings/tu113", menuDeptQuest);

extern u16 textOptionFlags;
/* Word 2 (byte offset 8) of gameWork receives bit 0x8000 on the success path. */
extern u32 gameWork[];

/* Script native menuDeptGetResult (table 0x3ec8d0). */
int menuDeptGetResult(int unused_ctx, int *error_flag)
{
    int result = msgPurchaseUpdate();

    if (result < 0) {
        *error_flag = 1;
    }
    return result;
}

/* Script native menuGetResult (table 0x3ec8c0): on failure the argument count is reported through arg_count_out. */
int menuGetResult(int ctx, int *arg_count_out)
{
    int arg_count = func_A9F0A8();
    int result;

    if (arg_count != 1) {
        func_A9CC70(ctx, 2);
        return 0;
    }
    result = msgPollSelect();
    if (result < 0) {
        *arg_count_out = arg_count;
    }
    return result + 1;
}

/* Script native slotWinOpen (table 0x3ec8d8): two arguments, passed to msgChoiceOpen when the window is ready. */
int slotWinOpen(int ctx, int *error_flag)
{
    int first;
    int second;

    if (func_A9F0A8() != 2) {
        func_A9CC70(ctx, 2);
        return 0;
    }
    if (msgWindowBusy() == 0 && msgWindowHasMore() == 0 && (textOptionFlags & 0x20) != 0) {
        first = func_A9F0C0(ctx, 1);
        second = func_A9F0C0(ctx, 2);
        msgChoiceOpen(first, second);
        gameWork[2] |= 0x8000;
    } else {
        *error_flag = 1;
    }
    return 1;
}
