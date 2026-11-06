/*
 * OV01 original TU 37: 0x00a99ec0..0x00a9a4b0 (11 functions)
 */
#include "common.h"
#include "main/face_point.h"
#include "main/xenoActor.h"
#include "ov01/tu005.h"
#include "ov01/tu022.h"
#include "ov01/tu037.h"
#include "ov01/tu044.h"
#include "ov01/tu075.h"

/* Scene init: resets the tables and the frame counter ov01SceneFrames, sets the draw
 * hook 0x645864 to ov01DrawAll and runs the reset chain (unitInit, dataStateReset,
 * tiWindClearTables and others). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu037", ov01SceneInit);

void ov01SceneInit(void);
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

/* Scene end: release the scene, reset the tables, clear the draw hook 0x645864. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu037", ov01SceneEnd);

/* Scene release: culling ignore off, counters cleared, records of the scene key
 * released (sbThreadsRelease), actors released, tables cleared. */
void partySnapshotBuild(void);
int func_00A9B100(void);

void ov01SceneRelease(void) {
    xglCullingIgnoreOff();
    dataCountersClear();
    sbThreadsRelease(func_00A9B100());
    XenoActorReleaseAll();
    ov01SceneSystemsShutdown();
    partySnapshotBuild();
    ov01ClearTables();
}

extern int ov01SceneFrames;

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
INCLUDE_ASM("asm/nonmatchings/ov01/tu037", unitSelectPlTarget);

/* Once-per-battle setup of the unit state; still unidentified (see the naming notes). */
INCLUDE_ASM("asm/nonmatchings/ov01/tu037", func_00A9A2F8);
