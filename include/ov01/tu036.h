#ifndef INCLUDE_OV01_TU036_H
#define INCLUDE_OV01_TU036_H

/* Functions of src/ov01/tu036.c that other files call. */

int cfEventGet(void);
void cfEventSet(int cfEvent);
short cfMonsSetNoGet(void);
void cfMonsSetNoSet(int monsSetNo);
int gattaiCodeGet(void);
struct Ov01Object;

/*
 * The battle state block battleStateGet returns, the same block as tgtSelectState (src/ov01/tu035.c
 * models the target selection words of it). Only the words the other files read are named: current
 * (+0x04) is the object under the cursor, object/first/second/third (+0x38..+0x44) are the unit and
 * the three values unitCmdBatchRun passes on, transition (+0x48) is set by unitCmdSetBattleTransition,
 * moveCmd (+0x50) by setMoveCmd, slotIndex (+0x58) is the index battleEntryActivate and
 * unitRecreate use and scale (+0x5C) the angle unitRecreate restores.
 */
struct BattleState {
    unsigned char unmodeled_00[4];
    void *current;                          /* +0x04 */
    unsigned char unmodeled_08[0x38 - 8];
    struct Ov01Object *object;               /* +0x38 */
    int first;                              /* +0x3C */
    int second;                             /* +0x40 */
    int third;                              /* +0x44 */
    int transition;                         /* +0x48: 1 when the motion mode is below 16, else 2 */
    unsigned char unmodeled_4c[4];
    int moveCmd;                            /* +0x50 */
    unsigned char unmodeled_54[4];
    int slotIndex;                          /* +0x58 */
    float scale;                            /* +0x5C */
};

struct BattleState *battleStateGet(void);

short tableKeyEntryGet(struct Ov01Object *owner, int index, void *target);
void gattaiCodeSet(int code);
void thinkNoSet(int value);

#endif /* INCLUDE_OV01_TU036_H */
