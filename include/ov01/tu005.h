#ifndef INCLUDE_OV01_TU005_H
#define INCLUDE_OV01_TU005_H

/* Functions of src/ov01/tu005.c that other files call. */

struct DataLookup;
struct MatrixBlock;
struct SaveStack;

void dataLookupReset(struct DataLookup *lookup);
void dataObjRelease(void *object);
void ov01MatrixBlockInit(struct MatrixBlock *mat);
void ov01ClearTables(void);
void ov01ResetAll(void);
int ov01RollPercent(int percent);
void ov01StackPop(struct SaveStack *stack);

#endif /* INCLUDE_OV01_TU005_H */
