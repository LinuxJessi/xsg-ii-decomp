#include "common.h"
#include "main/pad.h"
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

/* The record the first word of ptState points at. memberMask has one bit per
 * member who joined the party (ptAddMember sets bit id - 1 for ids below 11,
 * id - 4 for the others). tu092 adds the elapsed time to playTime; tu100
 * compares key with -1. */
typedef struct PartyProgress {
    unsigned short memberMask;
    unsigned int playTime;
    int key;
    PartyValue64 savedValues[2];
    unsigned char savedByte;
    /* Set to 1 by saveWriteImage when gameWork's map id is MAP_ID_SPLIT_FLAG_MAP. */
    unsigned char mapFlag;
    PartyName name;
} PartyProgress;

/* The status record behind a member's status pointer (D_006A5A80): two texts of
 * MainMenu.txd and the member index, which saveWriteImage saves + 1 per slot. */
typedef struct PartyMemberStatus {
    char *text;                 /* +0x00: the txd entry of the member index */
    char *textAlt;              /* +0x04: the txd entry of the member index + 17 */
    unsigned char memberIndex;  /* +0x08: ptReset stores the member index */
    unsigned char unmodeled_09[3];
} PartyMemberStatus;

/* A member record: word 0 is the member id. */
typedef struct PartyMember {
    int id;
    PartyMemberStatus *status;
    unsigned char unmodeled_08[0x10 - 0x08];
} PartyMember;

#define PARTY_FRONT_SLOTS 7
#define PARTY_BACK_SLOTS 3
#define PARTY_SLOTS (PARTY_FRONT_SLOTS + PARTY_BACK_SLOTS)
/* Member ids from 11 on are back members: they take the back slots 7-9. */
#define BACK_MEMBER_ID 11

/* Party state block ptState:
 *   member slots 0-9 (slots 0-6 front, 7-9 back) and a second roster of the
 *   same kind (swapped by ptSwapRosterEntries, searched by ptFindRosterEntry);
 *   splitMode is set to 1 by ptEnterSplit and cleared by ptLeaveSplit;
 *   frontIndex is the current front slot, backIndex the current back slot - PARTY_FRONT_SLOTS;
 *   counts are the member counts. tu092, tu100 and tu109 read the same object
 *   with other types. */
typedef struct PartyState {
    PartyProgress *progress;
    PartyMember *slots[PARTY_SLOTS];
    PartyMember *roster[PARTY_SLOTS];
    unsigned char splitMode;
    unsigned char frontIndex;
    unsigned char backIndex;
    unsigned char counts[3];
    /* The member the menu shows (tu100 menuLayoutPlaceStep stores it): a front
     * slot index, or in split mode an index over back slots 7-9 followed by
     * the second roster. */
    unsigned char menuSlot;
} PartyState;

extern PartyState ptState;

/* The member the menu shows: front slot menuSlot, or in split mode back
 * slot 7 + menuSlot for 0-2 and roster entry menuSlot - 3 after them. */
PartyMember *ptGetMenuMember(void)
{
    PartyMember *member;

    if (ptState.splitMode == 1) {
        if (ptState.menuSlot < 3) {
            member = ptState.slots[ptState.menuSlot + PARTY_FRONT_SLOTS];
        } else {
            member = ptState.roster[ptState.menuSlot - 3];
        }
    } else {
        member = ptState.slots[ptState.menuSlot];
    }
    return member;
}

#include "shared.h"
#include "main/tu100.h"

/* Admitted aligned(1) scalar: maxHp sits at the unaligned offset 2 of the record. */
typedef signed int UnalignedS32 __attribute__((aligned(1)));

/* A party member's character record (ov01 tu026.c: HP at +0x5C with its maximum the
 * unaligned int at +0x2, EP at +0x60 with its maximum at +0x6). saveWriteImage saves
 * the leader's level. */
