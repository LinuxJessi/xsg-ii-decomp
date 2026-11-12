#include "common.h"
#include "main/tu022.h"
#include "main/tu051.h"
#include "main/tu110.h"
#include "main/tu121.h"
#include "main/tu148.h"

/* The 64-bit party values saveWriteImage copies. Their loads stay after the
 * int store before them, so the access goes through a union (alias set 0). */
typedef union PartyValue64 {
    long long value;
    int words[2];
} PartyValue64;

typedef struct PartyName {
    char text[0x20];
} PartyName;

/* The record the first word of ptState points at. tu092 adds the elapsed
 * time to playTime; tu100 compares key with -1. */
typedef struct PartyProgress {
    unsigned short id;
    unsigned int playTime;
    int key;
    PartyValue64 savedValues[2];
    unsigned char savedByte;
    /* Set to 1 by saveWriteImage when gameWork's map id is MAP_ID_SPLIT_FLAG_MAP. */
    unsigned char mapFlag;
    PartyName name;
} PartyProgress;

/* The record behind a member's status pointer: saveWriteImage saves value + 1 per slot. */
typedef struct PartyMemberStatus {
    unsigned char unmodeled_00[8];
    unsigned char value;
    unsigned char unmodeled_09[0xC - 0x09];
} PartyMemberStatus;

/* A member record: word 0 is the member id. */
typedef struct PartyMember {
    int id;
    PartyMemberStatus *status;
    unsigned char unmodeled_08[0x10 - 0x08];
} PartyMember;

/* Party state block ptState:
 *   member slots 0-9 (slots 0-6 front, 7-9 back) and a second roster of the
 *   same kind (swapped by func_0022A7D8, searched by func_0022ADA0);
 *   splitMode is set to 1 by ptEnterSplit and cleared by ptLeaveSplit;
 *   frontIndex is the current front slot, backIndex the current back slot - 7;
 *   counts are the member counts. tu092, tu100 and tu109 read the same object
 *   with other types. */
typedef struct PartyState {
    PartyProgress *progress;
    PartyMember *slots[10];
    PartyMember *roster[10];
    unsigned char splitMode;
    unsigned char frontIndex;
    unsigned char backIndex;
    unsigned char counts[3];
    /* The member the menu shows (tu100 func_002171A0 stores it): a front
     * slot index, or in split mode an index over back slots 7-9 followed by
     * the second roster. */
    unsigned char menuSlot;
} PartyState;

extern PartyState ptState;

/* The member the menu shows: front slot menuSlot, or in split mode back
 * slot 7 + menuSlot for 0-2 and roster entry menuSlot - 3 after them. */
PartyMember *func_0022A560(void)
{
    PartyMember *member;

    if (ptState.splitMode == 1) {
        if (ptState.menuSlot < 3) {
            member = ptState.slots[ptState.menuSlot + 7];
        } else {
            member = ptState.roster[ptState.menuSlot - 3];
        }
    } else {
        member = ptState.slots[ptState.menuSlot];
    }
    return member;
}

#include "shared.h"

/* The member data record func_A8B490 returns; saveWriteImage saves value for the leader. */
typedef struct PartyMemberInfo {
    unsigned char unmodeled_00[0x13];
    unsigned char value;
} PartyMemberInfo;

PartyMemberInfo *func_A8B490(int id);

void func_0022A5A0(void) {
    func_A8B490(func_0022A560()->id);
}

/* Partial view of one 0x870-byte record of D_003400F8: only the leading
 * float is read by these functions. */
typedef struct ScaledEntry {
    float value;
    unsigned char unmodeled_04[0x86C];
} ScaledEntry;

extern ScaledEntry D_003400F8[];
extern int uwamonoCurrentIndex;
extern unsigned char ptSplitMode[];

/* Defined without a prototype: ptSelectMember calls it with no arguments and the asm passes its own a0 through. */
int ptFindSlot(id) int id;
{
    PartyMember *record;
    int found;
    int i;

    found = -1;
    for (i = 0; i < 10; i++) {
        record = ptState.slots[i];
        if (record != 0 && record->id == id) {
            found = i;
            break;
        }
    }
    return found;
}

PartyMember *ptFindRecord(int id) {
    PartyMember *record;
    PartyMember *found;
    int misses;
    PartyMember **slot;
    PartyState *party;

    found = 0;
    misses = 0;
    party = &ptState;
    slot = party->slots;
    for (;;) {
        record = *slot;
        slot++;
        if (record != 0 && record->id == id) {
            found = record;
            break;
        }
        misses++;
        if (misses >= 10) {
            break;
        }
    }
    return found;
}

void func_0022A7D8(int a, int b);

/* Swaps slots a and b (both 0-9 and occupied). frontIndex follows its member
 * when both are below 3, backIndex when both are 7 or 8; two back slots also
 * swap roster entries a - 7 / b - 7 and a - 4 / b - 4. */
void ptSwapSlots(int a, int b)
{
    PartyMember *member;

    if (a >= 10) {
        return;
    }
    if (b >= 10) {
        return;
    }
    if (a < 0) {
        return;
    }
    if (b < 0) {
        return;
    }
    if (ptState.slots[a] == 0) {
        return;
    }
    if (ptState.slots[b] == 0) {
        return;
    }
    if (a < 3 && b < 3) {
        if (ptState.frontIndex == a) {
            ptState.frontIndex = b;
        } else if (ptState.frontIndex == b) {
            ptState.frontIndex = a;
        }
    }
    member = ptState.slots[a];
    ptState.slots[a] = ptState.slots[b];
    ptState.slots[b] = member;
    if (a >= 7 && b >= 7) {
        if (a < 9 && b < 9) {
            if (ptState.backIndex + 7 == a) {
                ptState.backIndex = b - 7;
            } else if (ptState.backIndex + 7 == b) {
                ptState.backIndex = a - 7;
            }
        }
        func_0022A7D8(a - 7, b - 7);
        func_0022A7D8(a - 4, b - 4);
    }
}

