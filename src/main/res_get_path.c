#include "common.h"
#include "shared.h"
#include "main/res_get_path.h"
#include "main/tu021.h"
#include "main/tu022.h"

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E0B10);

int func_001DF980(void);
extern int resSelectedSlot[];

/* Stores the path slot index in resSelectedSlot[0]: the slot found by the search (func_001DF980) when search is
   nonzero, otherwise slot 1. Callers pass 1 to search and 0 for the default (proposed resSelectSlot). */
void resSelectSlot(int search) {
    if (search != 0) {
        resSelectedSlot[0] = func_001DF980();
    } else {
        resSelectedSlot[0] = 1;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resResetDescriptorTables);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resClearRecordTable);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resResetKindState);

void resResetKindState(int mode);

void resResetActState(void) {
    resResetKindState(0x10000);
}

void resClearRecordTable(void);

void resResetAllKinds(void) {
    resResetKindState(-1);
    resClearRecordTable();
}

void func_001E0E98(void) {
    resResetKindState(0);
}

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E0EB0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resReleaseRecordsOfType);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E0FD0);

void func_0013A618(int, int);

void soundRestart(void) {
    soundSlotsRestart();
    func_0013A618(1, 1);
    func_001A4010();
}


void soundReset(void) {
    soundRestart();
    seSetMuted(1, 0);
    func_001A3A18();
}

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1218);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E12B8);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1440);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1508);

extern f32 seMaxDistance;

/* Sets the sound's maximum audible distance when maxDistance is not negative; returns the current value
   (proposed seSetMaxDistance). The distance check in func_001E16A0 reads seMaxDistance. */
f32 seSetMaxDistance(f32 maxDistance) {
    if (maxDistance >= 0.0f) {
        seMaxDistance = maxDistance;
    }
    return seMaxDistance;
}

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E16A0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1830);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1978);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1A40);

void func_001E1B10(void) {
}

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resResetSlotTable);

void resSelectSlot(int search);
void resResetDescriptorTables(void);
void resResetSlotTable(void);
extern signed char D_0069CFD0;

/* 20-byte entries of xglCdSlotTable; entry 0 is the header that holds the
   two words stored by resInitPathHeader and the matched index. */
typedef struct {
    int key;
    int value;
    int index;
    u8 unmodeled_0C[8];
} ResPathEntry;

extern ResPathEntry xglCdSlotTable[];

/* Stores key and value in the header entry, then resets the path state (proposed resInitPathHeader). */
void resInitPathHeader(int key, int value) {
    xglCdSlotTable[0].key = key;
    xglCdSlotTable[0].value = value;
    resSelectSlot(0);
    resResetSlotTable();
    resResetDescriptorTables();
    resClearRecordTable();
    D_0069CFD0 = 0;
}

u8 *next_line(u8 *cursor)
{
    for (;;) {
        while (*cursor != '\n' && *cursor != '\r')
            cursor++;
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r')
            cursor++;
        if (*cursor != ';')
            return cursor;
    }
}

u8 *skip_space(u8 *cursor)
{
    while (*cursor == ' ' || *cursor == '\t')
        cursor++;
    return cursor;
}

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resParseFileGroup);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resEntryAdd);

void resParseFileGroup(int, int, int *);
void resEntryAdd(int, int);

/* Parses a name at cursor for kind and records the match; returns the cursor (proposed resCmdResolve). */
int resCmdResolve(int cursor, int kind) {
    int entryId;

    resParseFileGroup(cursor, kind, &entryId);
    resEntryAdd(entryId, 0);
    return cursor;
}

/* Script command handlers; each passes its kind to resCmdResolve (act: 0x10000, map: 0x20000, mot: 0x30000, mobj: 0x70000). */
void resCmdAct(int cursor) {
    resCmdResolve(cursor, 0x10000);
}

void resCmdMap(int cursor) {
    resCmdResolve(cursor, 0x20000);
}

void resCmdMot(int cursor) {
    resCmdResolve(cursor, 0x30000);
}

