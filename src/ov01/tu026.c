#include "common.h"
#include "shared.h"
#include "ov01/tu026.h"

/*
 * Naming pass for ov01/tu026. Proposed global names are in
 * .work/x2-names/proposals/ov01-tu026.json and are applied later; the
 * INCLUDE_ASM and function lines keep their current names here. Each proposed
 * name is given in a comment above its function.
 */

extern int calcStatIdx2Bit(int statIdx);

/* calcHpAdjust: HP at +0x5C moves by delta, clamped to [0, max] (max is the unaligned int at +0x2); returns 1 while HP stays above 0. calcHpDmg calls it. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcHpAdjust);

/* calcEpAdjust: EP halfword at +0x60 moves by delta, clamped to [0, max] (max is the halfword at +0x6); returns 1. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcEpAdjust);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8BCD8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8BD48);

/* calcAtkPhy: sums the physical attack halfword at +0x8 with its status-row modifiers. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcAtkPhy);

/* calcAtkMag: as calcAtkPhy for the halfword at +0xC (magic attack); used by calcAttackDamage. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcAtkMag);

/* calcDefPhy: as calcAtkPhy for the halfword at +0xA (physical defence) on the defender. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcDefPhy);

/* calcDefMag: as calcAtkPhy for the halfword at +0xE (magic defence) on the defender. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcDefMag);

/* Partial layout of one element: only the byte func_00A8C1C8 returns is modeled. */
struct OvElement20 {
    unsigned char unmodeled_00[0xE];
    unsigned char value;
};

void *func_00A9B3B8(int);

unsigned char func_00A8C1C8(int unusedFirst, int index) {
    struct OvElement20 *element;

    element = func_00A9B3B8(index);
    return element->value;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8C1E8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8C278);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8C330);

/* calcHitRecordFill: fills a hit record for one target (its mode is set by calcHitMode). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8C3C8);

/* calcHitRoll: rnd(99) rolls checked against the record's bytes at +0x4 and +0x5 and the target's flags; returns 0, 1 or 2. Returns 0 when the unit's state byte at +0x70 is set. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcHitRoll);

/* calcHitMode: maps the kind byte (+0x7) of a command record to the packed hit mode that calcHpDmg takes; calls calcHitRoll for attack kinds. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcHitMode);

/* calcAttackDamage: damage of one attack from the attacker's and defender's stats, the command power and the status rows. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcAttackDamage);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8CF88);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D110);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D240);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D320);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D380);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D528);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D6E0);

/* Partial layouts: only the fields unitStateFlagGet touches are modeled. */
struct OwnerRecord {
    unsigned char unmodeled_00[0x18];
    signed char defaultCode;
    unsigned char unmodeled_19[0x70 - 0x19];
    signed char state;
};

struct OwnerObject {
    unsigned char unmodeled_00[0x144];
    struct OwnerRecord *record;
};

/*
 * unitStateFlagGet (proposed, medium): the bit flag of the unit's state byte (+0x70 of
 * the record at +0x144): 1 for state 3, 4 for state 2, else the record's default code.
 * Callers combine the result as a bit mask with 1, 2 and 4.
 */
signed char unitStateFlagGet(struct OwnerObject *object) {
    struct OwnerRecord *record;

    record = object->record;
    if (record->state == 3) {
        return 1;
    }
    if (record->state == 2) {
        return 4;
    }
    return record->defaultCode;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D8C0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D8F0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D950);

int func_00A8D9E8(int, int);

int func_00A8D9C8(int *firstValue, int *secondValue) {
    return func_00A8D9E8(*firstValue, *secondValue);
}

/* unitIsInFacingArc: true when the bearing from the first unit to the second lies within a quarter turn of the second unit's facing (+0x6C of its block). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8D9E8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8DB38);

/* calcComboUpdate: hit counter on the target (+0xF4, capped at 99); restarts when the kind mask (+0xF0) does not overlap. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcComboUpdate);

/* calcHpDmg (named earlier; see tu026 header). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcHpDmg);

/* calcHitApply: applies one filled hit record to its target: HP and EP, status set and clear, flags. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8DD00);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8DFF0);

/* calcStatGet: the status word of one row (rowValue for rows 1 to 4; rows 3 and 4 also OR altMask), masked. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcStatGet);

/* calcStatSet: sets a status bit in a row (activeMask), with its turn count from calcStatDefTurnGet. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcStatSet);

/*
 * Status block reached through the pointer at +0x144 of its owner. Only the
 * members the status helpers touch are named; the extents of altMask and
 * activeMask beyond the indices used here are not evidenced.
 */
typedef struct StatBlock StatBlock;

struct StatBlock {
    unsigned char unmodeled_00[0x52];       /* +0x00..+0x51 */
    unsigned short altMask[5];              /* +0x52: calcStatAltSet/Reset use indices 3 and 4 */
    unsigned char unmodeled_5c[0x92 - 0x5c]; /* +0x5c..+0x91 */
    short activeMask[4];                    /* +0x92: one value per row; set bits are active statuses */
    unsigned char turnCount[4][16];         /* +0x9a: per row and status bit, turns left */
};

/* The object the status helpers receive: +0x144 points at its StatBlock. */
typedef struct StatOwner StatOwner;

struct StatOwner {
    unsigned char unmodeled_000[0x144];     /* +0x000..+0x143 */
    StatBlock *stats;                       /* +0x144 */
};

/* calcStatAltSet: sets mask bits in altMask[statIdx] for statIdx 3 and 4. */
int calcStatAltSet(StatOwner *owner, int statIdx, int mask)
{
    if (statIdx >= 3 && statIdx <= 4) {
        owner->stats->altMask[statIdx] |= mask;
    }
    return 1;
}

