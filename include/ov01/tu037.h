#ifndef INCLUDE_OV01_TU037_H
#define INCLUDE_OV01_TU037_H

/* Functions of src/ov01/tu037.c that other files call. */

int ov01SceneFrameCount(void);
void ov01SceneRelease(void);
void battleStartBonusApply(void);
struct Ov01Object *unitSelectPlTarget(struct Ov01Object *keep);

#endif /* INCLUDE_OV01_TU037_H */
