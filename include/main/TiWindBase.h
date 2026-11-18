#ifndef INCLUDE_MAIN_TIWINDBASE_H
#define INCLUDE_MAIN_TIWINDBASE_H

#include "shared.h"

/* Functions of src/main/TiWindBase.c that other files call. */

struct TiWindWorld;

void tiWindApplyAll(void *first_input, void *second_input);
void tiWindSolveCellPairs(struct TiWindWorld *world);
void tiWindDirSetEnabled(u32 index, int enabled);
void tiWindDirSetDirection(u32 index, float x, float y, float z);
void tiWindDirSetPosition(u32 index, const float *position);
void tiWindDirSetSpeed(u32 index, float speed);
void tiWindDirSetRange(u32 index, float upper, float lower);
void tiWindDirSetRadius(u32 index, float radius, int global);
void tiWindPointSetEnabled(u32 index, int enabled);
void tiWindPointSetPosition(u32 index, const float *position);
void tiWindPointSetSpeed(u32 index, float speed);
void tiWindPointSetRange(u32 index, float upper, float lower);
void tiWindPointSetRadius(u32 index, float radius, int global);
void tiWindClearTables(void);
void tiWindAdvanceAll(void);

#endif /* INCLUDE_MAIN_TIWINDBASE_H */
