/*
 * OV01 original TU 38: 0x00a9a4b0..0x00a9af68 (23 functions)
 *
 * Battle step chain (each step sets its successor with ov01StepSetNext; a
 * wait step pushes ov01StepAwaitLoads as the fallback): ov01StepChainStart starts it;
 * func_00A9A6A0 -> ov01StepInit -> ov01StepStartBlockLoad -> ov01StepAwaitBlockLoad ->
 * ov01StepOpenHandles -> ov01StepBindAndShow -> ov01StepLoadPartyIds (party ids) -> ov01StepSpawnPartyMember
 * (spawn party member) -> ov01StepLoadPartyMember -> ov01StepAwaitPartyLoad -> ov01StepNextPartyMember
 * (next member) -> ov01StepStartEnemies -> ov01StepSpawnEnemy (spawn enemy) -> ov01StepLoadEnemy
 * -> ov01StepAwaitEnemyLoad -> ov01StepNextEnemy (next enemy or battle start) -> func_00A9AF68
 * (tu039).
 */
#include "common.h"
#include "main/gifState.h"
#include "main/tu173.h"
#include "ov01/tu005.h"
#include "ov01/tu008.h"
#include "ov01/tu009.h"
#include "ov01/tu019.h"
#include "ov01/tu020.h"
#include "ov01/tu038.h"
#include "ov01/tu046.h"
#include "ov01/tu049.h"
#include "ov01/tu070.h"

/*
 * Spawn record of the party list ov01PartyList and the enemy list ov01EnemyList.
 * The party list holds three records; monsTblGet returns the enemy list.
 * Only these five words are touched by this TU.
 */
typedef struct UnitSpawnRecord {
    int id;
    int spawnArgA;
    int spawnArgB;
    int subArgC;
    int subArgD;
} UnitSpawnRecord;

/* The unit object ov01CurUnit[0] holds; this TU reads only its sub-record pointer.
 * It may be the same object as tu017's UnitRecord (integrator to reconcile). */
typedef struct UnitObject {
    unsigned char unmodeled_00[0x144];
    struct UnitSub *sub; /* +0x144 */
} UnitObject;

typedef struct UnitSub {
    unsigned char unmodeled_00[0xF8];
    int subF8; /* +0xF8: written from the enemy record's subArgC */
    int subFC; /* +0xFC: written from the enemy record's subArgD */
} UnitSub;

extern int ov01LoadDone;
extern int ov01LoadOwner;
extern int ov01PartyIndex;
extern int ov01EnemyIndex;
extern UnitSpawnRecord *ov01PartyList;
extern UnitSpawnRecord *ov01EnemyList;
void func_00120360(void);
void gifStateEnableEffect(void);
void func_00A81D30(void);
void ov01StepSetNext(void *item, int next_step);
void ov01LoadBlockStart(void);
void ov01LoadRequest(int value);
void ov01StepLoadPartyMember(int context);
void ov01StepNextEnemy(int context);
void *dataStreamOpen(int count, int small_buffer);
void func_00A88B60(int first, int second);
void monsSlotShow(void *slot);
void func_00A9BCC8(int first, int *out);
void menuStatRecordsCreate(void);
unsigned char *ptGetStatusBlock(void);
UnitSpawnRecord *ov01PartyListGet(void);
void effectPosTableSet(int value);
int dataUnitLoad(int id);
void func_00A82E18(int id);
UnitObject *unitPlCreate(int id);
UnitObject *unitEnCreate(int id);
void unitLoadSnapshot(UnitObject *object, int value);
void unitSetAngleDegrees(void *object, int degrees);
void func_00A84518(UnitObject *object, int value);
void unitFadeIn(UnitObject *object);
void ov01StepStartBlockLoad(int context);
void ov01StepAwaitBlockLoad(int context);
void ov01StepOpenHandles(int context);
void ov01StepBindAndShow(int context);
void ov01StepLoadPartyIds(int context);
void ov01StepSpawnPartyMember(int context);
void ov01StepNextPartyMember(int context);
void ov01StepStartEnemies(int context);
void ov01StepSpawnEnemy(int context);
void ov01StepLoadEnemy(int context);
void ov01StepAwaitEnemyLoad(int context);
void ov01StepAwaitLoads(int context);

/* Load-completion callback: marks the load done (ov01LoadDone) after dataBattleSeRelease. */
void ov01LoadComplete(void) {
    dataBattleSeRelease();
    ov01LoadDone = 1;
    func_00120360();
}

/* Starts a 0x1000-byte load into D_00AC41A0 whose completion is ov01LoadComplete. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu038", ov01LoadBlockStart);

/* Completion callback of ov01LoadRequest on the second path: passes the owner
 * (ov01LoadOwner) to dataSlotUnload, then marks the load done. */
