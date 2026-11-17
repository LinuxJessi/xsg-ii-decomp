#ifndef INCLUDE_MAIN_TU243_H
#define INCLUDE_MAIN_TU243_H

/* Functions of src/main/tu243.c that other files call. */

int deltaPerStep(int start, int target, int divisor);
int easeAngleStep(int start, int target);
void easeStepOverrideSet(int value);
void easeStepOverrideClear(void);

#endif /* INCLUDE_MAIN_TU243_H */
