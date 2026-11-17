#ifndef INCLUDE_OV01_TU023_H
#define INCLUDE_OV01_TU023_H

/* Functions of src/ov01/tu023.c (the OV01 battle file) that other files call. */

#include "shared.h"
#include "ov01/tu012.h"
#include "ov01/tu042.h"

/* StatOwner (the battle unit) and StatBlock (its +0x144 block) are defined with the unit object in tu012.h. */

/*
 * The boost gauge of a side (boostGaugeParty for the party, boostGaugeRest for the rest; boostGaugeGet
 * picks one for a unit). stock is the number of banked boosts, capped by maxStock (counted up to 5 by
 * calcHitApply, through its unsigned view); progress counts toward the next stock and is held at 50
 * while the gauge is full.
 */
struct BoostGauge {
    union {
        signed char maxStock;
        unsigned char maxStockCount;
    };                                      /* +0x00 */
    signed char stock;                      /* +0x01 */
    signed char progress;                   /* +0x02 */
};

/* Slots of the action stock: four stocked units and two boost units. */
#define ACT_STOCK_SLOTS 4
#define ACT_STOCK_BOOST_SLOTS 2

/*
 * The action-stock record (the global actStock, X1 ActStock layout): stock[4] at +0x00,
 * boost[2] at +0x10 (calcActStockSet fills them), max at +0x18, pickCursor at +0x1C,
 * lastPicked at +0x20, pickSource at +0x24.
 */
struct ActStock {
    StatOwner *stock[ACT_STOCK_SLOTS];
    StatOwner *boost[ACT_STOCK_BOOST_SLOTS];
    int max;
    int pickCursor;
    StatOwner *lastPicked;
    int pickSource;
};

struct ActStock *actStockGet(void);

/*
 * Hit record of one target: calcHitRoll rolls for it and calcHitRecordFill fills it; the status
 * steps (calcCritDamage, calcHitStateStep, calcHitRowGate, calcFollowKind) read it.
 */
struct HitRecord {
    struct Ov01Object *target;                      /* +0x00 */
    int reaction;                           /* +0x04: the UNIT_CMD_* the target answers with (attackQueue) */
    int damage;                             /* +0x08: calcAttackDamage result, or the calcRestoreValue result for type 2 */
    int followResult;                       /* +0x0c: calcFollowKind result, stored by calcAttackDamage */
    int backAttack;                         /* +0x10: calcBackAttack's facing test for kind 1; 1 skips calcHitRoll and adds 50 to calcCritDamage's chance */
    int stepState;                          /* +0x14: 1 to 3, written by calcHitRowGate and calcHitStateStep */
    union {
        int word;                           /* calcHitMode result, or the calcKindHitMode result for type 2: bit 8 set once taken, bit 9 blocks it */
        unsigned char kind;                 /* its low byte: the hit kind calcHitInflict tests */
    } hitMode;                              /* +0x18 */
    int inflictRow;                         /* +0x1c: row calcHitInflict inflicts */
    int inflictMask;                        /* +0x20: bits calcHitInflict inflicts */
    int cureRow;                            /* +0x24: row calcHitCure clears */
    int cureMask;                           /* +0x28: bits calcHitCure clears */
};

/*
 * Command record of one attack, with the hit record of its current target. battleAttackResolve
 * and calcHitApply build one on their frame; tgtFindFirst reads its attacker and action. The tu033 command
 * handlers (func_00A92C18 builds it, func_00A92D98, func_00A92DF0, func_00A93288) take one too.
 */
struct AttackCommand {
    struct Ov01Object *attacker;                    /* +0x00 */
    int kind;                               /* +0x04: 4 selects the base chance 200 in calcStatInflictRate */
    int actionIdx;                          /* +0x08: index tecRecordGet takes */
    int id;                                 /* +0x0C: the lookup key (the third argument of calcHitApply) */
    struct HitRecord hit;                   /* +0x10 */
};
StatOwner *actStockPick(void);
int unitCanAct(StatOwner *unit);

/* Battle unit and action-stock prototypes. */
int battleSlotFlagTest(int key, int flag);
struct StatBlock *battleSlotGet(int index);
int battleEffectApplies(struct Ov01Object *command, int index, struct Ov01Object *entity);
int battleStateClearStatus(struct Ov01Object *owner);
int partyUnitHpLow(struct Ov01Object *owner);
int tgtCollectUnits(struct Ov01Object *target, int flags, struct Ov01Object **found);
void boostAddStock(struct Ov01Object *unit, int amount);
struct BoostGauge *boostGaugeSelect(int flag);
int calcStatBit2Idx(int statBit);
void calcActStockSet(struct Ov01Object *unit, int slot);
int calcBoostOk(struct Ov01Object *obj, int id);
int calcEntryUsable(struct Ov01Object *entity, int index);
/* Unit HP and status rows: calcHpAdjust returns 1 while HP stays above 0, calcStatGet the set bits of mask in
 * row, calcStatReset/calcStatAltReset clear them (both return 1). */
int calcHpAdjust(struct Ov01Object *unit, int delta);
int calcStatGet(struct Ov01Object *owner, int row, int mask);
int calcStatReset(struct Ov01Object *owner, int row, int mask);
int calcStatAltReset(struct Ov01Object *owner, int statIdx, int mask);
void calcHitRecordFill(struct AttackCommand *action, struct HitRecord *hit);
int actLevelChanceRoll(struct Ov01Object *owner, int index);
int actStepCounterValue(void);
void actStockClearBoost1(void);
void sdvInitSpecialWork(void);

/*
 * The 0x50-byte battle result block (battleResult): word totals at +0x00, +0x04 and +0x08; up to eight
 * (category, id) pairs of dropped items from +0x0C and +0x2C (dataBoxInc and dataBoxChk take them in
 * that order, battleResultBuildEntries of src/main/tu122.c switches on the category); the pair count at +0x4C.
 */
struct BattleResult {
    int points;                    /* +0x00: points summed by battleResultAdd */
    int stepRateTotal;             /* +0x04: stepRate summed, scaled by the phase-2 multiplier */
    int flatTotal;                 /* +0x08: flat summed by battleResultAdd */
    int categories[8];             /* +0x0C: data box category of each pair */
    int ids[8];                    /* +0x2C: data box id of each pair */
    int count;                     /* +0x4C */
};

/*
 * One 28-byte row battleResultApply fills per party unit of keys below 16: the unit key, the three stat gains
 * battleGainsAdd adds to the slot record and the levels, hp cap and ep cap the experience granted.
 */
struct BattleResultRow {
    int key;                       /* +0x00 */
    int gainFirst;                 /* +0x04 */
    int gainSecond;                /* +0x08 */
    int gainThird;                 /* +0x0C */
    int levelGain;                 /* +0x10 */
    int maxHpGain;                 /* +0x14 */
    int maxEpGain;                 /* +0x18 */
};

/* Battle result block and party snapshot prototypes. */
struct BattleResult *battleResultGet(void);
int battleResultApply(struct BattleResultRow *rows);
int partySnapshotGet(StatOwner ***snapshots);

#endif /* INCLUDE_OV01_TU023_H */