void ov01LoadCompleteForOwner(void) {
    dataSlotUnload(ov01LoadOwner);
    ov01LoadDone = 1;
    func_00120360();
}

/* Load request for an owner object: when handleStateGet reports the data present
 * the load is marked done at once; otherwise the owner is kept in ov01LoadOwner and
 * the 0x1000-byte load starts with completion ov01LoadCompleteForOwner. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu038", ov01LoadRequest);

/* Load-done flag (ov01LoadDone), polled by the wait steps. */
int ov01LoadIsDone(void) {
    return ov01LoadDone;
}

/* Step-chain start: a new save stack (ov01StackNew), the first step func_00A9A6A0,
 * and the scene done flag ov01SceneDone cleared. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu038", ov01StepChainStart);

/* Wait-and-pop step: once no CD read is pending and no load wait is pending, pops
 * the saved continuation (ov01StackPop, ov01StackPop). */
void ov01StepAwaitLoads(int context) {
    if ((ov01CdPendingReads() == 0) && (loadWaitPending() == 0)) {
        ov01StackPop(context);
    }
}

/* Scene data step: prepares the scene records (the failure paths of the original
 * spin forever), then continues to ov01StepInit. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu038", func_00A9A6A0);

/* Init step: enables the GIF resCmdEffect (gifStateEnableEffect), runs four init calls and
 * continues to ov01StepStartBlockLoad with ov01StepAwaitLoads as the fallback. */
void ov01StepInit(int context) {
    gifStateEnableEffect();
    dataFaceFileLoad();
    dataStatFileLoad();
    func_00A81D30();
    dataBattleSeLoad();
    ov01StepSetNext(context, ov01StepStartBlockLoad);
    ov01StackPush(context, ov01StepAwaitLoads);
}

/* Starts the block load (ov01LoadBlockStart) and continues to ov01StepAwaitBlockLoad. */
void ov01StepStartBlockLoad(int context) {
    ov01LoadBlockStart();
    ov01StepSetNext(context, ov01StepAwaitBlockLoad);
}

/* Waits for the block load, then continues to ov01StepOpenHandles. */
void ov01StepAwaitBlockLoad(int context) {
    if (ov01LoadIsDone() != 0) {
        ov01StepSetNext(context, ov01StepOpenHandles);
    }
}

/* Opens two handles (D_00AB5CD4, ov01StreamBuffer) and continues to ov01StepBindAndShow. */
void ov01StepOpenHandles(int context) {
    extern int ov01StreamBuffer[];
    extern int D_00AB5CD4[];
    int out_value;

    menuTexFlushTaskStart();
    seVoiceStartTagged(2, 4);
    D_00AB5CD4[0] = monsSlotCreate();
    ov01StreamBuffer[0] = dataStreamOpen(monsTblValueGet(), 0);
    out_value = 0;
    func_00A9BCC8(6, &out_value);
    ov01StepSetNext(context, ov01StepBindAndShow);
    ov01StackPush(context, ov01StepAwaitLoads);
}

/* Binds the two handles, then continues to ov01StepLoadPartyIds. */
void ov01StepBindAndShow(int context) {
    extern int ov01StreamBuffer;
    extern int D_00AB5CD4;

    func_00A88B60(D_00AB5CD4, ov01StreamBuffer);
    monsSlotShow(D_00AB5CD4);
    nmlModelEnableSubWindow0();
    ov01StepSetNext(context, ov01StepLoadPartyIds);
}

/* Reads the three party ids (bytes of ptGetStatusBlock) into ov01PartyList, runs the
 * party setup (posDataGet, effectPosTableSet) and continues to ov01StepSpawnPartyMember. */
void ov01StepLoadPartyIds(int context) {
    int source_index;
    int entry_index;
    unsigned char *src;
    unsigned char byte;
    int result;

    source_index = 0;
    entry_index = 0;
    ov01PartyIndex = 0;
    ov01PartyList = ov01PartyListGet();
    do {
        src = ptGetStatusBlock() + source_index;
        source_index += 1;
        byte = *src;
        ov01PartyList[entry_index].id = byte;
        entry_index += 1;
    } while (source_index < 3);
    result = posDataGet();
    effectPosTableSet(result);
    ov01StepSetNext(context, ov01StepSpawnPartyMember);
}

/* Spawns party member ov01PartyIndex with unitPlCreate (unitPlCreate) and sets its
 * fields; a zero id skips to ov01StepNextPartyMember. Continues to ov01StepLoadPartyMember. */
