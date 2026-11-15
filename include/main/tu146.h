#ifndef INCLUDE_MAIN_TU146_H
#define INCLUDE_MAIN_TU146_H

/* Functions of src/main/tu146.c that other files call. */

#include "main/tu144.h"

CfeTrackNode *cfeListTail(CfeTrackNode *node);
void cfeNodePoolSet(CfeTrackNode *records);
void cfeNodePoolClear(void);
CfeTrackNode *cfeNodeAlloc(void);
void cfeNodeRelease(CfeTrackNode *record);

#endif /* INCLUDE_MAIN_TU146_H */