/* Swaps the second roster's slots a and b (bounds-checked). */
void func_0022A7D8(int a, int b)
{
    PartyMember *member;

    if (a < 10) {
        if (b < 10) {
            if (a >= 0) {
                if (b >= 0) {
                    member = ptState.roster[a];
                    ptState.roster[a] = ptState.roster[b];
                    ptState.roster[b] = member;
                }
            }
        }
    }
}

void ptRefresh(void);
void ptEnterSplit(void);
void ptLeaveSplit(void);
extern PartyMember ptMemberRecords[];
/* The record ptState.progress points at. */
extern PartyProgress D_006A5A30;

/* Puts member id (unless a slot already holds it) into the first free slot
 * of its group: ids 11 and up go to the back slots 7-9, the others to the
 * front slots 0-6. Its bit in D_006A5A30.id is set and the counts kept up.
 * Defined without a prototype: ptPlaceMember calls it with no arguments and
 * the asm passes its own a0 through. */
void ptAddMember(id) int id;
{
    PartyMember *record;
    int split;
    int slot;
    int i;

    if (ptFindRecord(id) != 0) {
        return;
    }
    record = 0;
    for (i = 0; i < 10; i++) {
        if (ptMemberRecords[i].id == id) {
            record = &ptMemberRecords[i];
            break;
        }
    }
    split = ptState.splitMode;
    if (split == 1) {
        ptLeaveSplit();
    }
    if (record != 0) {
        slot = -1;
        if (id >= 11) {
            for (i = 7; i < 10; i++) {
                if (ptState.slots[i] == 0) {
                    ptState.counts[2]++;
                    D_006A5A30.id |= 1 << (id - 4);
                    slot = i;
                    break;
                }
            }
        } else {
            for (i = 0; i < 7; i++) {
                if (ptState.slots[i] == 0) {
                    ptState.counts[1]++;
                    D_006A5A30.id |= 1 << (id - 1);
                    slot = i;
                    break;
                }
            }
        }
        if (slot != -1) {
            ptState.slots[slot] = record;
            ptState.counts[0]++;
        }
        ptRefresh();
    }
    if (split == 1) {
        ptEnterSplit();
    }
}

/* Removes member id from its slot, closing the gap within its group (front
 * slots 0-6 or back slots 7-9) and keeping frontIndex/backIndex on the same
 * members. Split mode is left for the move and restored afterwards. */
void ptRemoveMember(int id)
{
    int split;
    int i;
    int k;

    if (ptFindRecord(id) == 0) {
        return;
    }
    split = ptState.splitMode;
    if (split == 1) {
        ptLeaveSplit();
    }
    for (i = 0; i < 10; i++) {
        if (ptState.slots[i] != 0 && ptState.slots[i]->id == id) {
            /* The original compares the slot index unsigned (sltiu). */
            if ((unsigned int)i < 7) {
                if (ptState.frontIndex == i) {
                    ptState.frontIndex = 0;
                }
                for (k = i; k < 6; k++) {
                    ptState.slots[k] = ptState.slots[k + 1];
                    if (ptState.frontIndex == k + 1) {
                        ptState.frontIndex = k;
                    }
                }
            } else {
                if (ptState.backIndex + 7 == i) {
                    ptState.backIndex = 0;
                }
                for (k = i; k < 9; k++) {
                    ptState.slots[k] = ptState.slots[k + 1];
                    /* backIndex counts from slot 7: slot k + 1 is backIndex k - 6. */
                    if (ptState.backIndex == k - 6) {
                        ptState.backIndex = k - 7;
                    }
                }
            }
            ptState.slots[k] = 0;
            ptState.counts[0]--;
            if (id >= 11) {
                ptState.counts[2]--;
            } else {
                ptState.counts[1]--;
            }
            ptRefresh();
            break;
        }
    }
    if (split == 1) {
        ptEnterSplit();
    }
}

void ptResetSlots(void)
{
    int i;

    for (i = 9; i >= 0; i--) {
        ptState.slots[i] = 0;
    }
    ptState.counts[2] = 0;
    ptState.counts[1] = 0;
    ptState.counts[0] = 0;
    ptState.frontIndex = 0;
    ptState.backIndex = 0;
    ptRefresh();
}

void ptSetCurrentSlot(int slot)
{
    if (ptState.slots[slot] == 0) {
        return;
    }
    if (slot >= 7) {
        ptState.backIndex = slot - 7;
        return;
    }
    ptState.frontIndex = slot;
}

int ptFindSlot();

void ptSelectMember(void) {
    int slot;

    slot = ptFindSlot();
    if (slot != -1) {
        ptSetCurrentSlot(slot);
    }
}

int ptGetCurrentSlot(void)
{
    if (ptState.splitMode == 1) {
        return ptState.backIndex + 7;
    }
    return ptState.frontIndex;
}

int ptGetCurrentId(void)
{
    if (ptState.splitMode == 1) {
        return ptState.slots[ptState.backIndex + 7]->id;
    }
    return ptState.slots[ptState.frontIndex]->id;
}

/* Index 0-6 of the front slot holding the member that roster entry secondIndex
 * names, or -1 when no front slot holds it. */
