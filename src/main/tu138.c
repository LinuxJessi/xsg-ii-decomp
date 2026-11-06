#include "common.h"
#include "main/tu138.h"

extern int cfeEntryCount;
extern unsigned char cfeEntries[];

/* Number of entries in cfeEntries; each entry is 0x1B0 bytes. */
int cfeGetEntryCount(void) {
    return cfeEntryCount;
}

/* Entry `index` of cfeEntries, or null when index is outside the count. */
void *cfeGetEntry(int index) {
    void *entry = 0;

    if (cfeEntryCount == 0 || (entry = cfeEntries + index * 0x1B0, !(index < cfeEntryCount))) {
        entry = 0;
    }
    return entry;
}

INCLUDE_ASM("asm/main/nonmatchings/tu138", func_00247570);

extern int cfeRangeMin;
extern int cfeRangeMax;
extern int cfeRangeValue;

/* Steps the value cfeRangeValue by delta and wraps it inside [cfeRangeMin, cfeRangeMax]:
   below the minimum it goes to the maximum, above the maximum to the minimum. */
void cfeRangeStepWrap(int delta) {
    int next = cfeRangeValue + delta;

    cfeRangeValue = next;
    if (next < cfeRangeMin) {
        cfeRangeValue = cfeRangeMax;
    }
    if (cfeRangeMax < cfeRangeValue) {
        cfeRangeValue = cfeRangeMin;
    }
}

/* Sets the value cfeRangeValue and clamps it into [cfeRangeMin, cfeRangeMax]. */
void cfeRangeSetValue(int value) {
    cfeRangeValue = value;
    if (value < cfeRangeMin) {
        cfeRangeValue = cfeRangeMin;
    }
    if (cfeRangeMax < cfeRangeValue) {
        cfeRangeValue = cfeRangeMax;
    }
}

/* Sets the minimum cfeRangeMin; the value is raised to it when below. */
void cfeRangeSetMin(int lower) {
    cfeRangeMin = lower;
    if (cfeRangeValue < lower) {
        cfeRangeValue = lower;
    }
}

/* Adds delta to the minimum cfeRangeMin, keeps it at or below the maximum, and raises the value to it. */
void cfeRangeAdjustMin(int delta) {
    int next = cfeRangeMin + delta;

    cfeRangeMin = next;
    if (cfeRangeMax < next) {
        cfeRangeMin = cfeRangeMax;
    }
    if (cfeRangeValue < cfeRangeMin) {
        cfeRangeValue = cfeRangeMin;
    }
}

/* Sets the maximum cfeRangeMax; the value is lowered to it when above. */
void cfeRangeSetMax(int upper) {
    cfeRangeMax = upper;
    if (upper < cfeRangeValue) {
        cfeRangeValue = upper;
    }
}

/* Adds delta to the maximum cfeRangeMax, keeps it at or above the minimum, and lowers the value to it. */
void cfeRangeAdjustMax(int delta) {
    int next = cfeRangeMax + delta;

    cfeRangeMax = next;
    if (next < cfeRangeMin) {
        cfeRangeMax = cfeRangeMin;
    }
    if (cfeRangeMax < cfeRangeValue) {
        cfeRangeValue = cfeRangeMax;
    }
}

int cfeRangeGetValue(void) {
    return cfeRangeValue;
}

int cfeRangeGetMin(void) {
    return cfeRangeMin;
}

int cfeRangeGetMax(void) {
    return cfeRangeMax;
}
