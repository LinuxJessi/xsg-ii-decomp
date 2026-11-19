#ifndef INCLUDE_OV01_TU061_H
#define INCLUDE_OV01_TU061_H

/* Functions of src/ov01/tu061.c that other files call. */

struct Ov01Object;

/* Binds the unit to the timed sequence (or arms its tick, below MENU_TIMED_SEQ_MIN_HITS hits); called by dataTargetShow (tu022). */
void menuTimedSeqShow(struct Ov01Object *unit);
void menuTimedSeqArmTick(void);
void menuTimedSeqNew(void);
void menuTimedSeqStart(struct Ov01Object *unit);

#endif /* INCLUDE_OV01_TU061_H */
