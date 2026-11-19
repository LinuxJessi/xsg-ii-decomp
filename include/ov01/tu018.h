#ifndef INCLUDE_OV01_TU018_H
#define INCLUDE_OV01_TU018_H

/* Functions of src/ov01/tu018.c that other files call. */

struct Ov01Object;
struct FadeTask;

struct FadeTask *unitSlotFadeIn(struct Ov01Object *unit, int index);
struct FadeTask *unitSlotFadeOut(struct Ov01Object *unit, int index);

#endif /* INCLUDE_OV01_TU018_H */
