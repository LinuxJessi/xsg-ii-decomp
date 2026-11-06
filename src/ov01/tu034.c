/*
 * OV01 original TU 34: 0x00a97110..0x00a97180 (3 functions)
 */
#include "common.h"
#include "ov01/tu032.h"

/* The event record the event drain (eventQueueRun, ov01/tu033) passes to a handler: the kind word is at +0
 * (the drain indexes the handler table by it), the payload follows. Kinds 19 and 20 use count and value. */
typedef struct EventCounterRecord {
    unsigned char unmodeled_00[4];
    int count;
    unsigned char unmodeled_08[8];
    int value;
} EventCounterRecord;

void unitValueRefresh(void *obj);

int func_00A97110(void) {
    return 1;
}

/* Kind-19 handler: reads record->value, clears the second action-stock boost slot (actStockClearBoost1) and
 * refreshes the unit value when it differs from record->count. Always returns 1, so the drain goes on. */
int eventActStockBoostClear(EventCounterRecord *record) {
    int value;

    value = record->value;
    actStockClearBoost1();
    if (value != record->count) {
        unitValueRefresh(value);
    }
    return 1;
}

/* Kind-20 handler, a countdown wait: returns 1 once record->count has run out, otherwise steps the count down by
 * one and returns 0, which stops the drain at this record until a later call. */
int eventWaitCountdown(EventCounterRecord *record) {
    if (record->count == 0) {
        return 1;
    }
    record->count = record->count - 1;
    return 0;
}