int ptFindFrontSlot(int secondIndex) {
    PartyMember **frontSlot;
    PartyMember *secondRecord;
    int frontIndex;
    int found;

    found = -1;
    secondRecord = ptState.roster[secondIndex];
    frontSlot = ptState.slots;
    frontIndex = 0;
    for (;;) {
        if (secondRecord == 0) {
            frontIndex++;
        } else if (*frontSlot == secondRecord) {
            found = frontIndex;
            break;
        } else {
            frontIndex++;
        }
        frontSlot++;
        if (frontIndex >= 7) {
            break;
        }
    }
    return found;
}

/* Index of the second roster slot holding id, or -1 when none does. */
int func_0022ADA0(int id)
{
    PartyMember *record;
    int found;
    int i;

    found = -1;
    for (i = 0; i < 10; i++) {
        record = ptState.roster[i];
        if (record != 0 && record->id == id) {
            found = i;
            break;
        }
    }
    return found;
}

#define STORY_PROGRESS_FLAG 0x191

extern int D_003EC468[];
extern int D_003EC4A0[];
extern int D_003EC458[];
extern int D_003EC474[];
extern int D_003EC490[];
int xglFlagsGet8(int bit_offset);

/* The model (cloth) id of member id: func_0022AF98 passes it to
 * uwamonoChangePlayerModel, func_0022BAF8 to resPathSlotsLoadModels, and
 * ptRefresh keeps it in ptStatusBlock's clothIds. Ids 11 and up read a flat
 * table; the others compare the story-progress flag with a per-member
 * threshold, member 1 with an extra threshold/table pair. */
int func_0022AE10(int id)
{
    int progress;

    if (id >= 11) {
        return D_003EC468[id];
    }
    progress = xglFlagsGet8(STORY_PROGRESS_FLAG);
    id--;
    if (id != 0) {
        if (progress >= D_003EC4A0[id]) {
            return D_003EC458[id];
        }
        return D_003EC474[id];
    }
    if (progress >= D_003EC4A0[7]) {
        if (progress >= D_003EC4A0[0]) {
            return D_003EC458[0];
        }
        return D_003EC490[0];
    }
    return D_003EC474[0];
}

int uwamonoLoadCurrent();
int uwamonoStoreCurrent();
void func_0022AF98(void);

void ptRefreshCurrent(void) {
    int flag;

    flag = 0;
    if (uwamonoCurrentIndex != -1) {
        if (!(D_003400F8[uwamonoCurrentIndex].value < 5.0f)) {
            flag = 1;
        }
        flag = flag == ptSplitMode[0];
    }
    if (flag != 0) {
        uwamonoLoadCurrent();
    }
    func_0022AF98();
    if (flag != 0) {
        uwamonoStoreCurrent();
    }
}

void uwamonoChangePlayerModel(int request);

/* Passes the current member's func_0022AE10 value to uwamonoChangePlayerModel;
 * in split mode (the back member) with bit 31 set. */
void func_0022AF98(void)
{
    PartyMember *record;
    int request;

    if (ptState.splitMode == 1) {
        record = ptState.slots[ptState.backIndex + 7];
        if (record != 0) {
            request = func_0022AE10(record->id);
            uwamonoChangePlayerModel(request | 0x80000000);
        }
        return;
    }
    record = ptState.slots[ptState.frontIndex];
    if (record != 0) {
        request = func_0022AE10(record->id);
        uwamonoChangePlayerModel(request);
        return;
    }
}

void ptAddMember();
void func_0022BAF8(void);

void ptPlaceMember(int id) {
    int slot;

    /* ids 11 and up go to the back slot 7 in split mode; the others to front slot 0. */
    ptAddMember();
    if (id >= 0xB) {
        ptEnterSplit();
        slot = ptFindSlot(id);
        ptSwapSlots(7, slot);
        ptSetCurrentSlot(7);
    } else {
        ptLeaveSplit();
        slot = ptFindSlot(id);
        ptSwapSlots(0, slot);
        ptSetCurrentSlot(0);
    }
    func_0022BAF8();
    ptRefreshCurrent();
}

/* Copies each front member (slots 0-6) that the roster does not hold yet
 * into the next free roster entry 3-5 whose back slot (entry + 4) is filled;
 * returns when no such entry is left. */
void func_0022B0B8(void)
{
    int i;
    int entry;

    entry = 3;
    for (i = 0; i < 7; i++) {
        if (ptState.slots[i] != 0 && func_0022ADA0(ptState.slots[i]->id) == -1) {
            while (entry < 6) {
                if (ptState.roster[entry] != 0 || ptState.slots[entry + 4] == 0) {
                    entry++;
                    continue;
                }
                break;
            }
            if (entry < 6) {
                ptState.roster[entry] = ptState.slots[i];
            } else {
                return;
            }
        }
    }
}

/* Copies every front member that the roster does not hold yet into the free
 * roster entries 6-9. */
void func_0022B1C8(void)
{
    int i;
    int free;

    free = 6;
    for (i = 0; i < 7; i++) {
        if (ptState.slots[i] != 0 && func_0022ADA0(ptState.slots[i]->id) == -1) {
            for (;;) {
                if (free < 10 && ptState.roster[free] != 0) {
                    free++;
                } else {
                    break;
                }
            }
            if (free >= 10) {
                break;
            }
            ptState.roster[free] = ptState.slots[i];
        }
    }
}

/* {2, 6, 7}: the front member id paired with each of the back ids 11-13
 * (three bytes of tu108's .sdata). */
extern unsigned char D_0069D768[];

/* Enters split mode when the party has back members. Roster entries 0-2 take
 * the D_0069D768 counterpart of the member in back slots 7-9. Entries 3-9
 * drop a member that left the party or that entries 0-2 now hold; entries
 * 3-5 are also dropped when their back slot (entry + 4) is empty. Then
 * func_0022B0B8 and func_0022B1C8 fill the free entries. */
