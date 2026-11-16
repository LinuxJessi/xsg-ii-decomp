/*
 * TiWater: the water scene (file ../TiWater.euc.c in the assert strings): the scene loop, the
 * debug parameter panel, the object registry (tiWaterSubCount count, tiWaterSubList list of TiWaterSub
 * tables) and the drawing. Names are proposals (see the proposal file).
 */
#include "common.h"
#include "TiWater.h"
#include "gs.h"
#include "main/control_entry.h"
#include "main/gifState.h"
#include "main/nml_model_set.h"
#include "main/pad.h"
#include "main/sdv_033.h"
#include "main/test_129.h"
#include "main/tu026.h"
#include "main/tu045.h"
#include "main/tu051.h"
#include "main/tu093.h"
#include "main/xenoActor.h"
#include "main/xgl_font.h"
#include "main/xgl_studio.h"
#include "main/xgl_render.h"

extern int tiWaterDebugMode;      /* debug panel mode, toggled by select in the scene loop */
extern int tiWaterDebugIndex;      /* selected debug parameter, 0..11 */
extern float tiWaterEffectNum;
extern int tiWaterSubCount;
extern struct TiWaterSub *tiWaterSubList[];

/* Resets the debug panel mode and its selected parameter to zero. */
void tiWaterDebugReset(void) {
    tiWaterDebugMode = 0;
    tiWaterDebugIndex = 0;
}

