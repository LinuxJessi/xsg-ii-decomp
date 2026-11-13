/*
 * TiWater: the water scene (file ../TiWater.euc.c in the assert strings): the scene loop, the
 * debug parameter panel, the object registry (tiWaterSubCount count, tiWaterSubList list of TiWaterSub
 * tables) and the drawing. Names are proposals (see the proposal file).
 */
#include "common.h"
#include "TiWater.h"
#include "main/gifState.h"
#include "main/nml_model_set.h"
#include "main/pad.h"
#include "main/xgl_font.h"
#include "gs.h"
#include "main/tu093.h"
#include "main/tu045.h"

extern int tiWaterDebugMode;      /* debug panel mode, toggled by select in the scene loop */
extern int tiWaterDebugIndex;      /* selected debug parameter, 0..11 */
extern float tiWaterEffectNum;
extern unsigned int tiWaterSubCount;
extern struct TiWaterSub *tiWaterSubList[];

/* Resets the debug panel mode and its selected parameter to zero. */
void tiWaterDebugReset(void) {
    tiWaterDebugMode = 0;
    tiWaterDebugIndex = 0;
}

extern PadRecord PadData[];
extern const char D_0068C860[];
extern const char D_0068C878[];
extern const char *tiWaterDebugLabels[];
extern float *tiWaterDebugValues[];
extern float tiWaterDebugSteps[];

/* Number of debug parameters the panel cycles through. */
#define TI_WATER_DEBUG_PARAMS 12
/* Parameters shown at once, and the line height. */
#define TI_WATER_DEBUG_LINES 3
#define TI_WATER_DEBUG_LINE_HEIGHT 9

/* Draws the debug panel (the selected parameter and the next two, the selected one marked) and
 * changes the selected value by its step: triangle adds, cross subtracts, held while select is
 * held and repeated otherwise. */
void tiWaterDebugDrawParams(void)
{
    unsigned int i;
    unsigned int line;
    unsigned short buttons;

    line = 0;
    xglFontDebugPrintf(0, 0, D_0068C860, tiWaterDebugIndex + 1, TI_WATER_DEBUG_PARAMS);
    i = tiWaterDebugIndex;
    for (; line < TI_WATER_DEBUG_LINES && i < TI_WATER_DEBUG_PARAMS; i++, line++) {
        if (tiWaterDebugIndex == i) {
            xglFontDebugPrintf(0, (line + 1) * TI_WATER_DEBUG_LINE_HEIGHT, D_0068C878);
        }
        xglFontDebugPrintf(10, (line + 1) * TI_WATER_DEBUG_LINE_HEIGHT, tiWaterDebugLabels[i], *tiWaterDebugValues[i]);
    }
    buttons = PadData[0].held;
    if (!(buttons & PAD_SELECT)) {
        buttons = PadData[0].repeated;
    }
    if (buttons & PAD_TRIANGLE) {
        *tiWaterDebugValues[tiWaterDebugIndex] += tiWaterDebugSteps[tiWaterDebugIndex];
    }
    if (buttons & PAD_CROSS) {
        *tiWaterDebugValues[tiWaterDebugIndex] -= tiWaterDebugSteps[tiWaterDebugIndex];
    }
}

extern float tiWaterCx;
extern float tiWaterCy;
extern float tiWaterAlphaType;
extern float tiWaterAlpha;
extern float tiWaterDecrease;
extern float tiWaterFbp;
extern float tiWaterBlur;
extern float tiWaterIntVals[4];
void tiWaterDebugDrawParams(void);
/* Debug panel step: up and down on pad 0 select the parameter (wrapping), the panel is drawn and
 * every parameter is applied to the GIF state and the four model sub windows. */
void tiWaterDebugUpdate(void)
{
    if (PadData[0].pressed & PAD_UP) {
        if (--tiWaterDebugIndex < 0) {
            tiWaterDebugIndex = TI_WATER_DEBUG_PARAMS - 1;
        }
    }
    if (PadData[0].pressed & PAD_DOWN) {
        if (++tiWaterDebugIndex >= TI_WATER_DEBUG_PARAMS) {
            tiWaterDebugIndex = 0;
        }
    }
    tiWaterDebugDrawParams();
    gifStateSetCenter(tiWaterCx, tiWaterCy);
    gifStateSetEffectAlpha(tiWaterAlphaType, tiWaterAlpha);
    gifStateSetDecrease(tiWaterDecrease);
    gifStateSetFbp(tiWaterFbp);
    gifStateSetBlur(tiWaterBlur);
    nmlModelUseSubWindow(0, (int) tiWaterIntVals[0]);
    nmlModelUseSubWindow(1, (int) tiWaterIntVals[1]);
    nmlModelUseSubWindow(2, (int) tiWaterIntVals[2]);
    nmlModelUseSubWindow(3, (int) tiWaterIntVals[3]);
}

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

void gifStateReset(void);
void gifStateFlush(void);
void gifStateAddReg(int gifAddress, u64 gifData);
void xglPacketInitVuStateArg(int mode);
void tiWaterDrawItem(void *item);
/* Draw callback: in pass 0, draws every registered table (CLAMP_1 and the state template first when
 * it has items, then each item) and empties the list; the packet state is reset afterwards. */
void tiWaterDrawAll(int pass)
{
    XglPacket *packet = xglPacketGetCurrent();
    unsigned int i;
    unsigned int j;
    char *item;

    if (pass != 0) {
        return;
    }
    for (i = 0; i < tiWaterSubCount; i++) {
        item = (char *) tiWaterSubList[i] + 0x10;
        gifStateReset();
        if (tiWaterSubList[i]->active != 0) {
            sceVif1PkCnt(packet, 0);
            sceVif1PkOpenDirectHLCode(packet, 0);
            gifStateAddReg(GS_CLAMP_1, GS_SET_CLAMP(0, 0, 0xFF, 0xFF, 0, 0));
            gifStateFlush();
            sceVif1PkCloseDirectHLCode(packet);
            tiWaterSendStateTemplate();
        }
        for (j = 0; j < tiWaterSubList[i]->active; j++) {
            tiWaterDrawItem(item);
            item += 0x110;
        }
    }
    tiWaterSubCount = 0;
    xglPacketModeReset();
    xglPacketInitVuStateArg(pass);
    func_001CD134();
    func_001CD1AC();
    func_001CD160();
}

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

void pxySetDrawCallback(void (*callback)(int pass));
int tiWaterRegisterSub(struct TiWaterSub *sub);
void tiWaterStepSub(struct TiWaterSub *sub);
void tiWaterDrawAll(int pass);

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