void ptEnterSplit(void)
{
    int i;
    int id;
    PartyMember *member;

    if (ptState.splitMode == 1) {
        return;
    }
    if (ptState.counts[2] != 0) {
        for (i = 0; i < 3; i++) {
            member = ptState.slots[i + 7];
            if (member != 0) {
                /* The table entry of id - 11. The original adds the id to
                 * the address D_0069D768 - 11 on every pass; with
                 * D_0069D768[id - 11] the compiler loads the table address
                 * once before the loop. */
                id = member->id;
                ptState.roster[i] = ptFindRecord((D_0069D768 - 11)[id]);
            } else {
                ptState.roster[i] = 0;
            }
        }
        for (i = 3; i < 6; i++) {
            if (ptState.roster[i] != 0) {
                if (ptFindRecord(ptState.roster[i]->id) == 0) {
                    ptState.roster[i] = 0;
                } else if (func_0022ADA0(ptState.roster[i]->id) < 3) {
                    ptState.roster[i] = 0;
                } else if (ptState.slots[i + 4] == 0) {
                    ptState.roster[i] = 0;
                }
            }
        }
        for (i = 6; i < 10; i++) {
            if (ptState.roster[i] != 0) {
                if (ptFindRecord(ptState.roster[i]->id) == 0) {
                    ptState.roster[i] = 0;
                } else if (func_0022ADA0(ptState.roster[i]->id) < 3) {
                    ptState.roster[i] = 0;
                }
            }
        }
        func_0022B0B8();
        func_0022B1C8();
        ptState.splitMode = 1;
        ptRefresh();
    }
}

void ptLeaveSplit(void)
{
    if (ptState.splitMode == 0) {
        return;
    }
    if (ptState.counts[1] != 0) {
        ptState.splitMode = 0;
        ptRefresh();
    }
}

void func_0022B1C8(void);

void func_0022B458(void)
{
    int i;
    PartyState *party;
    PartyMember **slots;

    /* Roster entries 3-5. */
    party = &ptState;
    slots = &party->roster[3];
    for (i = 0; i < 3; i++) {
        *slots = 0;
        slots++;
    }
    func_0022B1C8();
}

/* The id in roster entry slot - 7 (0-2) for a back member, else 0. */
int func_0022B498(int id)
{
    int slot;
    PartyMember *member;

    if (id < 11) {
        return 0;
    }
    slot = ptFindSlot(id);
    if (slot < 7) {
        return 0;
    }
    member = ptState.roster[slot - 7];
    if (member != 0) {
        return member->id;
    }
    return 0;
}

/* The id in roster entry slot - 4 (3-5) for a back member, else 0: the
 * member tu101's special-attack list treats as its copilot. */
int func_0022B4E8(int id)
{
    int slot;
    PartyMember *member;

    if (id < 11) {
        return 0;
    }
    slot = ptFindSlot(id);
    if (slot < 7) {
        return 0;
    }
    member = ptState.roster[slot - 4];
    if (member != 0) {
        return member->id;
    }
    return 0;
}

/* 1 when every occupied back slot 7-9 has its roster entry 3-5 filled. */
int func_0022B538(void)
{
    int result;
    int i;

    result = 1;
    for (i = 0; i < 3; i++) {
        if (ptState.slots[i + 7] == 0) {
            continue;
        }
        if (ptState.roster[i + 3] != 0) {
            continue;
        }
        result = 0;
        break;
    }
    return result;
}

int ptGroupCount(void)
{
    int count;

    if (ptState.splitMode == 1) {
        count = ptState.counts[2];
        if (count >= 2) {
            count = 2;
        }
    } else {
        count = ptState.counts[1];
        if (count >= 3) {
            count = 3;
        }
    }
    return count;
}

/* ptStatusBlock (0x24 bytes) as func_0022B5E0 resets it and ptRefresh fills
 * it: the ids of the three members on screen and of the leader, a copy of
 * D_006A5A30.id, one bit per party member present (ov01's tu072 edits it as
 * the MEMBER row) and the func_0022AE10 cloth id of each of the seven members
 * (ov01's tu072 edits them as cloth ids). */
typedef struct PartyStatusBlock {
    unsigned char memberIds[3];
    unsigned char leaderId;
    unsigned short progressId;
    unsigned short member;
    int clothIds[7];
} PartyStatusBlock;

void *memset(void *dest, int value, unsigned int size);
extern u8 ptStatusBlock[];

void func_0022B5E0(void)
{
    int *clothIds;
    int i;

    memset(ptStatusBlock, 0, sizeof(PartyStatusBlock));
    clothIds = ((PartyStatusBlock *)ptStatusBlock)->clothIds;
    for (i = 0; i < 7; i++) {
        clothIds[i] = D_003EC458[i];
    }
}

extern PartyMemberStatus D_006A5A80[10];
int xglFlagsGet2(int bit_offset);
/* OV01 battleSlotCodeSet (config/symbols/ov01.txt). */
void func_A8B7E0(int handle, unsigned char value);
/* OV01 battleSlotSetShortLate (config/symbols/ov01.txt). */
void func_A8BA08(int id, int kind, int otherId);

/* Rebuilds ptStatusBlock from the party and pairs the members for the
 * current split mode. */
