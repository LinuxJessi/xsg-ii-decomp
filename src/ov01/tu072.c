/*
 * OV01 original TU 72: 0x00aa9340..0x00aaaa50 (50 functions)
 *
 * The BATTLE CONFIG debug menu: a main page (THINK, MONSTER SET, the three
 * party rows, mute flags, LEARN ALL, CF FLAG, GATTAI, MEMBER), an equip page
 * for the selected unit, and a script page.
 */
#include "common.h"
#include "shared.h"
#include "ov01/tu023.h"
#include "ov01/tu036.h"

/* Callees defined outside this allocation. Results are declared only where a caller reads them. */
u8 *func_00A91730(int);
void func_00A99DD8(int);
void cfEventSet(int);
void debugValLR(int *, int, int, int);
int configPlayer(int slot);
int configPlPos(int slot);
extern int configMonsSetNo;
extern int configCFFlagValue;
extern int configCpUDispFlag;
extern int configPlMutekiFlag;
extern int configEnMutekiFlag;
extern int configPlIchigekiFlag;
extern int configStockMugenFlag;
extern int configLearnAllFlag;

/* Mute and ICHIGEKI rows of the main page: constant 0 in this build. */
int configPlMutekiGet(void) {
    return 0;
}

int configEnMutekiGet(void) {
    return 0;
}

int configPlIchigekiGet(void) {
    return 0;
}

int configStockMugenGet(void) {
    return 0;
}

void func_00AA9360(void) {

}

void func_00AA9368(void) {

}

void func_00AA9370(void) {

}

void func_00AA9378(void) {

}

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", configInit);

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", configMenuRun);

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", configMenuUpdate);

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", configMenuFrame);

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", configMainPageDraw);

char *xglCdFindFileName(unsigned int file_id);
int *ptGetStatusBlock(void);
extern char D_00ABAF10[];