extern PadRecord PadData[];
extern const char tiWaterMenuFormat[];
extern const char tiWaterMenuMarker[];
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
    xglFontDebugPrintf(0, 0, tiWaterMenuFormat, tiWaterDebugIndex + 1, TI_WATER_DEBUG_PARAMS);
    i = tiWaterDebugIndex;
    for (; line < TI_WATER_DEBUG_LINES && i < TI_WATER_DEBUG_PARAMS; i++, line++) {
        if (tiWaterDebugIndex == i) {
            xglFontDebugPrintf(0, (line + 1) * TI_WATER_DEBUG_LINE_HEIGHT, tiWaterMenuMarker);
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
    nmlModelUseSubWindow(0, tiWaterIntVals[0]);
    nmlModelUseSubWindow(1, tiWaterIntVals[1]);
    nmlModelUseSubWindow(2, tiWaterIntVals[2]);
    nmlModelUseSubWindow(3, tiWaterIntVals[3]);
}

INCLUDE_ASM("asm/main/nonmatchings/TiWater", tiWaterRegisterSub);


XglPacket *xglPacketGetCurrent(void);
void sceVif1PkCnt(XglPacket *packet, int flag);
void sceVif1PkOpenDirectHLCode(XglPacket *packet, int flag);
void sceVif1PkAddDataN(XglPacket *packet, void *data, int count);
void sceVif1PkCloseDirectHLCode(XglPacket *packet);
void gifStateReset(void);
void gifStateFlush(void);

/* The water state template sent by tiWaterSendStateTemplate, and its length for sceVif1PkAddDataN. */
extern u64 tiWaterStateTemplate[];
#define TI_WATER_STATE_TEMPLATE_COUNT 0x3C
/* The two words tiWaterSendStateTemplate patches: the data of the FRAME_1 and TEX0_1 A+D pairs. */
#define TI_WATER_TEMPLATE_FRAME_1 (0x60 / 8)
#define TI_WATER_TEMPLATE_TEX0_1 (0x70 / 8)

/* Sends the water state template: FRAME_1 at the frame buffer and TEX0_1 on the display buffer
 * (512 x 512, with alpha) are patched in, then the template goes out as direct data. */
void tiWaterSendStateTemplate(void)
{
    XglPacket *packet = xglPacketGetCurrent();

    tiWaterStateTemplate[TI_WATER_TEMPLATE_FRAME_1] = GS_SET_FRAME(sRender.frame_base, 8, 0, 0);
    tiWaterStateTemplate[TI_WATER_TEMPLATE_TEX0_1] = GS_SET_TEX0(sRender.display_base << 5, 8, 0, 9, 9, 1, 0, 0, 0, 0, 0, 0);
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDataN(packet, tiWaterStateTemplate, TI_WATER_STATE_TEMPLATE_COUNT);
    sceVif1PkCloseDirectHLCode(packet);
}

void tiWaterSendFrame(XglCamera *camera, Matrix4 transform);
void tiWaterSendItemPair(float first, float second);
void tiWaterSendDirection(XglCamera *camera, Matrix4 transform);
void tiWaterSendGrid(TiWatSimItem *item, void *program);
void sceVif1PkAddCode(XglPacket *packet, unsigned int code);

/* VU1 microprogram entry points of the water drawing and the grid tables sent with them. */
extern unsigned char D_688[];
extern unsigned char D_7F0[];
extern unsigned char D_9F0[];
extern int tiWaterDepthProgramSelect;
extern unsigned char tiWaterSurfaceGridHeader[];
extern unsigned char tiWaterDepthGridHeader[];

/* VIF MSCAL: start the VU1 microprogram at the given address (in 8-byte units). */
#define TI_WATER_VIF_MSCAL(address) (0x14000000 | ((unsigned int) (address) >> 3))

/* Item flags: the two draw passes. */
#define TI_WATER_ITEM_DRAW_SURFACE 0x4
#define TI_WATER_ITEM_DRAW_DEPTH 0x2

/* Draws one water item: sends its frame, values and direction, then for each enabled pass the GS state
 * (clamp, frame on the display page, blend, the frame page as texture, Z test) and the VU1 program with
 * the item's grid. */
void tiWaterDrawItem(TiWatSimItem *item)
{
    XglPacket *packet = xglPacketGetCurrent();
    XglCamera *camera = xglStudioSelectGetActiveCamera(0);

    tiWaterSendFrame(camera, item->transform);
    tiWaterSendItemPair(item->drawPair[0], item->drawPair[1]);
    tiWaterSendDirection(camera, item->transform);
    if (item->flags & TI_WATER_ITEM_DRAW_SURFACE) {
        sceVif1PkCnt(packet, 0);
        sceVif1PkOpenDirectHLCode(packet, 0);
        gifStateAddReg(GS_CLAMP_1, GS_SET_CLAMP(GS_REGION_CLAMP, GS_REGION_CLAMP, 256, 510, 0, 222));
        gifStateAddReg(GS_FBA_1, 1);
        gifStateAddReg(GS_FRAME_1, GS_SET_FRAME(sRender.display_base, 8, 0, 0));
        gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 1, 2, 1, 0x4F));
        gifStateAddReg(GS_TEX0_1, GS_SET_TEX0(sRender.frame_base << 5, 8, 0, 9, 9, 0, 0, 0, 0, 0, 0, 0));
        gifStateAddReg(GS_TEX1_1, GS_SET_TEX1(0, 0, 1, 1, 0, 0, 0));
        gifStateAddReg(GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2));
        gifStateAddReg(GS_TEXFLUSH, 0);
        gifStateAddReg(GS_ZBUF_1, GS_SET_ZBUF(0, 0x31, 0));
        gifStateFlush();
        sceVif1PkCloseDirectHLCode(packet);
        sceVif1PkCnt(packet, 0);
        sceVif1PkAddCode(packet, TI_WATER_VIF_MSCAL(D_688));
        tiWaterSendGrid(item, tiWaterSurfaceGridHeader);
    }
    if (item->flags & TI_WATER_ITEM_DRAW_DEPTH) {
        sceVif1PkCnt(packet, 0);
        sceVif1PkOpenDirectHLCode(packet, 0);
        gifStateAddReg(GS_CLAMP_1, GS_SET_CLAMP(GS_REGION_CLAMP, GS_REGION_CLAMP, 0, 254, 0, 222));
        gifStateAddReg(GS_FBA_1, 1);
        gifStateAddReg(GS_FRAME_1, GS_SET_FRAME(sRender.display_base, 8, 0, 0));
        gifStateAddReg(GS_ALPHA_1, GS_SET_ALPHA(0, 2, 2, 1, 0x40));
        gifStateAddReg(GS_TEX0_1, GS_SET_TEX0(sRender.frame_base << 5, 8, 0, 8, 8, 1, 0, 0, 0, 0, 0, 0));
        gifStateAddReg(GS_TEX1_1, GS_SET_TEX1(0, 0, 1, 1, 0, 0, 0));
        gifStateAddReg(GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2));
        gifStateAddReg(GS_TEXFLUSH, 0);
        gifStateAddReg(GS_ZBUF_1, GS_SET_ZBUF(0, 0x31, 0));
        gifStateFlush();
        sceVif1PkCloseDirectHLCode(packet);
        sceVif1PkCnt(packet, 0);
        if (tiWaterDepthProgramSelect != 0) {
            sceVif1PkAddCode(packet, TI_WATER_VIF_MSCAL(D_7F0));
        } else {
            sceVif1PkAddCode(packet, TI_WATER_VIF_MSCAL(D_9F0));
        }
        tiWaterSendGrid(item, tiWaterDepthGridHeader);
    }
}