void ptRefresh(void)
{
    PartyStatusBlock *block;
    PartyMember *leader;
    int progress;
    int i;

    block = (PartyStatusBlock *)ptStatusBlock;
    if (ptState.splitMode == 1) {
        leader = ptState.slots[ptState.backIndex + 7];
        if (leader != 0) {
            block->leaderId = leader->id;
        } else {
            block->leaderId = 0;
        }
        block->memberIds[2] = 0;
        for (i = 0; i < 2; i++) {
            if (ptState.slots[i + 7] != 0) {
                block->memberIds[i] = ptState.slots[i + 7]->id;
            } else {
                block->memberIds[i] = 0;
            }
        }
        block->member = 0;
        for (i = 0; i < 3; i++) {
            if (ptState.slots[i + 7] != 0) {
                block->member |= 1 << (ptState.slots[i + 7]->id - 4);
            }
        }
        for (i = 0; i < 3; i++) {
            if (ptState.slots[i + 7] != 0) {
                if (ptState.roster[i] != 0) {
                    func_A8BA08(ptState.slots[i + 7]->id, 0, ptState.roster[i]->id);
                } else {
                    func_A8BA08(ptState.slots[i + 7]->id, 0, 0);
                }
                if (ptState.roster[i + 3] != 0) {
                    func_A8BA08(ptState.slots[i + 7]->id, 1, ptState.roster[i + 3]->id);
                } else {
                    func_A8BA08(ptState.slots[i + 7]->id, 1, 0);
                }
            }
        }
    } else {
        leader = ptState.slots[ptState.frontIndex];
        if (leader != 0) {
            block->leaderId = leader->id;
        } else {
            block->leaderId = 0;
        }
        for (i = 0; i < 3; i++) {
            if (ptState.slots[i] != 0) {
                block->memberIds[i] = ptState.slots[i]->id;
            } else {
                block->memberIds[i] = 0;
            }
        }
        block->member = 0;
        for (i = 0; i < 7; i++) {
            if (ptState.slots[i] != 0) {
                block->member |= 1 << (ptState.slots[i]->id - 1);
            }
        }
    }
    block->progressId = D_006A5A30.id;
    for (i = 0; i < 7; i++) {
        block->clothIds[i] = func_0022AE10(i + 1);
    }
    progress = xglFlagsGet8(STORY_PROGRESS_FLAG);
    for (i = 9; i >= 0; i--) {
        D_006A5A80[i].value = i;
    }
    /* From story progress 0x2B entries 2 and 5 take positions 11 and 12,
     * from 0x57 entry 1 position 10, after the others. */
    i = 10;
    if (progress >= 0x57) {
        D_006A5A80[1].value = i;
    }
    if (progress >= 0x2B) {
        D_006A5A80[2].value = 11;
        D_006A5A80[5].value = 12;
    }
    if (xglFlagsGet8(0x1EE) > 0) {
        /* The original leaves through func_A8B7E0 in cases 1 and 2 (a jump)
         * and calls it in case 3. */
        switch (xglFlagsGet2(0x1D6)) {
        case 1:
            func_A8B7E0(3, 0x37);
            return;
        case 2:
            func_A8B7E0(3, 0x38);
            return;
        case 3:
            func_A8B7E0(3, 0x39);
            break;
        }
    }
}

/* One row of the table func_0022BA18 searches: the four two-bit flag values
 * it is chosen for; a row whose first value is 0 ends the table. */
typedef struct FlagCombination {
    unsigned char values[4];
} FlagCombination;

extern FlagCombination D_003EC4C0[];

/* 1-based index of the last row of D_003EC4C0 that matches the flags
 * 0x1DE, 0x1E1, 0x1D6 and 0x1DB, or 0 when none does. */
int func_0022BA18(void)
{
    int flags[4];
    int found;
    int i;

    found = 0;
    flags[0] = xglFlagsGet2(0x1DE);
    flags[1] = xglFlagsGet2(0x1E1);
    flags[2] = xglFlagsGet2(0x1D6);
    flags[3] = xglFlagsGet2(0x1DB);
    for (i = 0; D_003EC4C0[i].values[0] != 0; i++) {
        if (D_003EC4C0[i].values[0] == flags[0] && D_003EC4C0[i].values[1] == flags[1]
            && D_003EC4C0[i].values[2] == flags[2] && D_003EC4C0[i].values[3] == flags[3]) {
            found = i + 1;
        }
    }
    return found;
}

void resPathSlotsLoadModels(int modelId0, int modelId1, int modelId2);

/* Loads the models of the three party members on screen: back slots 7-9 in
 * split mode, front slots 0-2 otherwise. */
void func_0022BAF8(void)
{
    int modelId0;
    int modelId1;
    int modelId2;

    modelId0 = 0;
    modelId1 = 0;
    modelId2 = 0;
    ptRefresh();
    if (ptState.splitMode == 1) {
        if (ptState.slots[7] != 0) {
            modelId0 = func_0022AE10(ptState.slots[7]->id);
        }
        if (ptState.slots[8] != 0) {
            modelId1 = func_0022AE10(ptState.slots[8]->id);
        }
        if (ptState.slots[9] != 0) {
            modelId2 = func_0022AE10(ptState.slots[9]->id);
        }
    } else {
        if (ptState.slots[0] != 0) {
            modelId0 = func_0022AE10(ptState.slots[0]->id);
        }
        if (ptState.slots[1] != 0) {
            modelId1 = func_0022AE10(ptState.slots[1]->id);
        }
        if (ptState.slots[2] != 0) {
            modelId2 = func_0022AE10(ptState.slots[2]->id);
        }
    }
    resPathSlotsLoadModels(modelId0, modelId1, modelId2);
}

/* The clock reading xglClockRead fills. */
typedef struct DayTime {
    unsigned char status;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned short year;
} DayTime;

/* The blocks saveWriteImage copies whole into the save image. Their types set
 * the copy alignment the original shows: the int blocks and the short tables
 * take the runtime alignment test, the byte blocks unaligned moves. */
