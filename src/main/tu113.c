#include "common.h"
#include "main/tu116.h"
#include "main/tu148.h"
#include "main/tu021.h"
#include "shared.h"

int segGetAddrOn(u32);
void func_A9CC70(int, int);
int func_A9F0A8(void);
int func_A9F0C0(int, int);
void questFlagSet(int, int);
int yg2PathGetCitizenFlag(u32);
int msgPurchaseUpdate(void);
int func_A9F140(int, int);
int func_A9F238(int, int);
void msgWindowOpenText(int);
extern u16 textOptionFlags;
extern u32 gameWork[];
void segSetAddrOn(int, int);
int uwamonoCurrentRecord(void);
void mapObjectBeginMessage(int);
void msgBegin(void);
void msgWindowSetScriptText(unsigned char *);
void msgWindowSetSlot(int, int);
int msgSlotCurrent(void);
void msgSlotSet(int, int);
extern unsigned char segAddrOnFoundText[];
extern unsigned char segAddrOnDoorText[];
extern unsigned char segAddrOnDecodedText[];

/* Message kinds of seg.addrOn, the native's second argument: the address was found, written on a door, or decoded. */
enum SegAddrMessage {
    SEG_ADDR_MSG_FOUND = 1,
    SEG_ADDR_MSG_ON_DOOR = 2,
    SEG_ADDR_MSG_DECODED = 3
};

/* Entries of the department/quest menu: names and costs are read per entry, and the item counts are set per entry. */
#define MENU_DEPT_ENTRIES 40

/* Script native seg.addrOn (table 0x3ec918): segment address number (1-based) and message kind; sets the address flag
   with segSetAddrOn, then shows the text for the kind (kinds 1 to 3). */
int segAddrOnMessage(int ctx)
{
    int arg_count = func_A9F0A8();
    int messageKind;

    if (arg_count != 2) {
        func_A9CC70(ctx, 2);
        return 0;
    }
    messageKind = func_A9F0C0(ctx, 2);
    segSetAddrOn(func_A9F0C0(ctx, 1) - 1, messageKind);
    mapObjectBeginMessage(uwamonoCurrentRecord());
    msgBegin();
    switch (messageKind) {
    case SEG_ADDR_MSG_FOUND:
        seVoiceStartDefault(0x100000a);
        msgWindowSetScriptText(segAddrOnFoundText);
        break;
    case SEG_ADDR_MSG_ON_DOOR:
        msgWindowSetScriptText(segAddrOnDoorText);
        break;
    case SEG_ADDR_MSG_DECODED:
        seVoiceStartDefault(0x200000c);
        msgWindowSetScriptText(segAddrOnDecodedText);
        break;
    }
    msgWindowSetSlot(1, func_A9F0C0(ctx, 1));
    msgSlotSet(msgSlotCurrent(), 0);
    return 1;
}

/* Script native seg.getAddrOn (table 0x3ec920): argument 1 is the segment address number, 1-based. */
int segAddrOnGetFlag(int ctx)
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
int menuAddItem(int ctx, int *error_flag)
{
    if (func_A9F0A8() != 2) {
        func_A9CC70(ctx, 2);
        return 0;
    }
    if ((func_A9F140(ctx, 2) & 0xff) != 3) {
        func_A9CC70(ctx, 2);
        return 0;
    }
    if (msgWindowBusy() == 0 && msgWindowHasMore() == 0 && (textOptionFlags & 0x20) != 0) {
        msgWindowOpenText(func_A9F238(ctx, func_A9F0C0(ctx, 2)));
        gameWork[2] |= 0x8000;
    } else {
        *error_flag = 1;
    }
    return 1;
}

void msgResetValues(int);
void msgSetMode(int);
void msgSetItemName(int, int);
void msgSetItemCost(int, int);
void msgSetItemCount(int, int);
void msgWindowBindTables(void);
int *func_A9F298(int handle, int value);
int func_A9C7A8(int);
int func_A9C808(int);

/* Script native menuDeptQuest (table 0x3ec8c8): four arguments; up to 40 string entries are read from the script. */
int menuDeptQuest(int ctx, int *error_flag)
{
    int *names;
    int *costs;
    int count;
    int i;

    count = 640;
    if (func_A9F0A8() != 4) {
        func_A9CC70(ctx, 2);
        return 0;
    }
    if (msgWindowBusy() != 0 || msgWindowHasMore() != 0) {
        *error_flag = 1;
    } else {
        names = func_A9F298(ctx, func_A9F0C0(ctx, 1));
        costs = func_A9F298(ctx, func_A9F0C0(ctx, 2));
        msgResetValues(func_A9F0C0(ctx, 3));
        for (i = 0; i < count / 16; i++) {
            msgSetItemName(i, func_A9F238(ctx, names[i]));
            msgSetItemCost(i, costs[i]);
        }
        if (func_A9F0C0(ctx, 4) == 0) {
            msgSetMode(0);
            for (i = 0; i < MENU_DEPT_ENTRIES; i++) {
                msgSetItemCount(i, func_A9C7A8(i + 1));
            }
        } else {
            msgSetMode(1);
            for (i = 0; i < MENU_DEPT_ENTRIES; i++) {
                msgSetItemCount(i, func_A9C808(i + 1));
            }
        }
        msgWindowBindTables();
        gameWork[2] |= 0x8000;
    }
    return 1;
}

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
    result = (int)msgPollSelect();
    if (result < 0) {
        *arg_count_out = arg_count;
    }
    return result + 1;
}

/* Script native slotWinOpen (table 0x3ec8d8): two arguments, passed to msgChoiceOpen when the window is ready. */
int slotWinOpen(int ctx, int *error_flag)
{
    int timelineFrame;
    int choiceIndex;

    if (func_A9F0A8() != 2) {
        func_A9CC70(ctx, 2);
        return 0;
    }
    if (msgWindowBusy() == 0 && msgWindowHasMore() == 0 && (textOptionFlags & 0x20) != 0) {
        timelineFrame = func_A9F0C0(ctx, 1);
        choiceIndex = func_A9F0C0(ctx, 2);
        msgChoiceOpen(timelineFrame, choiceIndex);
        gameWork[2] |= 0x8000;
    } else {
        *error_flag = 1;
    }
    return 1;
}
