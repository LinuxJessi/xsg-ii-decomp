/*
 * OV01 original TU 74: 0x00aaab80..0x00aaad58 (3 functions)
 */
#include "common.h"
#include "shared.h"
#include "main/sdv_033.h"
#include "main/xgl_studio.h"

#define OV01_STUDIO_COUNT 4
#define OV01_STUDIO_CAMERA_COUNT 8

/* Scene driver: runs the setup chain, then the frame loop until the config menu returns 0. */
typedef struct ClearEnvColor {
    u8 unmodeled_00[0x20];
    u32 color_r;
    u32 color_g;
    u32 color_b;
    u32 color_a;
} ClearEnvColor;

extern ClearEnvColor ClearEnv;
void xglRenderClearFrame(void);
void xglSleep(void);
void resInitPathHeader(int key, int value);
void xglCdArenaOpen(int size, long long handle);
void menuSceneReset(void);
void msgResetAll(void);
void GameResArenaRelease(void);
void battleResultScene(int mode);
void ov01InitAll(void);
void ov01SceneEnter(void);
void ov01SceneLoop(void);
void ov01SceneEnd(void);
int configMenuRun(int mode);
int tgtSelectStatusGet(void);
void ov01StudioCameraSetup(void);

void ov01SceneRun(void)
{
    int selectStatus;

    xglRenderClearFrame();
    ClearEnv.color_g = 64;
    ClearEnv.color_a = 128;
    ClearEnv.color_b = 0;
    ClearEnv.color_r = 0;
    resInitPathHeader(0x1000000, 0x1000000);
    ov01InitAll();
    ov01StudioCameraSetup();
    menuSceneReset();
    msgResetAll();
    for (;;) {
        ov01SceneEnter();
        if (configMenuRun(0) == 0) {
            break;
        }
        ov01SceneLoop();
        ov01SceneEnd();
        GameResArenaRelease();
        selectStatus = tgtSelectStatusGet();
        if (selectStatus == 1 || selectStatus == 3) {
            xglCdArenaOpen(0, 0);
            battleResultScene(0);
        }
        xglSleep();
    }
    ov01SceneEnd();
    GameResArenaRelease();
    xglRenderClearFrame();
}

void XenoActorPoolInitDefault(void);
void xglRenderDispOn(void);
void xglRenderCpUDispOff(void);
void func_001B274C(void);
XglCamera *xglStudioGetCamera2(int camera_index);
void xglCameraInit(XglCamera *camera);
void xglCameraMove(XglCamera *camera);
void xglCameraSetPadPort(XglCamera *camera, int port);

/* Studio camera setup: four studios of eight cameras each; camera 0 of each studio is the main camera. */
void ov01StudioCameraSetup(void)
{
    int studio_index;
    int camera_index;
    XglCamera *camera;

    XenoActorPoolInitDefault();
    xglRenderDispOn();
    xglStudioInit();
    for (studio_index = 0; studio_index < OV01_STUDIO_COUNT; studio_index++) {
        xglStudioChange(studio_index);
        xglStudioMainCameraInit();
        camera = xglStudioGetCamera2(0);
        camera->state = 2;
        xglCameraSetPadPort(camera, 1);
        for (camera_index = 1; camera_index < OV01_STUDIO_CAMERA_COUNT; camera_index++) {
            camera = xglStudioGetCamera2(camera_index);
            xglCameraInit(camera);
            xglCameraMove(camera);
            xglCameraSetPadPort(camera, 0);
            camera->state = 2;
        }
    }
    func_001B274C();
    xglRenderCpUDispOff();
}

/* Empty stub: does nothing and has no recorded caller. */
void func_00AAAD50(void)
{
}
