#ifndef INCLUDE_OV01_TU032_H
#define INCLUDE_OV01_TU032_H

/* Functions of src/ov01/tu032.c that other files call. */

struct Ov01Owner;

int actLevelChanceRoll(struct Ov01Owner *owner, int index);
int actStepCounterValue(void);
void actStockClearBoost1(void);
void sdvInitSpecialWork(void);

#endif /* INCLUDE_OV01_TU032_H */
