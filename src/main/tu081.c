#include "common.h"
#include "main/tu108.h"

typedef struct PtMember {
    int id;
} PtMember;

/* Party split state, same values as tu080.c: PARTY_SPLIT_ON while the split is open. */
enum PartySplitState {
    PARTY_SPLIT_OFF = 0,
    PARTY_SPLIT_ON = 1
};

typedef struct PtPartyView {
    int unmodeled_00;
    PtMember *slots[10];
    PtMember *roster[10];
    unsigned char splitMode;
    unsigned char frontIndex;
    unsigned char backIndex;
    unsigned char counts[3];
} PtPartyView;

extern PtPartyView ptState;
int func_A9CC70(int value, int mode);
int func_A9F0A8(void);
int func_A9F0C0(int value, int mode);
int ptFindSlot(int value);
void ptRefresh(void);
void func_0022A7D8(int slot, int index);

int ptSetPilot(int context)
{
    int pilotId;
    int rosterId;
    int slot;
    int rosterIndex;
    int candidate;

    if (func_A9F0A8() != 2) {
        func_A9CC70(context, 2);
        return 0;
    }
    if (ptState.splitMode != PARTY_SPLIT_ON) {
        return 0;
    }
    pilotId = func_A9F0C0(context, 1);
    rosterId = func_A9F0C0(context, 2);
    slot = ptFindSlot(pilotId);
    if (slot == -1) {
        return 0;
    }
    slot -= 4;
    rosterIndex = -1;
    for (candidate = 3; candidate < 10; candidate++) {
        if (ptState.roster[candidate] != 0 && ptState.roster[candidate]->id == rosterId) {
            rosterIndex = candidate;
            break;
        }
    }
    if (rosterIndex == -1) {
        return 0;
    }
    func_0022A7D8(slot, rosterIndex);
    ptRefresh();
    return 1;
}

void battleSlotsRefresh(void);

int ptGetLeader(int context)
{
    PtMember *member;
    int id;

    if (func_A9F0A8() != 1) {
        func_A9CC70(context, 2);
        return 0;
    }
    if (func_A9F0C0(context, 1) != 0) {
        PtPartyView *backParty = &ptState;
        PtMember *backMember = backParty->slots[backParty->backIndex + 7];
        id = 0;
        if (backMember != 0) {
            id = backMember->id;
        }
        return id;
    }
    member = ptState.slots[ptState.frontIndex];
    if (member == 0) {
        id = 0;
    } else {
        id = member->id;
    }
    return id;
}

/* Script native "umnReturn": takes no arguments; calls uwamonoDetachCurrent and returns 1. */
int umnReturn(int context)
{
    if (func_A9F0A8() != 0) {
        func_A9CC70(context, 2);
        return 0;
    }
    uwamonoDetachCurrent();
    return 1;
}

void ptSwapSlots(int a, int b);
void ptRefreshCurrent(void);
void func_0022BAF8(void);

int ptSetArray(int context)
{
    int memberId;
    int destSlot;
    int srcSlot;

    if (func_A9F0A8() != 2) {
        func_A9CC70(context, 2);
        return 0;
    }
    memberId = func_A9F0C0(context, 1);
    destSlot = func_A9F0C0(context, 2);
    srcSlot = ptFindSlot(memberId);
    if (srcSlot == -1) {
        return 0;
    }
    if (srcSlot >= 7) {
        return 0;
    }
    if (destSlot < ptState.counts[0]) {
        if (destSlot >= 0) {
            if (destSlot == srcSlot) {
                return 1;
            }
            ptSwapSlots(destSlot, srcSlot);
            ptRefresh();
            func_0022BAF8();
            ptRefreshCurrent();
            return 1;
        }
    }
    return 0;
}

int ptTent(int context)
{
    if (func_A9F0A8() != 0) {
        func_A9CC70(context, 2);
        return 0;
    }
    battleSlotsRefresh();
    return 1;
}
