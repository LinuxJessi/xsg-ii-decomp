#include "common.h"
#include "shared.h"
#include "main/control_entry.h"
#include "main/gifState.h"
#include "main/tu092.h"
#include "main/tu094.h"
#include "main/tu097.h"
#include "main/tu101.h"
#include "main/xenoActor.h"
#include "main/xgl_cd.h"
#include "main/yg2path.h"

extern void *memset(void *destination, int value, unsigned int count);

/* Partial view of sRender: a callback pointer at +0x48 and the word that
   follows it at +0x4C; the prefix before +0x48 is not modeled here. */
typedef struct YgPathRenderState {
    u8 unmodeled_00[0x48];
    void (*callback)(void);
    int callback_context;
} YgPathRenderState;

/* citizenFlags: one byte per citizen (0x32 of them) and questFlags: one byte
   per path (0x24 of them); the low nibble of a path byte says whether the
   path is available. yg2PathModelHandles[0] holds the handle of the loaded model. */
extern u8 citizenFlags[];
extern u8 questFlags[];
extern int yg2PathModelHandles[];
extern int D_0069D7D8;
extern int D_0069D7DC;
extern YgPathRenderState sRender;
void xglCallbackQueueRun(void);

/* Returns the citizen flag at index (0x32 entries), 0 when out of range. */
u8 yg2PathGetCitizenFlag(u32 index) {
    if (index < 0x32U) {
        return citizenFlags[index];
    }
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/yg2path", yg2PathShowPathMessage);

void yg2PathClearProgress(void) {
    memset(questFlags, 0, 0x24);
    memset(citizenFlags, 0, 0x32);
}

INCLUDE_ASM("asm/main/nonmatchings/yg2path", yg2PathInit);

void XenoActorSetMode(int handle, int value);

/* Sets the mode of the loaded yg2path model (yg2PathModelHandles[0]), when one is loaded. */
void yg2PathSetModelMode(int mode) {
    if (yg2PathModelHandles[0] != 0) {
        XenoActorSetChannelValue(yg2PathModelHandles[0], 7.0f);
        XenoActorEnableChannel(yg2PathModelHandles[0]);
        XenoActorSetMode(yg2PathModelHandles[0], mode);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/yg2path", yg2PathReadTaggedText);

void XenoActorSetFlagAll(int count);
void XenoActorDoit(void);
void XenoActorDisp(void);

void yg2PathWaitForLoad(void) {
    while (xglCdServiceIfPending() != 0) {
        XenoActorSetFlagAll(2);
        bgLayerClearAllFlags();
        XenoActorDoit();
        XenoActorDisp();
        xglSleep();
    }
}

INCLUDE_ASM("asm/main/nonmatchings/yg2path", yg2PathLoadQuestMessages);

INCLUDE_ASM("asm/main/nonmatchings/yg2path", yg2PathLoadCitizens);

INCLUDE_ASM("asm/main/nonmatchings/yg2path", yg2PathLoadScene);

void pxySetCameraEnable(int first, int second);
void XenoActorRelease(int handle);

void yg2PathUnload(void) {
    if (yg2PathModelHandles[0] != 0) {
        XenoActorRelease(yg2PathModelHandles[0]);
        yg2PathModelHandles[0] = 0;
    }
    D_0069D7D8 = 0;
    D_0069D7DC = 0;
    xglCdBufferRewind();
    pxySetCameraEnable(1, 0);
    sRender.callback = xglCallbackQueueRun;
    sRender.callback_context = 0;
}

void panelDrawNineSlice(unsigned char *, int, int, int, int);
extern int D_0069D834;
extern unsigned char D_003EB308[];
extern unsigned char D_003EB358[];

void yg2PathDrawFramedImage(int x, int y, int width, int height) {
    fontObjSetOpacity(0x80);
    fontObjSetDepth(D_0069D834);
    packetSubmitRequestEx(0x1EB, x, y, width, height, 0x20010);
    fontObjSetDepth(0);
    panelDrawNineSlice(D_003EB308, x, y, width, height);
    panelDrawNineSlice(D_003EB358, x - 5, y - 5, width + 10, height + 10);
}

INCLUDE_ASM("asm/main/nonmatchings/yg2path", func_0022F5C8);

extern unsigned char yg2PathMode;
extern signed char yg2PathDetailOpen;

void func_0022F6E0(void) {
    fontObjSetOpacity(0x80);
    if (yg2PathMode < 2) {
        if (yg2PathMode == 0) {
            fontObjSetDepth(0x1FFFF);
            packetSubmitRequestEx(0x206, 0, 0x92, 0x200, 0x8C, 0x20001);
            packetSubmitRequest(0x18E, 0, 0x30, 0x20001);
            packetSubmitRequest(0x1FA, 0x164, 0x6F, 0x20001);
            fontObjSetDepth(0);
        }
        packetSubmitRequest(0x190, 0, 0x14, 1);
        packetSubmitRequest(0x18D, 0, 0, 1);
        packetSubmitRequest(0x208, 0x10, 0x12, 1);
    }
    if (yg2PathDetailOpen == 0) {
        packetSubmitRequest(0x207, 0, 0x170, 1);
        packetSubmitRequest(0x18F, 0, 0x170, 1);
    }
    fontObjResetOpacity();
}

INCLUDE_ASM("asm/main/nonmatchings/yg2path", func_0022F7E8);

INCLUDE_ASM("asm/main/nonmatchings/yg2path", yg2PathDrawPathGrid);

INCLUDE_ASM("asm/main/nonmatchings/yg2path", func_0022FE98);

INCLUDE_ASM("asm/main/nonmatchings/yg2path", yg2PathDrawMessagePage);

INCLUDE_ASM("asm/main/nonmatchings/yg2path", yg2PathDrawCitizenProfile);

void textDrawSegment(int, int, unsigned char *, int);
void func_0021E338(int, int, int, int);
extern int yg2PathScroll;
extern int yg2PathQuestMessages;
extern signed char D_0069E27A;
extern unsigned char D_003EC958[];
extern unsigned char D_0068B0E0[];

void yg2PathDrawPathDetail(void)
{
  float fraction;
  unsigned char entry;
  panelDrawNineSlice(D_003EC958, 0xD, 0x14, 0xF7, 0x194);
  packetSubmitRequest(0x202, 0x19, 0x19, 1);
  func_0021E338(0x15, 0x33, 0xE7, 0x167);
  packetSubmitRequest(0x20A, 0x176, 0x185, 1);
  packetSubmitRequest(0x209, 0x18B, 0x189, 1);
  textDrawSegment(0, 0, D_0068B0E0, 0);
  entry = questFlags[D_0069E27A];
  if (entry == 3)
  {
    textDrawSegment(0x1E, 0x3C, (unsigned char *) (yg2PathQuestMessages + D_0069E27A * 0xB46) + 0x745, 0);
  }
  else if (entry == 2)
  {
    textDrawSegment(0x1E, 0x3C, (unsigned char *) (yg2PathQuestMessages + D_0069E27A * 0xB46) + 0x344, 0);
  }
  menuDrawKindIcon(0x1BC, 0x189, 0);
  if (yg2PathScroll >= 0x80)
  {
    yg2PathDrawFramedImage(0x115, 0x57, 0xD8, 0xFE);
    return;
  }
  fraction = (float) yg2PathScroll * 0.0078125f;
  yg2PathDrawFramedImage(0x115, (int) (214.0f - fraction * 127.0f), 0xD8, (int) (fraction * 254.0f));
}

void msgRangeDrawRevealed();
void func_0022F6E0(void);
void func_0022F7E8();
void yg2PathDrawPathGrid();
void yg2PathDrawMessagePage();
void yg2PathDrawCitizenProfile();
void yg2PathDrawPathDetail(void);
void xglFontFlush();
extern unsigned char D_006ABC28[];

void yg2PathDraw(void)
{
  fontObjResetAndBuild();
  func_0022F6E0();
  if (yg2PathDetailOpen != 0)
  {
    yg2PathDrawPathDetail();
  }
  else
  {
    switch (yg2PathMode)
    {
      case 0:
        func_0022F7E8(yg2PathScroll);
        break;

      case 1:
        yg2PathDrawPathGrid(yg2PathScroll);
        break;

      case 2:
        yg2PathDrawMessagePage(yg2PathScroll);
        break;

      case 3:
        yg2PathDrawCitizenProfile(yg2PathScroll);
        break;

    }

  }
  if (yg2PathDetailOpen == 0)
  {
    msgRangeDrawRevealed(D_006ABC28, 0x18, 0x175);
  }
  xglFontFlush();
}

void yg2PathDraw(void);

void yg2PathDrawFrame(void) {
    yg2PathDraw();
    gifStateSuppressDrawHook();
}

INCLUDE_ASM("asm/main/nonmatchings/yg2path", func_00231148);

INCLUDE_ASM("asm/main/nonmatchings/yg2path", func_00231278);

INCLUDE_ASM("asm/main/nonmatchings/yg2path", yg2PathFindNextPath);

INCLUDE_ASM("asm/main/nonmatchings/yg2path", yg2PathFindPrevPath);

INCLUDE_ASM("asm/main/nonmatchings/yg2path", yg2PathHandlePathInput);

INCLUDE_ASM("asm/main/nonmatchings/yg2path", yg2PathHandleCitizenInput);

INCLUDE_ASM("asm/main/nonmatchings/yg2path", yg2PathUpdateCamera);

INCLUDE_ASM("asm/main/nonmatchings/yg2path", func_00231A20);

INCLUDE_ASM("asm/main/nonmatchings/yg2path", yg2PathRun);