void xglPacketInitVuStateArg(int mode);

/* Draw callback: in pass 0, draws every registered table (CLAMP_1 and the state template first when
 * it has items, then each item) and empties the list; the packet state is reset afterwards. */
void tiWaterDrawAll(int pass)
{
    XglPacket *packet = xglPacketGetCurrent();
    unsigned int i;
    unsigned int j;
    TiWatSimItem *item;

    if (pass != 0) {
        return;
    }
    for (i = 0; i < tiWaterSubCount; i++) {
        item = tiWaterSubList[i]->items;
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
            item++;
        }
    }
    tiWaterSubCount = 0;
    xglPacketModeReset();
    xglPacketInitVuStateArg(pass);
    func_001CD134();
    func_001CD1AC();
    func_001CD160();
}

float atanf(float value);
extern float tiWaterTwoPi;

/* The angle of (x, y) in [0, 2 pi): atan(y / x) moved into the right half turn. */
float tiWaterVectorAngle(float x, float y)
{
    float angle;

    if (x == 0.0f) {
        x = 1e-05f;
    }
    angle = atanf(y / x);
    if (x > 0.0f) {
        if (angle < 0.0f) {
            angle += tiWaterTwoPi;
        }
    } else {
        angle += 3.1415927f;
    }
    return angle;
}

/* Source steps per unit of pad direction. */
#define TI_WATER_SOURCE_STEP 0.001f

/* Moves the water source actor by the pad direction of port 0 (cross enables its channel). While a direction
 * is held, it turns the actor towards it and disturbs every registered water table at the actor's position.
 * Then stores the source position for the simulation. */
void tiWaterUpdateSource(struct XenoActor *actor)
{
    int i;

    actor->previousPosition.x = actor->position.x;
    actor->previousPosition.z = actor->position.z;
    actor->position.x += PadData[0].directionX * TI_WATER_SOURCE_STEP;
    actor->position.z += PadData[0].directionY * TI_WATER_SOURCE_STEP;
    if (PadData[0].held & PAD_CROSS) {
        XenoActorEnableChannel(actor);
    }
    if (PadData[0].directionX != 0 || PadData[0].directionY != 0) {
        actor->rotation.y = tiWaterVectorAngle(PadData[0].directionY, PadData[0].directionX);
        for (i = 0; i < tiWaterSubCount; i++) {
            /* TiWatSimTable is TiWatSim.c's view of a TiWaterSub. */
            tiWatSimDisturbAll((TiWatSimTable *) tiWaterSubList[i], &actor->position);
        }
        /* The original computes the length of the direction here and drops it. */
        (void) __builtin_sqrtf(PadData[0].directionY * PadData[0].directionY
                               + PadData[0].directionX * PadData[0].directionX);
    }
    tiWatSimStoreSourcePos(&actor->position);
}

/* tu133.c defines it with the item only and passes $f12 on to tiWatSimAdvance. The step comes first
 * here: EABI uses the same registers either way, and this order gives the original's argument setup. */
void tiWatSimUpdate(float step, TiWatSimItem *item);

/* Steps the rain and the simulation of every item of the sub-object. */
void tiWaterStepSub(struct TiWaterSub *sub)
{
    TiWatSimItem *item = sub->items;
    unsigned int i;

    for (i = 0; i < sub->active; i++) {
        tiWatSimStepRain(item);
        tiWatSimUpdate(1.0f, item);
        item++;
    }
}