void ov01StepSpawnPartyMember(int context) {
    extern UnitObject *ov01CurUnit[];
    extern int D_00AB5CD8[];
    int id;
    UnitSub *sub;
    UnitObject *object;

    id = ov01PartyList[ov01PartyIndex].id;
    if (id == 0) {
        ov01StepSetNext(context, ov01StepNextPartyMember);
        return;
    }
    object = unitPlCreate(id);
    ov01CurUnit[0] = object;
    unitLoadSnapshot(object, ov01PartyList[ov01PartyIndex].spawnArgA);
    unitSetAngleDegrees(ov01CurUnit[0], ov01PartyList[ov01PartyIndex].spawnArgB);
    sub = ov01CurUnit[0]->sub;
    sub->subF8 = 0;
    sub->subFC = 0;
    D_00AB5CD8[0] = dataUnitLoad(id);
    func_00A82E18(id);
    ov01StepSetNext(context, ov01StepLoadPartyMember);
    ov01StackPush(context, ov01StepAwaitLoads);
}

/* Binds the spawned party member and starts its load (ov01LoadRequest); continues
 * to ov01StepAwaitPartyLoad. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu038", ov01StepLoadPartyMember);

/* Waits for the party member's load, then continues to ov01StepNextPartyMember. */
void ov01StepAwaitPartyLoad(int context) {
    if (ov01LoadIsDone() != 0) {
        ov01StepSetNext(context, ov01StepNextPartyMember);
    }
}

/* Next party member: ov01PartyIndex counts up; below three it goes back to
 * ov01StepSpawnPartyMember, else menuStatRecordsCreate runs and the enemy step starts (ov01StepStartEnemies). */
void ov01StepNextPartyMember(int context) {
    int count;
    int more;

    count = ov01PartyIndex + 1;
    more = count < 3;
    ov01PartyIndex = count;
    if (more != 0) {
        ov01StepSetNext(context, ov01StepSpawnPartyMember);
        return;
    }
    menuStatRecordsCreate();
    ov01StepSetNext(context, ov01StepStartEnemies);
}

/* Enemy list start: resets ov01EnemyIndex and takes the enemy list from monsTblGet. */
void ov01StepStartEnemies(int context) {
    ov01EnemyIndex = 0;
    ov01EnemyList = monsTblGet();
    ov01StepSetNext(context, ov01StepSpawnEnemy);
}

/* Spawns enemy ov01EnemyIndex with unitEnCreate (unitEnCreate) and sets its fields and
 * sub record; continues to ov01StepLoadEnemy. */
void ov01StepSpawnEnemy(int context) {
    extern UnitObject *ov01CurUnit[];
    extern int D_00AB5CD8[];
    int id;
    UnitObject *object;
    UnitSub *sub;

    id = ov01EnemyList[ov01EnemyIndex].id;
    object = unitEnCreate(id);
    ov01CurUnit[0] = object;
    unitLoadSnapshot(object, ov01EnemyList[ov01EnemyIndex].spawnArgA);
    unitSetAngleDegrees(ov01CurUnit[0], ov01EnemyList[ov01EnemyIndex].spawnArgB);
    sub = ov01CurUnit[0]->sub;
    sub->subF8 = ov01EnemyList[ov01EnemyIndex].subArgC;
    sub->subFC = ov01EnemyList[ov01EnemyIndex].subArgD;
    D_00AB5CD8[0] = dataUnitLoad(id);
    func_00A82E18(id);
    ov01StepSetNext(context, ov01StepLoadEnemy);
    ov01StackPush(context, ov01StepAwaitLoads);
}

/* Binds the enemy and starts its load (ov01LoadRequest); continues to ov01StepAwaitEnemyLoad. */
void ov01StepLoadEnemy(int context) {
    extern UnitObject *ov01CurUnit;
    extern int D_00AB5CD8;

    func_00A84518(ov01CurUnit, D_00AB5CD8);
    unitFadeIn(ov01CurUnit);
    ov01LoadRequest(D_00AB5CD8);
    ov01StepSetNext(context, ov01StepAwaitEnemyLoad);
}

/* Waits for the enemy's load, then continues to ov01StepNextEnemy. */
void ov01StepAwaitEnemyLoad(int context) {
    if (ov01LoadIsDone() != 0) {
        ov01StepSetNext(context, ov01StepNextEnemy);
    }
}

/* Next enemy: while the enemy list has an entry at ov01EnemyIndex it returns to
 * ov01StepSpawnEnemy; past the end it sets up the battle (menus, objects, func_00A9A2F8,
 * unitSelectPlTarget) and continues to func_00A9AF68. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu038", ov01StepNextEnemy);
