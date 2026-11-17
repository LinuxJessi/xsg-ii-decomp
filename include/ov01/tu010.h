#ifndef INCLUDE_OV01_TU010_H
#define INCLUDE_OV01_TU010_H

#include "shared.h"

/* Bits of XenoActor.drawFlags (+0xBC): hidden, and the fade flag set while a fade runs. */
#define ACTOR_STATE_HIDDEN 0x4
#define ACTOR_STATE_FADING 0x10000
/* Default progress step of a fade (per frame, tu018 and tu021). */
#define FADE_DEFAULT_STEP 0.1f

typedef struct ObjectTask ObjectTask;

#endif /* INCLUDE_OV01_TU010_H */