typedef struct SaveFlagBlock1080 {
    int words[0x420];
} SaveFlagBlock1080;

typedef struct SaveFlagBlock400 {
    int words[0x100];
} SaveFlagBlock400;

typedef struct SaveShortTable {
    short values[0xFF];
} SaveShortTable;

typedef struct SaveWords4 {
    int words[4];
} SaveWords4;

typedef struct SaveGameWork {
    unsigned char unmodeled_00[8];
    int control;
    unsigned char unmodeled_0C[0x14 - 0x0C];
    int state;
    int mapId;
    unsigned char unmodeled_1C[4];
    long long unmodeled_20[0x362];
} SaveGameWork;

typedef struct QuestFlagBlock {
    unsigned char bytes[0x24];
} QuestFlagBlock;

typedef struct CitizenFlagBlock {
    unsigned char bytes[0x32];
} CitizenFlagBlock;

typedef struct SegFlagBlock {
    unsigned char bytes[0x12];
} SegFlagBlock;

typedef struct SaveMemberName {
    unsigned char bytes[0x2B];
} SaveMemberName;

/* The party as the save image keeps it: first id bytes of every slot and
 * roster entry, the progress record's fields and the leader. */
typedef struct SaveParty {
    unsigned char slotIds[10];
    unsigned char rosterIds[10];
    unsigned short progressId;
    unsigned char unmodeled_16[2];
    int key;
    unsigned char unmodeled_1C[4];
    PartyValue64 savedValues[2];
    unsigned char savedByte;
    unsigned char splitMode;
    unsigned char frontIndex;
    unsigned char backIndex;
    unsigned char counts[3];
    SaveMemberName names[10];
    unsigned char unmodeled_1E5;
    unsigned char leaderId;
    unsigned char leaderValue;
    unsigned short slotValues[7];
    unsigned char mapFlag;
    PartyName name;
} SaveParty;

/* The save image at SaveData (0x5160 bytes, the size saveChecksum sums). */
typedef struct SaveImage {
    unsigned char unmodeled_00[8];
    long long checksum;
    int saveCount;
    unsigned char unmodeled_14[0x50 - 0x14];
    long long version;
    unsigned char unmodeled_58[8];
    DayTime saveTime;
    DayTime playTime;
    unsigned char unmodeled_70[0x1174 - 0x70];
    SaveFlagBlock1080 flagBlock1080;
    unsigned char unmodeled_21F4[0x2274 - 0x21F4];
    SaveFlagBlock400 flagBlock400;
    SaveShortTable shortTables[4];
    int word2E6C;
    SaveWords4 words2E70;
    SaveGameWork game;
    unsigned char unmodeled_49B0[0x4E80 - 0x49B0];
    QuestFlagBlock questFlags;
    CitizenFlagBlock citizenFlags;
    SegFlagBlock segFlags;
    unsigned char unmodeled_4EE8[8];
    SaveParty party;
    unsigned char unmodeled_5108[0x5150 - 0x5108];
    int balance;
    unsigned char unmodeled_5154[0x5160 - 0x5154];
} SaveImage;

extern SaveImage SaveData;
extern SaveFlagBlock1080 D_0061B380;
extern SaveFlagBlock400 D_0061C400;
extern SaveShortTable D_0061C800;
extern SaveShortTable D_0061CA00;
extern SaveShortTable D_0061CC00;
extern SaveShortTable D_0061CE00;
extern int D_0069DC7C;
extern SaveWords4 D_0061D000;
extern SaveGameWork gameWork;
extern SaveMemberName D_006A5880[10];
extern QuestFlagBlock questFlags;
extern CitizenFlagBlock citizenFlags;
extern SegFlagBlock segAddrOnFlags;

/* The image checksum: every byte of a copy of the image (its checksum field
 * zeroed) weighted by its 1-based position, plus 0x793 per byte. */
void saveChecksum(SaveImage *image, long long *sum)
{
    SaveImage copy;
    unsigned char *byte;
    int i;

    copy = *image;
    copy.checksum = 0;
    *sum = 0;
    byte = (unsigned char *)&copy;
    for (i = 0; i < sizeof(SaveImage); i++) {
        *sum = *sum + *byte++ * (long long)(i + 1) + 0x793;
    }
}

void xglClockRead(DayTime *time);
unsigned int xglClockDayTime2UInt(DayTime *time);
void xglClockUInt2DayTime(DayTime *time, unsigned int seconds);
int msgBalanceGet(void);
void saveChecksum(SaveImage *image, long long *sum);

/* Fills the save image: play time, the global blocks, the party and the
 * counters, then the checksum. The blocks are copied through their address
 * (the original addresses them as SaveData+offset constants, not from the
 * SaveData base). */
#define MAP_ID_SPLIT_FLAG_MAP 0x1B91 /* entering it sets the progress record's mapFlag */
#define SAVE_VERSION 200
#define PARTY_FRONT_SLOTS 7
#define PARTY_BACK_SLOTS 3

