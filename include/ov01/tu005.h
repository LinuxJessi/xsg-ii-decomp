#ifndef INCLUDE_OV01_TU005_H
#define INCLUDE_OV01_TU005_H

/* Functions of src/ov01/tu005.c that other files call, and the lookup block they share. */

#define DATA_LOOKUP_CAPACITY 12 /* items in a DataLookup block */

struct MatrixBlock;
struct SaveStack;

/* One 0x18-byte item of a lookup block. nextStep is the successor step ov01StepSetNext stores; dataLookupAppend clears
 * it (an int store, so the item is a union there). */
struct DataLookupItem {
    int command; /* +0x00: the command word of a queued record (unitRunCommands dispatches on it); tu022's match key reads it back */
    union {
        void (*nextStep)(int context);
        int word;
    } next;
    int unmodeled_08[4];
};

/* Lookup block (reached through DataTarget.lookup in tu022, and as the command queue at +0x10 of an Ov01Object in tu012):
 * up to DATA_LOOKUP_CAPACITY 0x18-byte items, the read cursor and the index of the last item (-1 when empty). */
struct DataLookup {
    struct DataLookupItem items[DATA_LOOKUP_CAPACITY];
    int cursor;
    int lastIndex;
    int flag;
};

/* Stores the 0x18-byte record at the next item and returns the item, or 0 when the block is full. */
struct DataLookupItem *dataLookupAppend(struct DataLookup *lookup, void *record);
void dataLookupReset(struct DataLookup *lookup);
void *dataLookupCurrent(struct DataLookup *lookup);
void *dataLookupNext(struct DataLookup *lookup);
void dataObjRelease(void *object);
void ov01MatrixBlockInit(struct MatrixBlock *mat);
void ov01ClearTables(void);
void ov01ResetAll(void);
int ov01RollPercent(int percent);
struct SaveStack *ov01StackNew(void);
void ov01StackPop(struct SaveStack *stack);

#endif /* INCLUDE_OV01_TU005_H */
