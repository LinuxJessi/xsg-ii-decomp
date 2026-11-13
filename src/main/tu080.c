#include "common.h"
#include "main/tu110.h"

int func_A9F0A8(int id);
void func_A9CC70(int id, int code);
int func_A9F0C0(int id, int mode);
void ptAddMember(int handle);
void ptRemoveMember(int handle);
void ptLoadPartyModels(void);

/* Script native "ptJoin": one argument; its handle (argument 1) goes to ptAddMember, then the refresh calls. */
int ptJoin(int context)
{
    int handle;

    if (func_A9F0A8(context) != 1) {
        func_A9CC70(context, 2);
        return 0;
    }
    handle = func_A9F0C0(context, 1);
    ptAddMember(handle);
    ptLoadPartyModels();
    ptRefreshCurrent();
    return 1;
}

/* Script native "ptLeave": one argument; its handle (argument 1) goes to ptRemoveMember, then the refresh calls. */
int ptLeave(int context)
{
    int handle;

    if (func_A9F0A8(context) != 1) {
        func_A9CC70(context, 2);
        return 0;
    }
    handle = func_A9F0C0(context, 1);
    ptRemoveMember(handle);
    ptLoadPartyModels();
    ptRefreshCurrent();
    return 1;
}

/* Party split state: ptSplitMode[0] is PARTY_SPLIT_ON while the split is open (read by tu100, tu109, tu110). */
enum PartySplitState {
    PARTY_SPLIT_OFF = 0,
    PARTY_SPLIT_ON = 1
};

void ptEnterSplit(void);
void ptLeaveSplit(void);
extern unsigned char ptSplitMode[];
extern unsigned char D_0069B9B7;

int ptChangeMode(int context)
{
    int argCount;
    int requested;
    enum PartySplitState splitMode;

    argCount = func_A9F0A8(context);
    if (argCount != 1) {
        func_A9CC70(context, 2);
        return 0;
    }
    requested = func_A9F0C0(context, 1);
    switch (requested) {
    case PARTY_SPLIT_OFF:
        ptLeaveSplit();
        break;
    case PARTY_SPLIT_ON:
        ptEnterSplit();
        break;
    default:
        splitMode = ptSplitMode[0];
        switch (splitMode) {
        case PARTY_SPLIT_OFF:
            ptEnterSplit();
            break;
        case PARTY_SPLIT_ON:
            ptLeaveSplit();
            break;
        }
        break;
    }
    D_0069B9B7 = 1;
    ptLoadPartyModels();
    ptRefreshCurrent();
    return 1;
}

int ptSet(int context)
{
    int memberSlot;

    if (func_A9F0A8(context) <= 0 || func_A9F0A8(context) >= 8) {
        func_A9CC70(context, 2);
        return 0;
    }
    memberSlot = 0;
    goto test;
body:
    ptRemoveMember(memberSlot + 1);
    memberSlot++;
test:
    if (memberSlot < 7) {
        goto body;
    }
    memberSlot = 0;
    while (memberSlot < func_A9F0A8(context)) {
        ptAddMember(func_A9F0C0(context, memberSlot + 1));
        memberSlot++;
    }
    ptLoadPartyModels();
    ptRefreshCurrent();
    func_A9F0A8(context);
    return 1;
}
