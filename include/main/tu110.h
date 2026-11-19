#ifndef INCLUDE_MAIN_TU110_H
#define INCLUDE_MAIN_TU110_H

#include "shared.h"

/* Functions of src/main/tu110.c that other files call. */

void *menuMainTxdEntry(int index);
void *menuTutorialTxdEntry(int index);
void ptRefreshCurrent(void);

/* ptStatusBlock (0x24 bytes) as ptResetStatusBlock resets it and ptRefresh fills
 * it: the ids of the three members on screen and of the leader; memberMask, a
 * copy of D_006A5A30.memberMask (one bit per member who joined the party);
 * presentMask, one bit per member of the group on screen (front slots 0-6,
 * or back slots 7-9 in split mode; ov01's tu072 edits it as the MEMBER row);
 * and the ptGetClothId cloth id of each of the seven members (ov01's tu072
 * edits them as cloth ids). */
typedef struct PartyStatusBlock {
    unsigned char memberIds[3];
    unsigned char leaderId;
    unsigned short memberMask;
    unsigned short presentMask;
    int clothIds[7];
} PartyStatusBlock;

PartyStatusBlock *ptGetStatusBlock(void);

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