void saveWriteImage(void)
{
    DayTime now;
    long long sum;
    PartyMemberInfo *leaderInfo;
    SaveParty *party;
    int i;

    ptRefresh();
    xglClockRead(&now);
    xglClockUInt2DayTime(&SaveData.playTime, xglClockDayTime2UInt(&now) - ptState.progress->playTime);
    *(SaveFlagBlock1080 *)(u8 *)&SaveData.flagBlock1080 = D_0061B380;
    *(SaveFlagBlock400 *) (unsigned char *) &SaveData.flagBlock400 = D_0061C400;
    *(SaveShortTable *) (unsigned char *) &SaveData.shortTables[0] = D_0061C800;
    *(SaveShortTable *) (unsigned char *) &SaveData.shortTables[1] = D_0061CA00;
    *(SaveShortTable *) (unsigned char *) &SaveData.shortTables[2] = D_0061CC00;
    *(SaveShortTable *) (unsigned char *) &SaveData.shortTables[3] = D_0061CE00;
    *(SaveWords4 *) (unsigned char *) &SaveData.words2E70 = D_0061D000;
    SaveData.word2E6C = D_0069DC7C;
    party = &SaveData.party;
    if (gameWork.mapId == MAP_ID_SPLIT_FLAG_MAP) {
        ptState.progress->mapFlag = 1;
    }
    party->mapFlag = ptState.progress->mapFlag;
    party->progressId = ptState.progress->id;
    party->splitMode = ptState.splitMode;
    party->frontIndex = ptState.frontIndex;
    party->backIndex = ptState.backIndex;
    party->counts[0] = ptState.counts[0];
    party->counts[1] = ptState.counts[1];
    party->counts[2] = ptState.counts[2];
    party->key = ptState.progress->key;
    party->savedValues[0] = ptState.progress->savedValues[0];
    party->savedValues[1] = ptState.progress->savedValues[1];
    party->savedByte = ptState.progress->savedByte;
    party->name = ptState.progress->name;
    for (i = 0; i < 10; i++) {
        if (ptState.slots[i] != 0) {
            party->slotIds[i] = ptState.slots[i]->id;
        } else {
            party->slotIds[i] = 0;
        }
        if (ptState.roster[i] != 0) {
            party->rosterIds[i] = ptState.roster[i]->id;
        } else {
            party->rosterIds[i] = 0;
        }
        party->names[i] = D_006A5880[i];
    }
    if (ptState.splitMode == 0) {
        PartyMember *leader = ptState.slots[ptState.frontIndex];

        if (leader != 0) {
            party->leaderId = leader->id;
            leaderInfo = func_A8B490(leader->id);
            party->leaderValue = leaderInfo->value;
        } else {
            party->leaderId = 0;
            party->leaderValue = 0;
        }
        for (i = 0; i < PARTY_FRONT_SLOTS; i++) {
            if (ptState.slots[i] != 0) {
                party->slotValues[i] = ptState.slots[i]->status->value + 1;
            } else {
                party->slotValues[i] = 0;
            }
        }
    } else {
        PartyMember *leader = ptState.slots[ptState.backIndex + PARTY_FRONT_SLOTS];

        if (leader != 0) {
            party->leaderId = leader->id;
            leaderInfo = func_A8B490(leader->id);
            party->leaderValue = leaderInfo->value;
        } else {
            party->leaderId = 0;
            party->leaderValue = 0;
        }
        for (i = 0; i < PARTY_BACK_SLOTS; i++) {
            if (ptState.slots[i + PARTY_FRONT_SLOTS] != 0) {
                party->slotValues[i] = ptState.slots[i + PARTY_FRONT_SLOTS]->status->value + 1;
            } else {
                party->slotValues[i] = 0;
            }
        }
        for (; i < PARTY_FRONT_SLOTS; i++) {
            party->slotValues[i] = 0;
        }
    }
    *(SaveGameWork *) (unsigned char *) &SaveData.game = gameWork;
    *(QuestFlagBlock *) (unsigned char *) &SaveData.questFlags = questFlags;
    *(CitizenFlagBlock *) (unsigned char *) &SaveData.citizenFlags = citizenFlags;
    *(SegFlagBlock *) (unsigned char *) &SaveData.segFlags = segAddrOnFlags;
    SaveData.balance = msgBalanceGet();
    SaveData.saveCount++;
    SaveData.version = SAVE_VERSION;
    xglClockRead(&SaveData.saveTime);
    saveChecksum(&SaveData, &sum);
    SaveData.checksum = sum;
}

void cursorBufferTablesInit(void);
PartyMember *ptMemberRecordFindById(int id);

/* Restores the play time base, the global blocks and the party from the save
 * image: the reverse of saveWriteImage. */
void saveReadImage(void)
{
    DayTime now;
    unsigned int playTime;
    SaveParty *party;
    int i;

    cursorBufferTablesInit();
    xglClockRead(&now);
    playTime = xglClockDayTime2UInt(&SaveData.playTime);
    ptState.progress->playTime = xglClockDayTime2UInt(&now) - playTime;
    /* Each block is copied through its address, as in saveWriteImage. */
    D_0061B380 = *(SaveFlagBlock1080 *) (unsigned char *) &SaveData.flagBlock1080;
    D_0061C400 = *(SaveFlagBlock400 *) (unsigned char *) &SaveData.flagBlock400;
    D_0061C800 = *(SaveShortTable *) (unsigned char *) &SaveData.shortTables[0];
    D_0061CA00 = *(SaveShortTable *) (unsigned char *) &SaveData.shortTables[1];
    D_0061CC00 = *(SaveShortTable *) (unsigned char *) &SaveData.shortTables[2];
    D_0061CE00 = *(SaveShortTable *) (unsigned char *) &SaveData.shortTables[3];
    D_0061D000 = *(SaveWords4 *) (unsigned char *) &SaveData.words2E70;
    D_0069DC7C = SaveData.word2E6C;
    party = &SaveData.party;
    ptState.progress->mapFlag = party->mapFlag;
    ptState.progress->id = party->progressId;
    ptState.splitMode = party->splitMode;
    ptState.frontIndex = party->frontIndex;
    ptState.backIndex = party->backIndex;
    ptState.counts[0] = party->counts[0];
    ptState.counts[1] = party->counts[1];
    ptState.counts[2] = party->counts[2];
    ptState.progress->key = party->key;
    ptState.progress->savedValues[0] = party->savedValues[0];
    ptState.progress->savedValues[1] = party->savedValues[1];
    ptState.progress->savedByte = party->savedByte;
    ptState.progress->name = party->name;
    for (i = 0; i < 10; i++) {
        if (party->slotIds[i] != 0) {
            ptState.slots[i] = ptMemberRecordFindById(party->slotIds[i]);
        } else {
            ptState.slots[i] = 0;
        }
        if (party->rosterIds[i] != 0) {
            ptState.roster[i] = ptMemberRecordFindById(party->rosterIds[i]);
        } else {
            ptState.roster[i] = 0;
        }
        D_006A5880[i] = party->names[i];
    }
    ptRefresh();
}