/* calcStatReset: clears a status bit of a row (activeMask) and its turn count. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcStatReset);

/* calcStatAltReset: clears mask bits in altMask[statIdx] for statIdx 3 and 4. */
int calcStatAltReset(StatOwner *owner, int statIdx, int mask)
{
    if (statIdx >= 3 && statIdx <= 4) {
        owner->stats->altMask[statIdx] &= ~mask;
    }
    return 1;
}

/* calcStatResetCha: clears every row's activeMask and turn counts. */
void calcStatResetCha(StatOwner *owner)
{
    int row;
    int col;

    row = 1;
    do {
        owner->stats->activeMask[row - 1] = 0;
        col = 0;
        do {
            owner->stats->turnCount[row - 1][col] = 0;
            col++;
        } while (col < 16);
        row++;
    } while (row < 5);
}

/* calcStatTurn: each turn, counts down the turn counts of the active statuses and clears those that run out. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcStatTurn);

/* calcStatDefTurnTable (data name below): default turns for (row, statBit), via calcStatBit2Idx. */
extern unsigned char calcStatDefTurnTable[];

/* calcStatDefTurnGet: default turn count of status statBit in row, from calcStatDefTurnTable (row 1 to 4 only). */
signed char calcStatDefTurnGet(int row, int statBit)
{
    int bitIdx;

    if (row == 0 || row >= 5) {
        return 0;
    }
    bitIdx = calcStatBit2Idx(statBit);
    return (signed char)calcStatDefTurnTable[(row * 16) + bitIdx];
}

/*
 * The inverse of calcStatIdx2Bit below: the index of statBit's lowest set bit
 * among the 16 status bits calcStatSet, calcStatReset and calcStatDefTurnGet
 * index with it, or 16 when statBit holds none of them.
 */
int calcStatBit2Idx(int statBit)
{
    int statIdx;
    int mask;

    mask = 1;
    for (statIdx = 0; statIdx < 16; statIdx++) {
        if (statBit & mask) {
            break;
        }
        mask <<= 1;
    }
    return statIdx;
}

int calcStatIdx2Bit(int statIdx)
{
    return 1 << statIdx;
}

extern unsigned char calcStatInflictPenaltyTable[];

/*
 * calcStatInflictPenaltyGet (proposed, medium): signed byte of calcStatInflictPenaltyTable
 * (calcStatInflictPenaltyTable) for rows 1 and 2 and the status bit's index. calcStatInflictRate subtracts
 * it from the chance when func_00A8C1E8 returns 0.
 */
signed char calcStatInflictPenaltyGet(int row, int statBit)
{
    int bitIdx;

    if (row == 0 || row >= 3) {
        return 0;
    }
    bitIdx = calcStatBit2Idx(statBit);
    return (signed char)calcStatInflictPenaltyTable[(row * 16) + bitIdx];
}

/* calcStatInflictRate: percent chance (clamped to 0..100) that a status is inflicted: base 200 when the word at +0x4 of the first argument is 4, else the signed byte at +0x17 of its unit block; plus calcStatEffectSum; minus the table value from calcStatInflictPenaltyGet when func_00A8C1E8 returns 0. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcStatInflictRate);

INCLUDE_ASM("asm/nonmatchings/ov01/tu026", func_00A8E808);

/* unitApplyStatBonus: a bonus record flagged 0x8000 adds its value, scaled by calcStatEffectSum, to the unit's halfword at +0x8, capped at 999. Other branches not evidenced here. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", unitApplyStatBonus);

/* calcStatEffectSum: sums calcStatEffectValue over the four equipment ids at +0x86 of the unit block (each item's halfword at +0xA is the code). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", calcStatEffectSum);

/*
 * Record returned by func_00A9B508. Only the two halfwords read here are named.
 */
typedef struct StatusRecord StatusRecord;

struct StatusRecord {
    unsigned char unmodeled_00[0x8];        /* +0x00..+0x07 */
    unsigned short flags;                   /* +0x08: tested with the caller's mask */
    unsigned short code;                    /* +0x0a: low 7 bits are the result */
};

void *func_00A9B508(int);

/* calcStatEffectValue: low 7 bits of a record's code when its flags and code pass the masks, else 0. */
int calcStatEffectValue(int unusedArg0, int recordIdx, int flagMask, int codeMask)
{
    StatusRecord *record;
    int code;

    record = func_00A9B508(recordIdx);
    if ((record->flags & flagMask) != 0) {
        code = record->code;
        if ((code & codeMask) != 0) {
            return code & 0x7F;
        }
    }
    return 0;
}

/* unitHasEquipId: true when the value is one of the four equipment ids (halfwords at +0x86). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", unitHasEquipId);

/* unitHasEffectCode: true when an equipment item (ids at +0x86, item halfword at +0xA) or one of the four bytes at +0x34 equals the code; a record flagged 0x2000 takes the slotFlagTest path instead. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu026", unitHasEffectCode);

int unitHasEffectCode(void *entry, void *target);
int unitTblGet(int side, void ***table);

/* partyHasEffectCode (proposed, medium): true when a unit of side 0 (the party, unitTblGet) has effect code effectCode. menuTgtStatusDraw calls it with 0x10. */
int partyHasEffectCode(void *effectCode)
{
    void **entries;
    void *entry;
    int count;
    int idx;

    idx = 0;
    count = unitTblGet(0, &entries);
    if (count > 0) {
        do {
            entry = entries[idx];
            idx++;
            if (entry != 0 && unitHasEffectCode(entry, effectCode) != 0) {
                return 1;
            }
        } while (idx < count);
    }
    return 0;
}
