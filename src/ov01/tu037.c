/*
 * OV01 original TU 37: 0x00a99ec0..0x00a9a4b0 (11 functions)
 */
#include "common.h"
#include "main/TiWindBase.h"
#include "main/pad.h"
#include "main/face_point.h"
#include "main/tu051.h"
#include "main/tu092.h"
#include "main/tu093.h"
#include "main/tu123.h"
#include "main/tu124.h"
#include "main/tu125.h"
#include "main/tu156.h"
#include "main/xenoActor.h"
#include "main/xgl_2.h"
#include "ov01/tu005.h"
#include "ov01/tu008.h"
#include "ov01/tu010.h"
#include "ov01/tu017.h"
#include "ov01/tu019.h"
#include "ov01/tu022.h"
#include "ov01/tu023.h"
#include "ov01/tu033.h"
#include "ov01/tu035.h"
#include "ov01/tu040.h"
#include "ov01/tu037.h"
#include "ov01/tu042.h"
#include "ov01/tu044.h"
#include "ov01/tu047.h"
#include "ov01/tu049.h"
#include "ov01/tu075.h"

/* ov01 0x00AA8EA0, in a part of the overlay that has no C file yet. */
void handleTableInit(void);
/* main xgl_2 (no header prototype: nml_model_set passes its own light block type). */
void xglLightSetDefault(XglLightSet *lightSet);

/* Frames run by the scene loop (counted by ov01SceneLoop). */
extern int ov01SceneFrames;

/* Scene init: runs the reset chain of the battle systems, makes the light block the default light set of xgl and
 * tu156, resets the menus and text systems, clears the frame counter ov01SceneFrames and sets the draw hook
 * (sRender.fade_callback) to ov01DrawAll. The light block is the 0x180-byte light set (see initLight, tu047). */
void ov01SceneInit(void)
{
    ov01ResetAll();
    dataFileInit();
    battleWorkTablesReset();
    func_00A9B0F8();
    monsSlotReset();
    unitInit();
    dataVPadReset();
    msgReset();
    cmdSystemInit();
    tgtSelectInit();
    dataStateReset();
    menuPanelRowsReset();
    handleTableInit();
    ov01SceneStateReset();
    cameraInit();
    xglLightSetDefault((XglLightSet *)lightBlockGet());
    func_00253048((XglLightSet *)lightBlockGet());
    choiceMenuResetAll();
    textEntryPoolReset();
    textBlockClear();
    textWindowReset();
    tiWindClearTables();
    ov01SceneFrames = 0;
    sRender.fade_callback = ov01DrawAll;
}

void ov01StepChainStart(void);

/* Scene entry: culling ignore on, scene init, then the battle step chain. */
void ov01SceneEnter(void) {
    xglCullingIgnore();
    ov01SceneInit();
    ov01StepChainStart();
}

/*
 * Scene frame loop: each pass counts ov01SceneFrames, runs the update and draw
 * passes (ov01UpdateAll, the actors, the text and message systems) and ends
 * when the scene done flag (ov01SceneIsDone) is set or a pad combination asks
 * for the exit (func_00A9A100).
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu037", ov01SceneLoop);

INCLUDE_ASM("asm/nonmatchings/ov01/tu037", func_00A9A100);

/* Wait hook polled by the frame loop; it never reports a wait. */
int func_00A9A138(void) {
    return 0;
}

/* Scene end: release the scene, reset the tables, clear the draw hook (sRender.fade_callback). */
void ov01SceneEnd(void) {
    ov01SceneRelease();
    ov01ResetAll();
    sRender.fade_callback = 0;
    xglCdArenaClose();
}

/* Scene release: culling ignore off, counters cleared, records of the scene key
 * released (sbThreadsRelease), actors released, tables cleared. */
void partySnapshotBuild(void);

void ov01SceneRelease(void) {
    xglCullingIgnoreOff();
    dataCountersClear();
    sbThreadsRelease(ov01ScriptKeyGet());
    XenoActorReleaseAll();
    ov01SceneSystemsShutdown();
    partySnapshotBuild();
    ov01ClearTables();
}

/* Frames run by the scene loop (ov01SceneFrames, counted by ov01SceneLoop). */
int ov01SceneFrameCount(void) {
    return ov01SceneFrames;
}

extern int ov01SceneDone;

/* Scene done flag, set by the last step ov01StepFinish. */
int ov01SceneIsDone(void) {
    return ov01SceneDone;
}

/* Keeps the given unit when it is still valid; otherwise a random valid unit of
 * the player side (unitTblGet side 0) is returned, or 0 when there is none. */
/* Player-side party slots (unitTblGet side 0 returns 3; one spare). */
#define UNIT_SELECT_SLOT_COUNT 4
/* Entry indices below this are player characters. */
#define UNIT_SELECT_PL_INDEX_LIMIT 8

StatOwner *unitSelectPlTarget(StatOwner *keep)
{
    StatOwner **table;
    StatOwner *picked[UNIT_SELECT_SLOT_COUNT];
    int count;
    int i;
    int pickedCount;
    int idx;

    if (keep != 0) {
        if (unitCanAct(keep) != 0) {
            if (keep->stats->key < UNIT_SELECT_PL_INDEX_LIMIT) {
                return keep;
            }
        }
    }

    if (unitNumFlag4ClearStatZeroGet(0) == 0) {
        return 0;
    }

    i = 0;
    count = unitTblGet(0, &table);
    pickedCount = 0;
    if (count > 0) {
        for (; i < count; i++) {
            if (table[i] != 0 && unitCanAct(table[i]) != 0
                && table[i]->stats->key < UNIT_SELECT_PL_INDEX_LIMIT) {
                picked[pickedCount] = table[i];
                pickedCount++;
            }
        }
    }

    if (pickedCount == 0) {
        return 0;
    }

    idx = rnd(pickedCount - 1);
    return picked[idx];
}

/* Once-per-battle setup of the unit state; still unidentified (see the naming notes). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu037", func_00A9A2F8);