/* Cloth name of a unit: table entry unit + 1 resolved to a name, "NULL" when missing. */
void *unitClothNameGet(int unit)
{
    int *table;
    void *entry;

    if (unit < 0xB) {
        table = ptGetStatusBlock();
        entry = xglCdFindFileName(table[unit + 1]);
        if (entry != 0) {
            return entry;
        }
    }
    return &D_00ABAF10;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", configEquipPageDraw);

/* STOCK MUGEN apply (proposed configStockMugenApply): run by configMenuFrame only when the
 * STOCK MUGEN row value is nonzero. Copies byte 0 of the stock record to byte 1. */
void configStockMugenApply(void) {
    u8 *stockRecord;

    stockRecord = func_00A91730(0);
    stockRecord[1] = *func_00A91730(0);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", configLearnAllApply);


/* Bitmask of unit flags 1..32 (the LEARN SKILL row). */
int unitLearnMaskGet(int unit)
{
    int mask;
    int bit;
    int index;

    if (unit == 0) {
        return 0;
    }
    mask = 0;
    bit = 1;
    for (index = 1; index <= 0x20; index++) {
        if (battleSlotFlagTest(unit, index) != 0) {
            mask |= bit;
        }
        bit *= 2;
    }
    return mask;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", unitLearnMaskSet);

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", configScriptPage);

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", configSelectPressed);

extern int configThinkNo;

/* THINK row: edits the AI script number (0..999) and passes it on. */
int configThink(void)
{
    debugValLR(&configThinkNo, 0, 999, 1);
    thinkNoSet(configThinkNo);
    return 0;
}

/* MONSTER SET row: edits the enemy group number (0..999) and passes it on. */
int configMonsSet(void) {
    debugValLR(&configMonsSetNo, 0, 999, 1);
    func_00A99DD8(configMonsSetNo);
    return 0;
}

void configPlayer1(void) {
    configPlayer(0);
}

void configPlayer2(void) {
    configPlayer(1);
}

void configPlayer3(void) {
    configPlayer(2);
}

/* One party slot of the BATTLE CONFIG records (ov01PartyListGet returns the table, 0x14 bytes each). */
typedef struct ConfigPartyRecord {
    int charaId;  /* PLAYERn name row, resolved through func_00A9B338 */
    int position; /* PLAYERn POS row */
    unsigned char unmodeled_08[0x14 - 0x08];
} ConfigPartyRecord;

void *ov01PartyListGet(void);

/* PLAYERn row: cycles the character of party slot `slot` (0..0xF). */
int configPlayer(int slot)
{
    ConfigPartyRecord *records;
    int charaId;

    records = ov01PartyListGet();
    charaId = records[slot].charaId;
    debugValLR(&charaId, 0, 0xF, 1);
    records[slot].charaId = charaId;
    return 0;
}

void configPlPos1(void) {
    configPlPos(0);
}

void configPlPos2(void) {
    configPlPos(1);
}

void configPlPos3(void) {
    configPlPos(2);
}

/* PLAYERn POS row: cycles the position of party slot `slot` (0..0x1F). */
int configPlPos(int slot)
{
    ConfigPartyRecord *records;
    int position;

    records = ov01PartyListGet();
    position = records[slot].position;
    debugValLR(&position, 0, 0x1F, 1);
    records[slot].position = position;
    return 0;
}

/* Handler of the PL MUTEKI row (proposed configPlMuteki): slot 10 of the main-page table D_00AB6B00.
 * The flag it edits is never read in ov01; the row prints the constant from configPlMutekiGet. */
int configPlMuteki(void) {
    debugValLR(&configPlMutekiFlag, 0, 1, 1);
    return 0;
}

int configEnMuteki(void) {
    debugValLR(&configEnMutekiFlag, 0, 1, 1);
    return 0;
}

int configPlIchigeki(void) {
    debugValLR(&configPlIchigekiFlag, 0, 1, 1);
    return 0;
}

int configStockMugen(void) {
    debugValLR(&configStockMugenFlag, 0, 1, 1);
    return 0;
}

/* LEARN ALL row: a 0..1 flag; when it is 1 the page runs configLearnAllApply. */
int configLearnAll(void) {
    debugValLR(&configLearnAllFlag, 0, 1, 1);
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", func_00AAA288);

/* CF FLAG row: a 16-bit value written through cfEventSet. */
int configCFFlag(void) {
    debugValLR(&configCFFlagValue, 0, 0xFFFF, 1);
    cfEventSet(configCFFlagValue);
    return 0;
}

/* CP U DISP row: a 0..1 flag that picks the display routine on the main page. */
int configCpUDisp(void) {
    debugValLR(&configCpUDispFlag, 0, 1, 1);
    return 0;
}

int func_00A99E18();

/* GATTAI row: an id offset by 0x16000 in 1..0x51. */
int configGattai(void)
{
    int gattaiId;

    gattaiId = func_00A99E18() - 0x16000;
    debugValLR(&gattaiId, 1, 0x51, 1);
    gattaiCodeSet(gattaiId + 0x16000);
    return 0;
}

/* The block ptGetStatusBlock returns; its halfword at +6 is the MEMBER row. */
typedef struct ConfigGlobalBlock {
    unsigned char unmodeled_00[6];
    unsigned short member; /* MEMBER row, 0..0x3FF */
} ConfigGlobalBlock;

int configMember(void)
{
    int member;

    member = ((ConfigGlobalBlock *) ptGetStatusBlock())->member;
    debugValLR(&member, 0, 0x3FF, 1);
    ((ConfigGlobalBlock *) ptGetStatusBlock())->member = member;
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", configUnitSelect);

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", configEquipSkill);

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", configLearnSkill);

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", configAcc);

extern int configUnitIndex;

/* CLOTH row (proposed configCloth): edits the cloth id in the unit table entry unit + 1,
 * the entry unitClothNameGet resolves to the name printed on the equip page. */
int configCloth(void)
{
    int clothId;

    clothId = ptGetStatusBlock()[configUnitIndex + 1];
    debugValLR(&clothId, 0x10001, 0x10028, 1);
    ptGetStatusBlock()[configUnitIndex + 1] = clothId;
    return 0;
}

/* One unit of the battle roster (battleSlotGet returns it, 0x108 bytes each). */
typedef struct ConfigUnit {
    unsigned char unmodeled_00[0x13];
    signed char level;         /* +0x13, the LV UP row */
    unsigned char unmodeled_14[0x90 - 0x14];
    short subPilotId;          /* +0x90, the SUB PILOT row */
} ConfigUnit;

void *battleSlotGet(int);
ConfigUnit *func_00A9B2A8(int);

/* SUB PILOT row: edits the sub-pilot id (0..0xA) of the selected unit. */
int configSubPilot(void)
{
    ConfigUnit *unit;
    int subPilotId;

    unit = battleSlotGet(configUnitIndex);
    subPilotId = unit->subPilotId;
    debugValLR(&subPilotId, 0, 0xA, 1);
    unit->subPilotId = subPilotId;
    return 0;
}

void unitSetLevel(int, int);

/* LV UP row: the minimum comes from the base unit; a change goes through unitSetLevel. */
int configLvUp(void)
{
    ConfigUnit *unit;
    ConfigUnit *baseUnit;
    int level;

    unit = battleSlotGet(configUnitIndex);
    baseUnit = func_00A9B2A8(configUnitIndex);
    level = unit->level;
    debugValLR(&level, baseUnit->level, 0x63, 1);
    if (level != unit->level) {
        unitSetLevel(configUnitIndex, level);
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", unitSetLevel);

INCLUDE_ASM("asm/nonmatchings/ov01/tu072", configScriptNo);
