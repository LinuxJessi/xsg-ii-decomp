#ifndef INCLUDE_MAIN_TIWINDBASE_H
#define INCLUDE_MAIN_TIWINDBASE_H

/* Functions of src/main/TiWindBase.c that other files call. */

struct TiWindWorld;

void tiWindApplyAll(void *first_input, void *second_input);
void tiWindSolveCellPairs(struct TiWindWorld *world);
void tiWindPointSetEnabled(unsigned int index, int enabled);
void tiWindClearTables(void);

#endif /* INCLUDE_MAIN_TIWINDBASE_H */
