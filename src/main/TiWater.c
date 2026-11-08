/*
 * TiWater: the water scene (file ../TiWater.euc.c in the assert strings): the scene loop, the
 * debug parameter panel, the object registry (tiWaterSubCount count, tiWaterSubList list of TiWaterSub
 * tables) and the drawing. Names are proposals (see the proposal file).
 */
#include "common.h"
#include "TiWater.h"
#include "main/gifState.h"
#include "main/nml_model_set.h"

extern int tiWaterDebugMode;      /* debug panel mode, toggled by select in the scene loop */
extern int tiWaterDebugIndex;      /* selected debug parameter, 0..11 */
extern float tiWaterEffectNum;

/* Resets the debug panel mode and its selected parameter to zero. */
void tiWaterDebugReset(void) {
    tiWaterDebugMode = 0;
    tiWaterDebugIndex = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterDebugDrawParams);

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterDebugUpdate);

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterRegisterSub);

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterSendStateTemplate);

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterDrawItem);

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterDrawAll);

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterVectorAngle);

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterUpdateSource);

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterStepSub);

void tiWaterSetEffectMode(void) {
    func_00246AF8();
    func_00246DD8(0xFFFFFF);
    gifStateSetMode((int) tiWaterEffectNum);
}

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterSceneLoop);

void pxySetDrawCallback(void (*callback)(void));
void tiWaterRegisterSub(struct TiWaterSub *sub);
void tiWaterStepSub(struct TiWaterSub *sub);
extern void tiWaterDrawAll(void);

/* Per-object update: registers the sub-table when it has items, steps it when stepMode is 0,
 * and installs the draw callback tiWaterDrawAll. */
void tiWaterObjectUpdate(struct TiWaterObject *obj, int stepMode) {
    struct TiWaterSub *sub;

    sub = (struct TiWaterSub *) ((char *) obj + obj->sub_offset);
    if (sub->active != 0) {
        tiWaterRegisterSub(sub);
        if (stepMode == 0) {
            tiWaterStepSub(sub);
        }
        func_001BA658();
        pxySetDrawCallback(tiWaterDrawAll);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterInitStudio);

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterCameraUpdate);

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterCameraUpdateFine);