void tiWaterSetEffectMode(void) {
    func_00246AF8();
    func_00246DD8(0xFFFFFF);
    gifStateSetMode((int) tiWaterEffectNum);
}

void tiWaterInitStudio(void);
struct XenoActor *XenoActorAlloc(void);
void XenoActorAllocMatrices(struct XenoActor *actor);
void XenoActorSetMode(struct XenoActor *actor, int mode);
void func_001BA68C(void);
extern const char tiWaterLandscapeName[];
extern const char tiWaterSourceFile[];
extern const char tiWaterLandscapeModelPath[];
extern const char tiWaterLandscapeTexturePath[];
extern const char tiWaterLandscapeModelAssert[];
extern const char tiWaterLandscapeTextureAssert[];
extern const char tiWaterKosmosName[];
extern const char tiWaterKosmosModelPath[];
extern const char tiWaterKosmosTexturePath[];
extern const char tiWaterKosmosHierarchyPath[];
extern const char tiWaterKosmosMotionPath[];
extern const char tiWaterKosmosMotionExt2Path[];
extern const char tiWaterKosmosModelAssert[];
extern const char tiWaterKosmosTextureAssert[];
extern const char tiWaterKosmosHierarchyAssert[];
extern const char tiWaterKosmosMotionAssert[];
extern const char tiWaterKosmosMotionExt2Assert[];
extern const DoublewordAlignedVector4 tiWaterMulColor;

/* The test heap the scene files are loaded into. */
#define TI_WATER_HEAP 0x1000000
/* XenoActor drawFlags bit set on the source actor. */
#define TI_WATER_SOURCE_DRAW_FLAG 0x800000
/* Select held and start pressed end the scene. */
#define TI_WATER_EXIT (PAD_SELECT | (PAD_START << 16))
/* The landscape frame counts up to this and starts again. */
#define TI_WATER_FRAMES 120.0f

/* The water test scene: loads the landscape and the KOS-MOS source actor from the host, sets the GIF effect
 * up from the debug parameters and runs the frame loop (L2 toggles the debug panel, circle applies the effect
 * mode) until select is held with start pressed. */
void tiWaterSceneLoop(void)
{
    DoublewordAlignedVector4 color;
    int heap;
    XglCamera *camera;
    struct XenoActor *landscape;
    struct XenoActor *kosmos;

    heap = TI_WATER_HEAP;
    tiWaterInitStudio();
    XenoActorPoolInitDefault();
    landscape = XenoActorAlloc();
    if (landscape == 0) {
        assertFailed(tiWaterLandscapeName, tiWaterSourceFile, 578);
    }
    landscape->pack = testLoadFile(&heap, tiWaterLandscapeModelPath);
    landscape->effect = testLoadFile(&heap, tiWaterLandscapeTexturePath);
    if (landscape->pack == 0) {
        assertFailed(tiWaterLandscapeModelAssert, tiWaterSourceFile, 581);
    }
    if (landscape->effect == 0) {
        assertFailed(tiWaterLandscapeTextureAssert, tiWaterSourceFile, 582);
    }
    kosmos = XenoActorAlloc();
    if (kosmos == 0) {
        assertFailed(tiWaterKosmosName, tiWaterSourceFile, 585);
    }
    kosmos->pack = testLoadFile(&heap, tiWaterKosmosModelPath);
    kosmos->effect = testLoadFile(&heap, tiWaterKosmosTexturePath);
    kosmos->hierarchy = testLoadFile(&heap, tiWaterKosmosHierarchyPath);
    kosmos->slotTable = testLoadFile(&heap, tiWaterKosmosMotionPath);
    kosmos->xapExt2 = testLoadFile(&heap, tiWaterKosmosMotionExt2Path);
    if (kosmos->pack == 0) {
        assertFailed(tiWaterKosmosModelAssert, tiWaterSourceFile, 596);
    }
    if (kosmos->effect == 0) {
        assertFailed(tiWaterKosmosTextureAssert, tiWaterSourceFile, 597);
    }
    if (kosmos->hierarchy == 0) {
        assertFailed(tiWaterKosmosHierarchyAssert, tiWaterSourceFile, 598);
    }
    if (kosmos->slotTable == 0) {
        assertFailed(tiWaterKosmosMotionAssert, tiWaterSourceFile, 599);
    }
    if (kosmos->xapExt2 == 0) {
        assertFailed(tiWaterKosmosMotionExt2Assert, tiWaterSourceFile, 600);
    }
    kosmos->drawFlags |= TI_WATER_SOURCE_DRAW_FLAG;
    XenoActorAllocMatrices(kosmos);
    XenoActorSetChannelValue(kosmos, 8.0f);
    XenoActorSetMode(kosmos, 0);
    tiWaterDebugReset();
    gifStateSetCenter(tiWaterCx, tiWaterCy);
    gifStateSetEffectAlpha(tiWaterAlphaType, tiWaterAlpha);
    gifStateSetDecrease(tiWaterDecrease);
    gifStateSetFbp(tiWaterFbp);
    gifStateSetBlur(tiWaterBlur);
    color = tiWaterMulColor;
    nmlModelSetMulColor(&color);
    while ((PadData[0].heldAndPressed & TI_WATER_EXIT) != TI_WATER_EXIT) {
        xglRenderClearFrame();
        func_001BA68C();
        if ((unsigned short) (PadData[0].pressed & PAD_L2)) {
            if (++tiWaterDebugMode >= 2) {
                tiWaterDebugMode = 0;
            }
        }
        if (tiWaterDebugMode == 1) {
            tiWaterDebugUpdate();
        }
        if (PadData[0].pressed & PAD_CIRCLE) {
            tiWaterSetEffectMode();
        }
        landscape->frame += 1.0f;
        if (TI_WATER_FRAMES <= landscape->frame) {
            landscape->frame = 0.0f;
        }
        xglStudioGetCamera(&camera, 0);
        XenoActorDoit();
        XenoActorDisp();
        tiWaterUpdateSource(kosmos);
        xglSleep();
    }
    XenoActorReleaseAll();
    xglRenderClearFrame();
    gifStateModeHook = 0;
}

