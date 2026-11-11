#include "common.h"
#include "shared.h"

/* One 208-byte record of the 16-slot ladder table at ladderSlots; its first halfword is -1 when the slot is unused. */
typedef struct LadderSlot {
    short id;                               /* 0x00 */
    unsigned char unmodeled_02[2];
    int slotIndex;                          /* 0x04: the slot's own number, -1 by ladderInitTable */
    int key;                                /* 0x08: the key passed to uwamonoGetEntryPose, -1 when unused */
    unsigned char unmodeled_0c[4];
    float positionX;                     /* 0x10: set to 0.0f by ladderInitTable */
    float positionY;                            /* 0x14: height of the ladder foot */
    float positionZ;                     /* 0x18: set to 0.0f by ladderInitTable */
    float positionW;                     /* 0x1C: set to 1.0f by ladderInitTable */
    float angle;                            /* 0x20: facing angle of the ladder */
    float height;                           /* 0x24 */
    float rungs[32];                        /* 0x28: height of each rung above the foot */
    unsigned char rungExits[32];            /* 0xA8: RUNG_EXIT_* bits */
    short rungCount;                        /* 0xC8 */
    signed char footBlocked;                /* 0xCA: 1 the foot of the ladder cannot be left */
    unsigned char unmodeled_cb[5];
} LadderSlot;

enum {
    LADDER_SLOT_COUNT = 16,
    LADDER_SLOT_SIZE = 208,
    LADDER_MAX_RUNGS = 32
};

/* Bits of a rung's exits: the rung can be left to the left or to the right. */
enum {
    RUNG_EXIT_LEFT = 1,
    RUNG_EXIT_RIGHT = 2
};

extern LadderSlot ladderSlots[LADDER_SLOT_COUNT];
extern short uwamonoLadderState;

/* Clears every ladder slot to its unused state and selects slot 0. */
void ladderInitTable(void)
{
    int slotIndex;
    int rungIndex;
    LadderSlot *slot;

    uwamonoLadderState = 0;
    for (slotIndex = 0; slotIndex < LADDER_SLOT_COUNT; slotIndex++) {
        slot = &ladderSlots[slotIndex];
        slot->id = -1;
        slot->slotIndex = -1;
        slot->positionX = slot->positionY = slot->positionZ = 0.0f;
        slot->positionW = 1.0f;
        slot->height = slot->angle = 0.0f;
        slot->rungCount = 0;
        slot->footBlocked = 0;
        for (rungIndex = 0; rungIndex < LADDER_MAX_RUNGS; rungIndex++) {
            slot->rungs[rungIndex] = 0.0f;
            slot->rungExits[rungIndex] = 0;
        }
    }
}

/* Returns the index of the first slot whose id is id, or -1 when no slot holds it. */
int ladderFindSlot(short id)
{
    int slotIndex;
    LadderSlot *slot;

    slot = ladderSlots;
    for (slotIndex = 0; slotIndex < LADDER_SLOT_COUNT; slotIndex++) {
        if (slot->id == id) {
            return slotIndex;
        }
        slot += 1;
    }
    return -1;
}

/* Returns how many slots, counted from slot 0, are in use before the first unused one. */
int ladderCountUsedPrefix(void)
{
    int usedCount;
    LadderSlot *slot;

    usedCount = 0;
    if (ladderSlots[0].id != -1) {
        slot = ladderSlots;
        for (;;) {
            usedCount += 1;
            slot += 1;
            if (usedCount < LADDER_SLOT_COUNT && slot->id != -1) {
                continue;
            }
            break;
        }
    }
    return usedCount;
}

int func_A9F0A8(void);
int func_A9F0C0(int index, int selector);
int func_A9CC70(int index, int value);
float uwamonoGetEntryPose(int key, float *out);

/* Fills the slot of the ladder at the first unused position from the entry key and the foot data. */
int ladderSetLadder(int source)
{
    LadderSlot *slot;
    float *angleHeight;
    int usedCount;
    int ladderId;
    int entryKey;
    float height;
    int poseOffset;
    char *slotBytes;

    if (func_A9F0A8() != 3) {
        func_A9CC70(source, 2);
        return 0;
    }
    usedCount = ladderCountUsedPrefix();
    ladderId = func_A9F0C0(source, 1);
    entryKey = func_A9F0C0(source, 2);
    height = (float) func_A9F0C0(source, 3) / 100.0f;
    if (entryKey == -1) {
        ladderSlots[usedCount].id = 0;
        return 1;
    }
    slotBytes = (char *)ladderSlots;
    slot = &ladderSlots[usedCount];
    poseOffset = usedCount * LADDER_SLOT_SIZE + 32;
    angleHeight = (float *)(slotBytes + poseOffset);
    slot->slotIndex = usedCount;
    slot->id = ladderId;
    angleHeight[1] = height;
    slot->key = entryKey;
    *angleHeight = uwamonoGetEntryPose(entryKey, &ladderSlots[usedCount].positionX);
    return 1;
}

/* Adds a rung to the ladder of the entry key: its height and the exits it allows. */
int ladderSetEntrance(int source)
{
    int slotIndex;
    float rungHeight;
    int leftExit;
    int rightExit;
    int rungExitBits;
    int rungIndex;

    if (func_A9F0A8() != 4) {
        func_A9CC70(source, 2);
        return 0;
    }
    slotIndex = ladderFindSlot(func_A9F0C0(source, 1));
    rungHeight = (float) func_A9F0C0(source, 2) / 100.0f;
    leftExit = (func_A9F0C0(source, 3) == 1) ? RUNG_EXIT_LEFT : 0;
    rightExit = (func_A9F0C0(source, 4) == 1) ? RUNG_EXIT_RIGHT : 0;
    rungExitBits = leftExit | rightExit;
    rungIndex = ladderSlots[slotIndex].rungCount;
    ladderSlots[slotIndex].rungs[rungIndex] = rungHeight;
    ladderSlots[slotIndex].rungCount += 1;
    ladderSlots[slotIndex].rungExits[rungIndex] = rungExitBits;
    return 1;
}
