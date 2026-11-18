#include "common.h"
#include "shared.h"
#include "main/pad.h"
#include "main/TiWater.h"
#include "main/tu051.h"

extern PadRecord PadData[];

/* Moves the glow actor by the pad direction of port 0 (x and z), raises and lowers it with the up and
 * down buttons, and turns it towards a held direction. */
void glowUpdateFromPad(struct XenoActor *actor)
{
    actor->position.x += PadData[0].directionX * 0.001f;
    actor->position.z += PadData[0].directionY * 0.001f;
    if (PadData[0].held & PAD_UP) {
        actor->position.y += 0.1f;
    }
    if (PadData[0].held & PAD_DOWN) {
        actor->position.y -= 0.1f;
    }
    if (PadData[0].directionX != 0 || PadData[0].directionY != 0) {
        actor->rotation.y = tiWaterVectorAngle(PadData[0].directionY, PadData[0].directionX);
    }
}

#include "main/xenoActor.h"
#include "main/test_129.h"
#include "main/tu026.h"
#include "main/gifState.h"
#include "main/xgl_cd.h"
#include "main/xgl_render.h"
#include "main/xgl_studio.h"
#include "main/control_entry.h"

void XenoActorAllocMatrices(struct XenoActor *actor);
void XenoActorSetMode(struct XenoActor *actor, int mode);
void tiWaterInitStudio(void);
void func_001BA68C(void);
extern const char D_0068CB58[];
/* "pLandscape" */
extern const char D_0068CB68[];
/* "../glow.euc.c" */
extern const char D_0068CB78[];
/* "host0:/home/xeno/work/iwase/glow3.pxy" */
extern const char D_0068CBA0[];
/* "host0:/home/xeno/work/iwase/glow3.txy" */
extern const char D_0068CBC8[];
/* "pLandscape->rsrc[XAct_Rsrc_MDL]" */
extern const char D_0068CBE8[];
/* "pLandscape->rsrc[XAct_Rsrc_TEX]" */
extern const char D_0068CC08[];
/* "pChaos" */
extern const char D_0068CC10[];
/* "host0:/home/xeno/work/iwase/ziggy00.pxy" */
extern const char D_0068CC38[];
/* "host0:/home/xeno/work/iwase/ziggy00.txy" */
extern const char D_0068CC60[];
/* "host0:/home/xeno/work/iwase/ziggy00.xhr" */
extern const char D_0068CC88[];
/* "host0:/home/xeno/work/iwase/ziggy00.xap" */
extern const char D_0068CCB0[];
/* "pChaos->rsrc[XAct_Rsrc_MDL]" */
extern const char D_0068CCD0[];
/* "pChaos->rsrc[XAct_Rsrc_TEX]" */
extern const char D_0068CCF0[];

/* Loads the landscape and the chaos models, then runs the scene until SELECT and START are pressed. */
void glowRunScene(void)
{
    struct XenoActor *pLandscape;
    int cdState;
    int bufAddr;
    int loadAddr;
    struct XglCamera *camera;
    struct XenoActor *pChaos;

    loadAddr = 0x1000000;
    tiWaterInitStudio();
    XenoActorPoolInitDefault();
    pLandscape = XenoActorAlloc();
    if (pLandscape == 0) {
        assertFailed(D_0068CB58, D_0068CB68, 72);
    }
    pLandscape->rsrc[XAct_Rsrc_MDL] = testLoadFile(&loadAddr, D_0068CB78);
    pLandscape->rsrc[XAct_Rsrc_TEX] = testLoadFile(&loadAddr, D_0068CBA0);
    if (pLandscape->rsrc[XAct_Rsrc_MDL] == 0) {
        assertFailed(D_0068CBC8, D_0068CB68, 75);
    }
    if (pLandscape->rsrc[XAct_Rsrc_TEX] == 0) {
        assertFailed(D_0068CBE8, D_0068CB68, 76);
    }

    pChaos = XenoActorAlloc();
    if (pChaos == 0) {
        assertFailed(D_0068CC08, D_0068CB68, 79);
    }
    pChaos->rsrc[XAct_Rsrc_MDL] = testLoadFile(&loadAddr, D_0068CC10);
    pChaos->rsrc[XAct_Rsrc_TEX] = testLoadFile(&loadAddr, D_0068CC38);
    pChaos->rsrc[XAct_Rsrc_XHR] = testLoadFile(&loadAddr, D_0068CC60);
    pChaos->rsrc[XAct_Rsrc_XAP] = testLoadFile(&loadAddr, D_0068CC88);
    if (pChaos->rsrc[XAct_Rsrc_MDL] == 0) {
        assertFailed(D_0068CCB0, D_0068CB68, 84);
    }
    if (pChaos->rsrc[XAct_Rsrc_TEX] == 0) {
        assertFailed(D_0068CCD0, D_0068CB68, 85);
    }
    cdState = -1;
    pChaos->position.x = 2.0f;
    XenoActorAllocMatrices(pChaos);
    XenoActorSetChannelValue(pChaos, 8.0f);
    XenoActorSetMode(pChaos, 0);
    bufAddr = loadAddr;
    xglCdReadFile(D_0068CCF0, (void *) bufAddr, 1, 1);

    while ((PadData[0].heldAndPressed & (PAD_SELECT | (PAD_START << 16))) != (PAD_SELECT | (PAD_START << 16))) {
        xglRenderClearFrame();
        if (PadData[0].pressed & PAD_L2) {
            xglCdReadFile(D_0068CCF0, (void *) bufAddr, 1, 1);
            cdState = 0;
        }
        if (cdState >= 0) {
            cdState = xglCdServiceIfPending() ? cdState + 1 : -1;
        }
        glowUpdateFromPad(pChaos);
        xglStudioGetCamera(&camera, 0);
        func_001BA68C();
        XenoActorDoit();
        XenoActorDisp();
        xglSleep();
    }
    XenoActorReleaseAll();
    xglRenderClearFrame();
    gifStateModeHook = 0;
}

const char D_0068CB58[16] = "pLandscape";
const char D_0068CB68[16] = "../glow.euc.c";
const char D_0068CB78[40] = "host0:/home/xeno/work/iwase/glow3.pxy";
const char D_0068CBA0[40] = "host0:/home/xeno/work/iwase/glow3.txy";
const char D_0068CBC8[32] = "pLandscape->rsrc[XAct_Rsrc_MDL]";
const char D_0068CBE8[32] = "pLandscape->rsrc[XAct_Rsrc_TEX]";
const char D_0068CC08[8] = "pChaos";
const char D_0068CC10[40] = "host0:/home/xeno/work/iwase/ziggy00.pxy";
const char D_0068CC38[40] = "host0:/home/xeno/work/iwase/ziggy00.txy";
const char D_0068CC60[40] = "host0:/home/xeno/work/iwase/ziggy00.xhr";
const char D_0068CC88[40] = "host0:/home/xeno/work/iwase/ziggy00.xap";
const char D_0068CCB0[32] = "pChaos->rsrc[XAct_Rsrc_MDL]";
const char D_0068CCD0[32] = "pChaos->rsrc[XAct_Rsrc_TEX]";
const char D_0068CCF0[32] = "iwase\\citizen.xtx";