void resCmdMobj(int cursor) {
    resCmdResolve(cursor, 0x70000);
}

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resCmdEffect);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resCmdMapse);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resCmdRuntimeBgm);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resCmdBgmVol);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resCmdFoot);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resCmdEnefoot);

u8 *resCmdDisk(u8 *entry) {
    resEntryAdd(0xF10000, *entry + 0x1D0);
    return entry;
}

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resCmdSave);

void GameMovieCaptionPlay(void);

/*
 * Script command ";mpeg2" (name at 0x002BB670, handler at 0x002BB674 in the command table; proposed resCmdMpeg2).
 * It plays the caption and returns the script cursor unchanged, as the other resCmd handlers do.
 */
int resCmdMpeg2(int cursor) {
    GameMovieCaptionPlay();
    return cursor;
}

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resScriptRun);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E2E08);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resLoadCfFile);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E3000);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resParseJumpTarget);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E3308);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E3630);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E36F8);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resResetNameTables);

void xglCdReadCancel(int);
void resResetNameTables(void);

void resResetScriptState(void) {
    xglCdReadCancel(0);
    resResetNameTables();
}

/* Adds the record's address to its pointer words (toAbsolute != 0) or subtracts it, for kinds 0x10000, 0x20000 and 0x70000 (proposed resRelocateRecord). */
void resRelocateRecord(int toAbsolute, int kindWord, int *record) {
    int delta;
    u32 kind;

    kind = kindWord & 0xFF0000;
    switch (kind) {
    case 0x10000:
    case 0x20000:
    case 0x70000:
        break;
    default:
        return;
    }
    delta = (int) record;
    if (toAbsolute == 0) {
        delta = -delta;
    }
    if (record[0] != 0) {
        record[0] += delta;
    }
    if (record[1] != 0) {
        record[1] += delta;
    }
    if (record[2] != 0) {
        record[2] += delta;
    }
    if (record[3] != 0) {
        record[3] += delta;
    }
    if (record[4] != 0) {
        record[4] += delta;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E39D0);

extern unsigned char D_006A0AC0[];
extern unsigned char resEntryTable[];

void resGetEntryTables(u8 **first, u8 **second) {
    if (first != 0) {
        *first = resEntryTable;
    }
    if (second != 0) {
        *second = D_006A0AC0;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E4000);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E40E0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E4178);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", resSetPathSlot);

typedef struct PathSlot {
    int base;
    int unmodeled_04;
} PathSlot;

extern PathSlot resPathSlots[];
void resSetPathSlot(int slot, int base, int limit, int value);

void resUpdatePathSlot(int slot)
{
    int base;

    if (slot < 0) {
        base = resPathSlots[2].base;
        resSetPathSlot(2, base, base + 0xC6000, base);
        if (slot < -1) {
            base = resPathSlots[1].base;
            resSetPathSlot(1, base, base + 0xC6000, base);
        }
        if (slot < -2) {
            base = resPathSlots[0].base;
            resSetPathSlot(0, base, base + 0xC6000, base);
        }
        return;
    }
    base = resPathSlots[slot].base;
    resSetPathSlot(slot, base, base + 0xC5FC0, xglCdSlotTable[xglCdSlotTable[0].index].key);
}

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E44F0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E4700);

void func_001DC468(int, int, int);
void func_001E4700(int);

void resResetPathSlots(void) {
    int i;

    func_001E4700(resPathSlots[0].base);
    func_001E4700(resPathSlots[1].base);
    func_001E4700(resPathSlots[2].base);
    for (i = 1; i < 0x80; i++) {
        if (xglCdSlotTable[i].key == resPathSlots[3].base) {
            xglCdSlotTable[0].index = i;
            break;
        }
    }
    func_001DC468(resPathSlots[0].unmodeled_04, resPathSlots[1].unmodeled_04, resPathSlots[2].unmodeled_04);
}

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E4890);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E4AE0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E4B60);