typedef struct MenuPartyRecord {
    unsigned char unmodeled_00[2];
    UnalignedS32 maxHp;                     /* 0x02 */
    short maxEp;                            /* 0x06 */
    short strength;                         /* 0x08: "Str" ("Pow" in split mode) */
    short vitality;                         /* 0x0A: "Vit" ("Arm" in split mode) */
    short etherAttack;                      /* 0x0C: "Eatk" */
    short etherDefense;                     /* 0x0E: "Edef" */
    signed char dexterity;                  /* 0x10: "Dex" */
    signed char evasion;                    /* 0x11: "Eva" */
    signed char agility;                    /* 0x12: "Agl" */
    signed char level;                      /* 0x13 */
    unsigned char unmodeled_14[0x48];
    int hp;                                 /* 0x5C */
    short ep;                               /* 0x60 */
    unsigned char unmodeled_62[0x108 - 0x62]; /* menuDrawCharStats copies 0x108 bytes */
} MenuPartyRecord;

/* OV01 battleContextStateGet (config/symbols/ov01.txt): the state record (BattleState in
 * ov01/tu023.c, 0x108 bytes) of the member bound to slot key. */
MenuPartyRecord *battleContextStateGet(int key);

void func_0022A5A0(void) {
    battleContextStateGet(ptGetMenuMember()->id);
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
    for (i = 0; i < PARTY_SLOTS; i++) {
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

void ptSwapRosterEntries(int a, int b);

/* Swaps slots a and b (both 0-9 and occupied). frontIndex follows its member
 * when both are below 3, backIndex when both are 7 or 8; two back slots also
 * swap roster entries a - 7 / b - 7 and a - 4 / b - 4. */
void ptSwapSlots(int a, int b)
{
    PartyMember *member;

    if (a >= PARTY_SLOTS) {
        return;
    }
    if (b >= PARTY_SLOTS) {
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
    if (a >= PARTY_FRONT_SLOTS && b >= PARTY_FRONT_SLOTS) {
        if (a < 9 && b < 9) {
            if (ptState.backIndex + PARTY_FRONT_SLOTS == a) {
                ptState.backIndex = b - PARTY_FRONT_SLOTS;
            } else if (ptState.backIndex + PARTY_FRONT_SLOTS == b) {
                ptState.backIndex = a - PARTY_FRONT_SLOTS;
            }
        }
        ptSwapRosterEntries(a - 7, b - 7);
        ptSwapRosterEntries(a - 4, b - 4);
    }
}

/* Swaps the second roster's slots a and b (bounds-checked). */
void ptSwapRosterEntries(int a, int b)
{
    PartyMember *member;

    if (a < PARTY_SLOTS) {
        if (b < PARTY_SLOTS) {
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
/* The record ptState.progress points at. ptAddMember and ptRefresh address it
 * by its symbol, as the original does; the save functions go through
 * ptState.progress. */
extern PartyProgress D_006A5A30;

/* Puts member id (unless a slot already holds it) into the first free slot
 * of its group: ids 11 and up go to the back slots 7-9, the others to the
 * front slots 0-6. Its bit in D_006A5A30.memberMask is set and the counts kept up.
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
    for (i = 0; i < PARTY_SLOTS; i++) {
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
        if (id >= BACK_MEMBER_ID) {
            for (i = PARTY_FRONT_SLOTS; i < PARTY_SLOTS; i++) {
                if (ptState.slots[i] == 0) {
                    ptState.counts[2]++;
                    D_006A5A30.memberMask |= 1 << (id - 4);
                    slot = i;
                    break;
                }
            }
        } else {
            for (i = 0; i < PARTY_FRONT_SLOTS; i++) {
                if (ptState.slots[i] == 0) {
                    ptState.counts[1]++;
                    D_006A5A30.memberMask |= 1 << (id - 1);
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
    for (i = 0; i < PARTY_SLOTS; i++) {
        if (ptState.slots[i] != 0 && ptState.slots[i]->id == id) {
            /* The original compares the slot index unsigned (sltiu). */
            if ((unsigned int)i < PARTY_FRONT_SLOTS) {
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
                if (ptState.backIndex + PARTY_FRONT_SLOTS == i) {
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
            if (id >= BACK_MEMBER_ID) {
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
    if (slot >= PARTY_FRONT_SLOTS) {
        ptState.backIndex = slot - PARTY_FRONT_SLOTS;
        return;
    }
    ptState.frontIndex = slot;
}

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
        return ptState.backIndex + PARTY_FRONT_SLOTS;
    }
    return ptState.frontIndex;
}

int ptGetCurrentId(void)
{
    if (ptState.splitMode == 1) {
        return ptState.slots[ptState.backIndex + PARTY_FRONT_SLOTS]->id;
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
        if (frontIndex >= PARTY_FRONT_SLOTS) {
            break;
        }
    }
    return found;
}

/* Index of the second roster slot holding id, or -1 when none does. */
int ptFindRosterEntry(int id)
{
    PartyMember *record;
    int found;
    int i;

    found = -1;
    for (i = 0; i < PARTY_SLOTS; i++) {
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

/* The model (cloth) id of member id: ptApplyCurrentModel passes it to
 * uwamonoChangePlayerModel, ptLoadPartyModels to resPathSlotsLoadModels, and
 * ptRefresh keeps it in ptStatusBlock's clothIds. Ids 11 and up read a flat
 * table; the others compare the story-progress flag with a per-member
 * threshold, member 1 with an extra threshold/table pair. */
int ptGetClothId(int id)
{
    int progress;

    if (id >= BACK_MEMBER_ID) {
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
void ptApplyCurrentModel(void);

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
    ptApplyCurrentModel();
    if (flag != 0) {
        uwamonoStoreCurrent();
    }
}

void uwamonoChangePlayerModel(int request);

/* Passes the current member's ptGetClothId value to uwamonoChangePlayerModel;
 * in split mode (the back member) with bit 31 set. */
void ptApplyCurrentModel(void)
{
    PartyMember *record;
    int request;

    if (ptState.splitMode == 1) {
        record = ptState.slots[ptState.backIndex + PARTY_FRONT_SLOTS];
        if (record != 0) {
            request = ptGetClothId(record->id);
            uwamonoChangePlayerModel(request | 0x80000000);
        }
        return;
    }
    record = ptState.slots[ptState.frontIndex];
    if (record != 0) {
        request = ptGetClothId(record->id);
        uwamonoChangePlayerModel(request);
        return;
    }
}

void ptLoadPartyModels(void);

void ptPlaceMember(int id) {
    int slot;

    /* ids 11 and up go to the back slot 7 in split mode; the others to front slot 0. */
    ptAddMember();
    if (id >= BACK_MEMBER_ID) {
        ptEnterSplit();
        slot = ptFindSlot(id);
        ptSwapSlots(PARTY_FRONT_SLOTS, slot);
        ptSetCurrentSlot(PARTY_FRONT_SLOTS);
    } else {
        ptLeaveSplit();
        slot = ptFindSlot(id);
        ptSwapSlots(0, slot);
        ptSetCurrentSlot(0);
    }
    ptLoadPartyModels();
    ptRefreshCurrent();
}

/* Copies each front member (slots 0-6) that the roster does not hold yet
 * into the next free roster entry 3-5 whose back slot (entry + 4) is filled;
 * returns when no such entry is left. */
void ptFillCopilots(void)
{
    int i;
    int entry;

    entry = 3;
    for (i = 0; i < PARTY_FRONT_SLOTS; i++) {
        if (ptState.slots[i] != 0 && ptFindRosterEntry(ptState.slots[i]->id) == -1) {
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
void ptFillReserve(void)
{
    int i;
    int entry;

    entry = 6;
    for (i = 0; i < PARTY_FRONT_SLOTS; i++) {
        if (ptState.slots[i] != 0 && ptFindRosterEntry(ptState.slots[i]->id) == -1) {
            for (;;) {
                if (entry < 10 && ptState.roster[entry] != 0) {
                    entry++;
                } else {
                    break;
                }
            }
            if (entry >= 10) {
                break;
            }
            ptState.roster[entry] = ptState.slots[i];
        }
    }
}

/* {2, 6, 7}: the front member id paired with each of the back ids 11-13
 * (three bytes of tu108's .sdata). */
extern unsigned char D_0069D768[3];

/* Enters split mode when the party has back members. Roster entries 0-2 take
 * the D_0069D768 counterpart of the member in back slots 7-9. Entries 3-9
 * drop a member that left the party or that entries 0-2 now hold; entries
 * 3-5 are also dropped when their back slot (entry + 4) is empty. Then
 * ptFillCopilots and ptFillReserve fill the free entries. */
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
            member = ptState.slots[i + PARTY_FRONT_SLOTS];
            if (member != 0) {
                id = member->id;
                ptState.roster[i] = ptFindRecord(D_0069D768[id - BACK_MEMBER_ID]);
            } else {
                ptState.roster[i] = 0;
            }
        }
        for (i = 3; i < 6; i++) {
            if (ptState.roster[i] != 0) {
                if (ptFindRecord(ptState.roster[i]->id) == 0) {
                    ptState.roster[i] = 0;
                } else if (ptFindRosterEntry(ptState.roster[i]->id) < 3) {
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
                } else if (ptFindRosterEntry(ptState.roster[i]->id) < 3) {
                    ptState.roster[i] = 0;
                }
            }
        }
        ptFillCopilots();
        ptFillReserve();
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
    ptFillReserve();
}

/* The id in roster entry slot - PARTY_FRONT_SLOTS (0-2) for a back member, else 0. */
int ptGetMainPilotId(int id)
{
    int slot;
    PartyMember *member;

    if (id < BACK_MEMBER_ID) {
        return 0;
    }
    slot = ptFindSlot(id);
    if (slot < PARTY_FRONT_SLOTS) {
        return 0;
    }
    member = ptState.roster[slot - PARTY_FRONT_SLOTS];
    if (member != 0) {
        return member->id;
    }
    return 0;
}

/* The id in roster entry slot - 4 (3-5) for a back member, else 0: the
 * member tu101's special-attack list treats as its copilot. */
int ptGetCopilotId(int id)
{
    int slot;
    PartyMember *member;

    if (id < BACK_MEMBER_ID) {
        return 0;
    }
    slot = ptFindSlot(id);
    if (slot < PARTY_FRONT_SLOTS) {
        return 0;
    }
    member = ptState.roster[slot - 4];
    if (member != 0) {
        return member->id;
    }
    return 0;
}

/* 1 when every occupied back slot 7-9 has its roster entry 3-5 filled. */
int ptAllCopilotsAssigned(void)
{
    int result;
    int i;

    result = 1;
    for (i = 0; i < 3; i++) {
        if (ptState.slots[i + PARTY_FRONT_SLOTS] == 0) {
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

/* ptStatusBlock (0x24 bytes) as ptResetStatusBlock resets it and ptRefresh fills
 * it: the ids of the three members on screen and of the leader; memberMask, a
 * copy of D_006A5A30.memberMask (one bit per member who joined the party);
 * presentMask, one bit per member of the group on screen (front slots 0-6,
 * or back slots 7-9 in split mode; ov01's tu072 edits it as the MEMBER row);
 * and the ptGetClothId cloth id of each of the seven members (ov01's tu072
 * edits them as cloth ids). */
typedef struct PartyStatusBlock {
    unsigned char memberIds[3];
    unsigned char leaderId;
    unsigned short memberMask;
    unsigned short presentMask;
    int clothIds[7];
} PartyStatusBlock;

void *memset(void *dest, int value, unsigned int size);
extern PartyStatusBlock ptStatusBlock;

void ptResetStatusBlock(void)
{
    int *clothIds;
    int i;

    memset(&ptStatusBlock, 0, sizeof(ptStatusBlock));
    clothIds = ptStatusBlock.clothIds;
    for (i = 0; i < 7; i++) {
        clothIds[i] = D_003EC458[i];
    }
}

extern PartyMemberStatus D_006A5A80[10];
int xglFlagsGet2(int bit_offset);
/* OV01 battleSlotCodeSet (config/symbols/ov01.txt). */
void battleSlotCodeSet(int key, int code);
/* OV01 battleSlotSetShortLate (config/symbols/ov01.txt): stores pairedId as entry index
 * (0 or 1) of the slot of key (11 to 15). */
void battleSlotSetShortLate(int key, int index, int pairedId);

/* Rebuilds ptStatusBlock from the party and pairs the members for the
 * current split mode. */
void ptRefresh(void)
{
    PartyStatusBlock *block;
    PartyMember *leader;
    int progress;
    int i;

    block = &ptStatusBlock;
    if (ptState.splitMode == 1) {
        leader = ptState.slots[ptState.backIndex + PARTY_FRONT_SLOTS];
        if (leader != 0) {
            block->leaderId = leader->id;
        } else {
            block->leaderId = 0;
        }
        block->memberIds[2] = 0;
        for (i = 0; i < 2; i++) {
            if (ptState.slots[i + PARTY_FRONT_SLOTS] != 0) {
                block->memberIds[i] = ptState.slots[i + PARTY_FRONT_SLOTS]->id;
            } else {
                block->memberIds[i] = 0;
            }
        }
        block->presentMask = 0;
        for (i = 0; i < 3; i++) {
            if (ptState.slots[i + PARTY_FRONT_SLOTS] != 0) {
                block->presentMask |= 1 << (ptState.slots[i + PARTY_FRONT_SLOTS]->id - 4);
            }
        }
        for (i = 0; i < 3; i++) {
            if (ptState.slots[i + PARTY_FRONT_SLOTS] != 0) {
                if (ptState.roster[i] != 0) {
                    battleSlotSetShortLate(ptState.slots[i + PARTY_FRONT_SLOTS]->id, 0, ptState.roster[i]->id);
                } else {
                    battleSlotSetShortLate(ptState.slots[i + PARTY_FRONT_SLOTS]->id, 0, 0);
                }
                if (ptState.roster[i + 3] != 0) {
                    battleSlotSetShortLate(ptState.slots[i + PARTY_FRONT_SLOTS]->id, 1, ptState.roster[i + 3]->id);
                } else {
                    battleSlotSetShortLate(ptState.slots[i + PARTY_FRONT_SLOTS]->id, 1, 0);
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
        block->presentMask = 0;
        for (i = 0; i < PARTY_FRONT_SLOTS; i++) {
            if (ptState.slots[i] != 0) {
                block->presentMask |= 1 << (ptState.slots[i]->id - 1);
            }
        }
    }
    block->memberMask = D_006A5A30.memberMask;
    for (i = 0; i < 7; i++) {
        block->clothIds[i] = ptGetClothId(i + 1);
    }
    progress = xglFlagsGet8(STORY_PROGRESS_FLAG);
    for (i = 9; i >= 0; i--) {
        D_006A5A80[i].memberIndex = i;
    }
    /* From story progress 0x2B entries 2 and 5 take the indices 11 and 12,
     * from 0x57 entry 1 the index 10, after the others. */
    i = 10;
    if (progress >= 0x57) {
        D_006A5A80[1].memberIndex = i;
    }
    if (progress >= 0x2B) {
        D_006A5A80[2].memberIndex = 11;
        D_006A5A80[5].memberIndex = 12;
    }
    if (xglFlagsGet8(0x1EE) > 0) {
        /* The original leaves through battleSlotCodeSet in cases 1 and 2 (a jump)
         * and calls it in case 3. */
        switch (xglFlagsGet2(0x1D6)) {
        case 1:
            battleSlotCodeSet(3, 0x37);
            return;
        case 2:
            battleSlotCodeSet(3, 0x38);
            return;
        case 3:
            battleSlotCodeSet(3, 0x39);
            break;
        }
    }
}

/* One row of the table ptFlagCombinationIndex searches: the four two-bit flag values
 * it is chosen for; a row whose first value is 0 ends the table. */
typedef struct FlagCombination {
    unsigned char values[4];
} FlagCombination;

extern FlagCombination D_003EC4C0[];

/* 1-based index of the last row of D_003EC4C0 that matches the flags
 * 0x1DE, 0x1E1, 0x1D6 and 0x1DB, or 0 when none does. */
int ptFlagCombinationIndex(void)
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
void ptLoadPartyModels(void)
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
            modelId0 = ptGetClothId(ptState.slots[7]->id);
        }
        if (ptState.slots[8] != 0) {
            modelId1 = ptGetClothId(ptState.slots[8]->id);
        }
        if (ptState.slots[9] != 0) {
            modelId2 = ptGetClothId(ptState.slots[9]->id);
        }
    } else {
        if (ptState.slots[0] != 0) {
            modelId0 = ptGetClothId(ptState.slots[0]->id);
        }
        if (ptState.slots[1] != 0) {
            modelId1 = ptGetClothId(ptState.slots[1]->id);
        }
        if (ptState.slots[2] != 0) {
            modelId2 = ptGetClothId(ptState.slots[2]->id);
        }
    }
    resPathSlotsLoadModels(modelId0, modelId1, modelId2);
}

/* The clock reading xglClockRead fills (xgl_cd.c's XglClock). */
typedef struct XglClock {
    unsigned char status;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned short year;
} XglClock;

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

/* One 0x2B-byte record of D_006A5880, copied whole into the save image (tu100.c, tu101.c and
 * tu109.c read it as ControlBuffer; the member names are tu109.c's): the taken bits of one
 * member (header, cells), the highest open group, the taken count and the entries tu109.c's
 * selectionFindIndex searches. */
typedef struct SaveMemberName {
    unsigned char header[4];            /* +0x00 */
    unsigned char cells[4][8];          /* +0x04 */
    unsigned char openGroup;            /* +0x24 */
    unsigned char takenCount;           /* +0x25 */
    unsigned char entries[4];           /* +0x26 */
    unsigned char entry_count;          /* +0x2A */
} SaveMemberName;

/* The party as the save image keeps it: first id bytes of every slot and
 * roster entry, the progress record's fields and the leader. */
typedef struct SaveParty {
    unsigned char slotIds[10];
    unsigned char rosterIds[10];
    unsigned short memberMask;
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
    unsigned char unmodeled_14[0x28 - 0x14];
    unsigned char soundMode;                /* +0x28: the sound function func_001A4400 gets soundMode != 0 */
    unsigned char unmodeled_29[0x32 - 0x29];
    short displayOffsetX;                   /* +0x32: the horizontal argument of tyaSetDisplayOffset */
    short displayOffsetY;                   /* +0x34: the vertical argument */
    unsigned char unmodeled_36[0x3E - 0x36];
    unsigned char actuatorMode;             /* +0x3E: copied to both pads' actuatorMode */
    unsigned char unmodeled_3F[0x50 - 0x3F];
    long long version;
    unsigned char unmodeled_58[8];
    XglClock saveTime;
    XglClock playTime;
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

void xglClockRead(XglClock *time);
unsigned int xglClockDayTime2UInt(XglClock *time);
void xglClockUInt2DayTime(XglClock *time, unsigned int seconds);
int msgBalanceGet(void);

/* Fills the save image: play time, the global blocks, the party and the
 * counters, then the checksum. The blocks are copied through their address
 * (the original addresses them as SaveData+offset constants, not from the
 * SaveData base). */
#define MAP_ID_SPLIT_FLAG_MAP 0x1B91 /* entering it sets the progress record's mapFlag */
#define SAVE_VERSION 200

void saveWriteImage(void)
{
    XglClock now;
    long long sum;
    MenuPartyRecord *leaderInfo;
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
    party->memberMask = ptState.progress->memberMask;
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
    for (i = 0; i < PARTY_SLOTS; i++) {
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
            leaderInfo = battleContextStateGet(leader->id);
            party->leaderValue = leaderInfo->level;
        } else {
            party->leaderId = 0;
            party->leaderValue = 0;
        }
        for (i = 0; i < PARTY_FRONT_SLOTS; i++) {
            if (ptState.slots[i] != 0) {
                party->slotValues[i] = ptState.slots[i]->status->memberIndex + 1;
            } else {
                party->slotValues[i] = 0;
            }
        }
    } else {
        PartyMember *leader = ptState.slots[ptState.backIndex + PARTY_FRONT_SLOTS];

        if (leader != 0) {
            party->leaderId = leader->id;
            leaderInfo = battleContextStateGet(leader->id);
            party->leaderValue = leaderInfo->level;
        } else {
            party->leaderId = 0;
            party->leaderValue = 0;
        }
        for (i = 0; i < PARTY_BACK_SLOTS; i++) {
            if (ptState.slots[i + PARTY_FRONT_SLOTS] != 0) {
                party->slotValues[i] = ptState.slots[i + PARTY_FRONT_SLOTS]->status->memberIndex + 1;
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

PartyMember *ptMemberRecordFindById(int id);

/* Restores the play time base, the global blocks and the party from the save
 * image: the reverse of saveWriteImage. The blocks are read through a cast of
 * their address: with plain assignments (D_0061B380 = SaveData.flagBlock1080;
 * ...) gcc computes the later source addresses from one SaveData base register
 * and allocates other registers; through the cast each source is a
 * SaveData+offset constant (lui/addiu), as in the original. */
void saveReadImage(void)
{
    XglClock now;
    unsigned int playTime;
    SaveParty *party;
    int i;

    cursorBufferTablesInit();
    xglClockRead(&now);
    playTime = xglClockDayTime2UInt(&SaveData.playTime);
    ptState.progress->playTime = xglClockDayTime2UInt(&now) - playTime;
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
    ptState.progress->memberMask = party->memberMask;
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
    for (i = 0; i < PARTY_SLOTS; i++) {
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

#define MAP_ID_SPLIT_FLAG_RESUME 0x9EC /* the map a save made on MAP_ID_SPLIT_FLAG_MAP loads into */

/* Restores gameWork, the quest, citizen and segment flags and the balance
 * from the save image. A save made on MAP_ID_SPLIT_FLAG_MAP resumes on
 * MAP_ID_SPLIT_FLAG_RESUME with story progress 0x75. CONTROL_SOFT_RESET and
 * the control bits 27, 26 and 20 do not survive a load (tu069's runtimeSystem
 * sets 27 and toggles 20). The original copies the flag blocks inline in the
 * order quest, citizen, segment, all addressed from &SaveData.questFlags. A
 * plain memcpy is a call (the build has -fno-builtin), and plain struct
 * assignments (questFlags = SaveData.questFlags; ...) are scheduled by
 * priority whatever their order: the segment block is copied before the
 * citizen block and the control mask moves ahead of the copies. With
 * __builtin_memcpy in the order below the moves come out as in the original.
 * gameWork is read through a cast of its address as in saveReadImage: plain
 * gameWork = SaveData.game; addresses the copy from the SaveData base. */
void saveReadGameWork(void)
{
    int control;
    int balance;

    gameWork = *(SaveGameWork *) (unsigned char *) &SaveData.game;
    gameWork.state = gameWork.mapId;
    if (gameWork.mapId == MAP_ID_SPLIT_FLAG_MAP) {
        gameWork.mapId = gameWork.state = MAP_ID_SPLIT_FLAG_RESUME;
        xglFlagsSet8(STORY_PROGRESS_FLAG, 0x75);
    }
    control = gameWork.control;
    __builtin_memcpy(&questFlags, &SaveData.questFlags, sizeof(questFlags));
    balance = SaveData.balance;
    __builtin_memcpy(&segAddrOnFlags, &SaveData.segFlags, sizeof(segAddrOnFlags));
    __builtin_memcpy(&citizenFlags, &SaveData.citizenFlags, sizeof(citizenFlags));
    gameWork.control = control & ~(CONTROL_SOFT_RESET | 0x08000000 | 0x04000000 | 0x00100000);
    msgBalanceSet(balance);
}

void tyaSetDisplayOffset(s16, s16);

extern PadRecord PadData[2];

void padApplySaveSettings(void)
{
    PadData[0].actuatorMode = PadData[1].actuatorMode = SaveData.actuatorMode;
    func_001A4400(SaveData.soundMode != 0);
    tyaSetDisplayOffset(SaveData.displayOffsetX, SaveData.displayOffsetY);
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
    return (u8 *)&ptStatusBlock;
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
