/*
 * TiWater: the water scene (file ../TiWater.euc.c in the assert strings): the scene loop, the
 * debug parameter panel, the object registry (tiWaterSubCount count, tiWaterSubList list of TiWaterSub
 * tables) and the drawing. Names are proposals (see the proposal file).
 */
#include "common.h"
#include "TiWater.h"
#include "main/gifState.h"
#include "main/nml_model_set.h"
#include "gs.h"
#include "main/tu093.h"

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

typedef struct XglPacket XglPacket;
XglPacket *xglPacketGetCurrent(void);
void sceVif1PkCnt(XglPacket *packet, int flag);
void sceVif1PkOpenDirectHLCode(XglPacket *packet, int flag);
void sceVif1PkAddDataN(XglPacket *packet, void *data, int count);
void sceVif1PkCloseDirectHLCode(XglPacket *packet);
extern u64 D_00631220[];

/* Sends the water state template: FRAME_1 at the frame buffer and TEX0_1 on the display buffer
 * (512 x 512, with alpha) are patched in, then the template (count 0x3C) goes out as direct data. */
void tiWaterSendStateTemplate(void)
{
    XglPacket *packet = xglPacketGetCurrent();

    D_00631220[0x60 / 8] = GS_SET_FRAME(sRender.frame_base, 8, 0, 0);
    D_00631220[0x70 / 8] = GS_SET_TEX0(sRender.display_base << 5, 8, 0, 9, 9, 1, 0, 0, 0, 0, 0, 0);
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDataN(packet, D_00631220, 0x3C);
    sceVif1PkCloseDirectHLCode(packet);
}

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterDrawItem);

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterDrawAll);

float func_002A1790(float value);
extern float tiWaterTwoPi;

/* The angle of (x, y) in [0, 2 pi): atan(y / x) moved into the right half turn. */
float tiWaterVectorAngle(float x, float y)
{
    float angle;

    if (x == 0.0f) {
        x = 1e-05f;
    }
    angle = func_002A1790(y / x);
    if (x > 0.0f) {
        if (angle < 0.0f) {
            angle += tiWaterTwoPi;
        }
    } else {
        angle += 3.1415927f;
    }
    return angle;
}

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