void pxySetDrawCallback(void (*callback)(int pass));
int tiWaterRegisterSub(struct TiWaterSub *sub);

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

/* Starts the studio with its main camera, clears the camera's rotation and position, marks it active in
 * state 2 and moves the studio. */
void tiWaterInitStudio(void)
{
    XglCamera *camera;
    XglCamera *current;

    xglStudioInit();
    xglStudioChange(0);
    xglStudioMainCameraInit();
    xglStudioGetCamera(&camera, 0);
    camera->travel.rotation.x = 0.0f;
    camera->travel.rotation.y = 0.0f;
    camera->travel.rotation.z = 0.0f;
    camera->travel.position.x = 0.0f;
    camera->travel.position.y = 0.0f;
    camera->travel.position.z = 0.0f;
    current = camera;
    current->active = 1;
    current->state = 2;
    xglStudioMove();
}

float cosf(float angle);
float sinf(float angle);

/* The fine camera steps: distance per frame and angle per frame. */
#define TI_WATER_CAMERA_FINE_STEP 0.2f
#define TI_WATER_CAMERA_FINE_TURN 0.02f

/* The coarse camera step (the turn step is the fine one). */
#define TI_WATER_CAMERA_STEP 5.0f

/* Debug camera control on pad 0 (held buttons) like tiWaterCameraUpdateFine, with a step of 5;
 * with start held, square/circle turn the roll instead of the heading. */