int xglFlagsSet8(int bit_offset, int value);
void msgBalanceSet(int value);

/* SaveImage from questFlags (+0x4E80) to balance (+0x5150), the same storage
 * under a second type: func_0022CEE0 addresses the three flag blocks and the
 * balance from &SaveData.questFlags. The bytes between segFlags and balance
 * are SaveImage's party block and its neighbours, which it does not read. */
typedef struct SaveFlagsTail {
    QuestFlagBlock questFlags;
    CitizenFlagBlock citizenFlags;
    SegFlagBlock segFlags;
    unsigned char unmodeled_68[0x2D0 - 0x68];
    int balance;
} SaveFlagsTail;

#define MAP_ID_SPLIT_FLAG_RESUME 0x9EC /* the map a save made on MAP_ID_SPLIT_FLAG_MAP loads into */

/* Restores gameWork, the quest, citizen and segment flags and the balance
 * from the save image. A save made on MAP_ID_SPLIT_FLAG_MAP resumes on
 * MAP_ID_SPLIT_FLAG_RESUME with story progress 0x75. CONTROL_SOFT_RESET and
 * the control bits 27, 26 and 20 do not survive a load (tu069's runtimeSystem
 * sets 27 and toggles 20). The flag blocks are copied byte by byte; the
 * segment statement comes before the citizen one, and the compiler emits the
 * citizen moves first, as the original has them (struct assignments in either
 * order give other code). */
void func_0022CEE0(void)
{
    SaveFlagsTail *tail;
    int control;
    int balance;

    gameWork = *(SaveGameWork *) (unsigned char *) &SaveData.game;
    gameWork.state = gameWork.mapId;
    if (gameWork.mapId == MAP_ID_SPLIT_FLAG_MAP) {
        gameWork.mapId = gameWork.state = MAP_ID_SPLIT_FLAG_RESUME;
        xglFlagsSet8(STORY_PROGRESS_FLAG, 0x75);
    }
    tail = (SaveFlagsTail *) &SaveData.questFlags;
    control = gameWork.control;
    balance = tail->balance;
    __builtin_memcpy(questFlags.bytes, tail->questFlags.bytes, sizeof(questFlags.bytes));
    __builtin_memcpy(segAddrOnFlags.bytes, tail->segFlags.bytes, sizeof(segAddrOnFlags.bytes));
    __builtin_memcpy(citizenFlags.bytes, tail->citizenFlags.bytes, sizeof(citizenFlags.bytes));
    gameWork.control = control & ~(CONTROL_SOFT_RESET | 0x08000000 | 0x04000000 | 0x00100000);
    msgBalanceSet(balance);
}

void tyaSetDisplayOffset(s16, s16);
extern u8 PadData[];

/* Save-record fields read by padApplySaveSettings. Unmodeled spans keep the layout. */
typedef struct SaveRecordFields {
    u8 unmodeled_00[0x28];
    u8 flag;
    u8 unmodeled_29[0x32 - 0x29];
    s16 pairFirst;
    s16 pairSecond;
    u8 unmodeled_36[0x3E - 0x36];
    u8 copyByte;
} SaveRecordFields;

/* Pad-record fields written by padApplySaveSettings. */
typedef struct PadRecordFields {
    u8 unmodeled_00[0x56];
    u8 copiedValue;
    u8 unmodeled_57[0xD6 - 0x57];
    u8 copiedValueAlt;
} PadRecordFields;

void padApplySaveSettings(void)
{
    PadRecordFields *pad;
    SaveRecordFields *save;

    pad = (PadRecordFields *) PadData;
    save = (SaveRecordFields *) &SaveData;
    pad->copiedValue = pad->copiedValueAlt = save->copyByte;
    func_001A4400(save->flag != 0);
    tyaSetDisplayOffset(save->pairFirst, save->pairSecond);
}

extern u8 menuMainTxdBuffer[];

void *menuMainTxdEntry(int index) {
    return (void *)txdTableEntry(menuMainTxdBuffer, index);
}

extern u8 menuTutorialTxdBuffer[];

void *menuTutorialTxdEntry(int index) {
    return (void *)txdTableEntry(menuTutorialTxdBuffer, index);
}

PartyState *ptGetState(void) {
    return &ptState;
}

extern int D_003EC60C[];

int ptGetMember(int index) {
    return D_003EC60C[index];
}

u8 *ptGetStatusBlock(void) {
    return ptStatusBlock;
}

extern int talkResult;
extern int D_002B5D98[];

int talk_0022D170(void) {
    int previous;

    previous = talkResult;
    talkResult = -1;
    if (msgWindowBusy() != 0) {
        return -1;
    }
    return (D_002B5D98[0] & 0x20000) ? -1 : previous;
}

int talk_0022D1C8(void) {
    talkResult = -1;
    return 1;
}
