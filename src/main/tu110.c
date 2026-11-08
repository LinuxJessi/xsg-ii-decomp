#include "common.h"
#include "main/tu022.h"
#include "main/tu110.h"
#include "main/tu121.h"
#include "main/tu148.h"

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022A560);

#include "shared.h"

int *func_0022A560(void);
/* The member data record func_A8B490 returns; saveWriteImage saves value for the leader. */
typedef struct PartyMemberInfo {
    unsigned char unmodeled_00[0x13];
    unsigned char value;
} PartyMemberInfo;

PartyMemberInfo *func_A8B490(int id);

void func_0022A5A0(void) {
    func_A8B490(*func_0022A560());
}

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptFindSlot);

/* Partial view of one 0x870-byte record of D_003400F8: only the leading
 * float is read by these functions. */
typedef struct ScaledEntry {
    float value;
    unsigned char unmodeled_04[0x86C];
} ScaledEntry;

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
} PartyMemberStatus;

/* A member record: word 0 is the member id. */
typedef struct PartyMember {
    int id;
    PartyMemberStatus *status;
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
} PartyState;

extern PartyState ptState;
extern ScaledEntry D_003400F8[];
extern int uwamonoCurrentIndex;
extern unsigned char ptSplitMode[];

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

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptSwapSlots);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022A7D8);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptAddMember);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptRemoveMember);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptResetSlots);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptSetCurrentSlot);

int ptFindSlot();
void ptSetCurrentSlot(int);

void ptSelectMember(void) {
    int slot;

    slot = ptFindSlot();
    if (slot != -1) {
        ptSetCurrentSlot(slot);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptGetCurrentSlot);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptGetCurrentId);

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

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022ADA0);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022AE10);

int uwamonoLoadCurrent();
int uwamonoStoreCurrent();
int func_0022AF98();

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

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022AF98);

void ptSwapSlots(int, int);
void ptAddMember();
void ptEnterSplit();
void ptLeaveSplit();
void func_0022BAF8();

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

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022B0B8);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022B1C8);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptEnterSplit);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptLeaveSplit);

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

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022B498);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022B4E8);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022B538);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptGroupCount);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022B5E0);

INCLUDE_ASM("asm/main/nonmatchings/tu110", ptRefresh);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022BA18);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022BAF8);

INCLUDE_ASM("asm/main/nonmatchings/tu110", saveChecksum);

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
    unsigned char unmodeled_00[0x18];
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

void ptRefresh(void);
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

INCLUDE_ASM("asm/main/nonmatchings/tu110", saveReadImage);

INCLUDE_ASM("asm/main/nonmatchings/tu110", func_0022CEE0);

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

extern u8 ptStatusBlock[];

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
