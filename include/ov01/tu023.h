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
