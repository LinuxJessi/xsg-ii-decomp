#ifndef INCLUDE_OV01_TU061_H
#define INCLUDE_OV01_TU061_H

/* Functions of src/ov01/tu061.c that other files call. */

struct Ov01Object;

void menuTimedSeqArmTick(void);
void menuTimedSeqNew(void);
void menuTimedSeqStart(struct Ov01Object *unit);

#endif /* INCLUDE_OV01_TU061_H */
