/*
 * OV01 original TU 25: 0x00a8ba68..0x00a8bc60 (7 functions)
 */
#include "common.h"
#include "ov01/tu025.h"

/* Body fields the functions below touch (the body is reached through the
 * unit's pointer at +0x144): +0x1C flags, +0x71 level (0..3), +0x80 percent
 * (0..100), +0x82 current value clamped to the max at +0x84. */
/* unitPercentAdd: adds amount to the percent at body+0x80 (clamped 0..100) only
 * while flag 0x800 of body+0x1C is set; returns 0 and changes nothing otherwise. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu025", unitPercentAdd);

/* The boost gauge a unit owns: func_00A91708 (tu031) picks one of two gauges
 * for a unit. stock is the number of banked boosts, capped by maxStock;
 * progress counts toward the next stock and is held at 50 while the gauge is
 * full. The three bytes are at +0, +1 and +2 of the gauge. */
struct BoostGauge {
    signed char maxStock;
    signed char stock;
    signed char progress;
};

struct BoostGauge *func_00A91708(void);
int boostGaugeAddStock(struct BoostGauge *gauge, int amount);

/* Adds amount to the stock of the unit's boost gauge. The unit is the first
 * parameter; the original passes it on to func_00A91708, this source calls
 * func_00A91708() without it. calcActStockSet (tu031) calls it with -1 when a
 * boost is spent. */
void boostAddStock(void *unit, int amount) {
    boostGaugeAddStock(func_00A91708(), amount);
}

/* Stock step: stock (+1) moves by amount, clamped to 0..maxStock (+0). A gain
 * that fills the gauge holds progress (+2) at 50; a non-positive change clears
 * a progress held at 50. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu025", boostGaugeAddStock);

int boostGaugeAddProgress(struct BoostGauge *gauge, int amount);

/* Adds amount to the progress of the unit's boost gauge. Same unit note as
 * boostAddStock. Callers: func_00A92C18 and func_00A93D50 (tu033). */
void boostAddProgress(int unit, int amount) {
    boostGaugeAddProgress(func_00A91708(), amount);
}

/* Progress step: progress (+2) moves by amount, floored at 0. Reaching 50
 * banks one stock through boostGaugeAddStock and subtracts 50; a full gauge pins
 * the progress at 50. Returns 1. */
int boostGaugeAddProgress(struct BoostGauge *gauge, int amount) {
    int next_progress;

    next_progress = gauge->progress + amount;
    if (next_progress < 0) {
        next_progress = 0;
    } else if (next_progress >= 0x32) {
        boostGaugeAddStock(gauge, 1);
        next_progress = next_progress - 0x32;
        if (gauge->stock >= gauge->maxStock) {
            next_progress = 0x32;
        }
    }
    gauge->progress = (signed char) next_progress;
    return 1;
}

/* Level step: body+0x71 moves by amount, clamped to 0..3. Returns 1.
 * Callers include battleResolveStatus (tu033), which passes 3. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu025", unitLevelAdd);

/* Current-value step: body+0x82 moves by amount, floored at 0 and capped at
 * the max stored at body+0x84. Returns 1. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu025", unitCurrentAdd);
