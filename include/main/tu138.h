#ifndef INCLUDE_MAIN_TU138_H
#define INCLUDE_MAIN_TU138_H

/* Functions of src/main/tu138.c that other files call. */

void *cfeGetEntry(int index);
void cfeRangeSetMax(int upper);
void cfeRangeSetMin(int lower);
void cfeRangeSetValue(int value);
int cfeRangeGetValue(void);
int cfeRangeGetMin(void);

#endif /* INCLUDE_MAIN_TU138_H */
