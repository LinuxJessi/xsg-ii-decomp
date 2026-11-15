#ifndef INCLUDE_MAIN_TU145_H
#define INCLUDE_MAIN_TU145_H

/* Functions of src/main/tu145.c that other files call. */

#include "main/tu144.h"

void func_0024FC90(CfeTrackNode *track);
void cfeChainUnwrapAngles(CfeTrackNode *node);
float cfeTrackSample(CfeTrackNode *track, int time);
float func_002508C8(CfeTrackNode *track, int time);

#endif /* INCLUDE_MAIN_TU145_H */
