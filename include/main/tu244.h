#ifndef INCLUDE_MAIN_TU244_H
#define INCLUDE_MAIN_TU244_H

/* Functions of src/main/tu244.c that other files call. */

struct SizedRecord;

int sizedRecordCenterX(struct SizedRecord *record);
int sizedRecordCenterY(struct SizedRecord *record);
void sizedRecordSetValue(struct SizedRecord *record, int value);

#endif /* INCLUDE_MAIN_TU244_H */
