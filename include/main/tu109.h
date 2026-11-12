#ifndef INCLUDE_MAIN_TU109_H
#define INCLUDE_MAIN_TU109_H

/* Functions of src/main/tu109.c that other files call. */

void func_00229898(void);
void selectionBuildKeyTable(void);
char *selectionRecordSetValue(int record, int value);
char *selectionSetValue(int group, int slot, int sub, int value);
unsigned short selectionGetValue(int group, int slot, int sub);

#endif /* INCLUDE_MAIN_TU109_H */
