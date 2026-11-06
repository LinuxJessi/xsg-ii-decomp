#include "common.h"
#include "main/tu243.h"

/* The change from start to target divided by divisor, as a float then truncated to int. */
int func_00286ED0(int start, int target, int divisor) {
    return (int) ((float) (target - start) / (float) divisor);
}

/*
 * Level for a distance: the override value when D_0069DD60 is set; otherwise 45 when
 * |target - start| is under 31 and 23 when it is not.
 */
INCLUDE_ASM("asm/main/nonmatchings/tu243", func_00286F08);

/* The override: D_0069DD60 is its enable flag, D_0069DD64 its value. */
extern int D_0069DD60;
extern int D_0069DD64;

/* Sets the override value and turns the override on. */
void func_00286F38(int value) {
    D_0069DD64 = value;
    D_0069DD60 = 1;
}

/* Turns the override off. */
void func_00286F48(void) {
    D_0069DD60 = 0;
}
