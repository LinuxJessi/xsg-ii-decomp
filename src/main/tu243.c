#include "common.h"
#include "main/tu243.h"

/* The change from start to target divided by divisor, as a float then truncated to int. */
int deltaPerStep(int start, int target, int divisor) {
    return (int) ((float) (target - start) / (float) divisor);
}

/*
 * Level for a distance: the override value when easeStepOverrideEnabled is set; otherwise 45 when
 * |target - start| is under 31 and 23 when it is not.
 */
INCLUDE_ASM("asm/main/nonmatchings/tu243", easeAngleStep);

/* The override: easeStepOverrideEnabled is its enable flag, easeStepOverrideValue its value. */
extern int easeStepOverrideEnabled;
extern int easeStepOverrideValue;

/* Sets the override value and turns the override on. */
void easeStepOverrideSet(int value) {
    easeStepOverrideValue = value;
    easeStepOverrideEnabled = 1;
}

/* Turns the override off. */
void easeStepOverrideClear(void) {
    easeStepOverrideEnabled = 0;
}