void tiWaterCameraUpdate(XglCamera *camera)
{
    float step = TI_WATER_CAMERA_STEP;
    float forward = cosf(camera->travel.rotation.y) * step;
    float side = sinf(camera->travel.rotation.y) * step;

    if (PadData[0].held & PAD_RIGHT) {
        camera->travel.position.x += forward;
        camera->travel.position.z -= side;
    }
    if (PadData[0].held & PAD_LEFT) {
        camera->travel.position.x -= forward;
        camera->travel.position.z += side;
    }
    if (PadData[0].held & PAD_UP) {
        camera->travel.position.x -= side;
        camera->travel.position.z -= forward;
    }
    if (PadData[0].held & PAD_DOWN) {
        camera->travel.position.x += side;
        camera->travel.position.z += forward;
    }
    if (PadData[0].held & PAD_R2) {
        camera->travel.position.y -= step;
    }
    if (PadData[0].held & PAD_R1) {
        camera->travel.position.y += step;
    }
    if (PadData[0].held & PAD_TRIANGLE) {
        camera->travel.rotation.x += TI_WATER_CAMERA_FINE_TURN;
        if (camera->travel.rotation.x > tiWaterTwoPi) {
            camera->travel.rotation.x -= tiWaterTwoPi;
        }
    }
    if (PadData[0].held & PAD_CROSS) {
        camera->travel.rotation.x -= TI_WATER_CAMERA_FINE_TURN;
        if (camera->travel.rotation.x < 0.0f) {
            camera->travel.rotation.x += tiWaterTwoPi;
        }
    }
    if (PadData[0].held & PAD_SQUARE) {
        if (PadData[0].held & PAD_START) {
            camera->travel.rotation.z += TI_WATER_CAMERA_FINE_TURN;
            if (camera->travel.rotation.z > tiWaterTwoPi) {
                camera->travel.rotation.z -= tiWaterTwoPi;
            }
        } else {
            camera->travel.rotation.y += TI_WATER_CAMERA_FINE_TURN;
            if (camera->travel.rotation.y > tiWaterTwoPi) {
                camera->travel.rotation.y -= tiWaterTwoPi;
            }
        }
    }
    if (PadData[0].held & PAD_CIRCLE) {
        if (PadData[0].held & PAD_START) {
            camera->travel.rotation.z -= TI_WATER_CAMERA_FINE_TURN;
            if (camera->travel.rotation.z < 0.0f) {
                camera->travel.rotation.z += tiWaterTwoPi;
            }
        } else {
            camera->travel.rotation.y -= TI_WATER_CAMERA_FINE_TURN;
            if (camera->travel.rotation.y < 0.0f) {
                camera->travel.rotation.y += tiWaterTwoPi;
            }
        }
    }
}

/* Fine debug camera control on pad 0 (held buttons): the d-pad moves in the heading's plane, R2/R1
 * move down/up, triangle/cross change the pitch and square/circle the heading (both kept in
 * [0, 2 pi)). */
void tiWaterCameraUpdateFine(XglCamera *camera)
{
    float step = TI_WATER_CAMERA_FINE_STEP;
    float forward = cosf(camera->travel.rotation.y) * step;
    float side = sinf(camera->travel.rotation.y) * step;

    if (PadData[0].held & PAD_RIGHT) {
        camera->travel.position.x += forward;
        camera->travel.position.z -= side;
    }
    if (PadData[0].held & PAD_LEFT) {
        camera->travel.position.x -= forward;
        camera->travel.position.z += side;
    }
    if (PadData[0].held & PAD_UP) {
        camera->travel.position.x -= side;
        camera->travel.position.z -= forward;
    }
    if (PadData[0].held & PAD_DOWN) {
        camera->travel.position.x += side;
        camera->travel.position.z += forward;
    }
    if (PadData[0].held & PAD_R2) {
        camera->travel.position.y -= step;
    }
    if (PadData[0].held & PAD_R1) {
        camera->travel.position.y += step;
    }
    if (PadData[0].held & PAD_TRIANGLE) {
        camera->travel.rotation.x += TI_WATER_CAMERA_FINE_TURN;
        if (camera->travel.rotation.x > tiWaterTwoPi) {
            camera->travel.rotation.x -= tiWaterTwoPi;
        }
    }
    if (PadData[0].held & PAD_CROSS) {
        camera->travel.rotation.x -= TI_WATER_CAMERA_FINE_TURN;
        if (camera->travel.rotation.x < 0.0f) {
            camera->travel.rotation.x += tiWaterTwoPi;
        }
    }
    if (PadData[0].held & PAD_SQUARE) {
        camera->travel.rotation.y += TI_WATER_CAMERA_FINE_TURN;
        if (camera->travel.rotation.y > tiWaterTwoPi) {
            camera->travel.rotation.y -= tiWaterTwoPi;
        }
    }
    if (PadData[0].held & PAD_CIRCLE) {
        camera->travel.rotation.y -= TI_WATER_CAMERA_FINE_TURN;
        if (camera->travel.rotation.y < 0.0f) {
            camera->travel.rotation.y += tiWaterTwoPi;
        }
    }
}
