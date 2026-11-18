#ifndef INCLUDE_MAIN_TU110_H
#define INCLUDE_MAIN_TU110_H

#include "shared.h"

/* Functions of src/main/tu110.c that other files call. */

void *menuMainTxdEntry(int index);
void *menuTutorialTxdEntry(int index);
void ptRefreshCurrent(void);

/* The active map object (mapActiveObject, .sdata of tu110.c), also driven by tu114.c. */
typedef struct MapModeBlock {
    u8 unmodeled_0000[0x4084];
    int mode;
} MapModeBlock;

typedef struct MapObject {
    u8 unmodeled_000[0x330];
    void *handle;
    MapModeBlock *modeBlock;
    u8 unmodeled_338[0x570 - 0x338];
    u32 flags;
} MapObject;

#endif /* INCLUDE_MAIN_TU110_H */
