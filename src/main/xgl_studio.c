#include "common.h"
#include "main/sdv_033.h"
#include "shared.h"
#include "main/xgl_studio.h"


typedef float RgVector[4];

/*
 * main/tu149's light editor (DB_lightWrite, updateLight) indexes these
 * vectors as float[4] rows (component offsets fold into the row's index, not
 * the load displacement), so each vector also has its RgVector row.
 */
typedef struct StudioLightEntry {
    union { Vector4 color; RgVector color_row; };
    union { Vector4 direction; RgVector direction_row; };
} StudioLightEntry;

/* The light block of a studio record, also the light record of an Actor
 * (+0x510). Whole-block copies (game.c GameStateSave/RestoreCameraLight) are
 * ld/sd, while the Studio records holding it are only 8-aligned (xgl_studio
 * .bss): the block is 8-aligned, which the doubleword view of its first vector
 * gives. */
typedef struct StudioLight {
    union {
        struct {
            union { Vector4 ambient_color; RgVector ambient_color_row; u64 ambient_color_words[2]; };
            StudioLightEntry lights[3];
        };
        Vector4 parameters[7];          /* the seven quadwords xglLightCalcMatrix loads: ambient, then three colour/direction pairs */
    };
    float normal_matrix[4][4];          /* +0x70: map_1.c's MapDrawLight view */
    float color_modifier_matrix[4][4];  /* +0xb0: map_1.c's MapDrawLight view */
} StudioLight;

extern void xglStudioFlushActiveCamera(void);
XglCamera *xglStudioGetCamera2(int camera_index);
void xglCameraMove(XglCamera *camera);

/*
 * XglCamera (include/shared.h; config/header-canon.json,
 * decisions.XglCamera) is the active-flag/state/camera-slot layout this
 * TU evidences. xglStudioSelectGetActiveCamera (main:0x0022c618,
 * `lw $3,0($4)` feeding the `bne $3,$0` "camera in use" test) reads +0 as a
 * boolean active flag, and xglStudioMainCameraInit (main:0x0022c6f0/
 * 0x0022c6f8, `sw $17,4($2)` / `sw $17,0($2)` with $17=1) stores the same
 * word 1 to both +4 and +0 on init. The same function's
 * `addiu $4,$4,1520` stride (main:0x0022c620) sizes one camera slot at
 * 0x5F0 bytes, not just the 8-byte leading pair. +4 state keeps its name,
 * type and offset, so src/main/game.c's `camera->state == 4` reads the
 * same byte either way.
 */
/*
 * Layout the asm readers use: a studio record is 0x3180 bytes, its cameras
 * start at +0x180 and are 0x600 apart (xglStudioGetCamera2, xglStudioGetActiveCamera,
 * xglStudioSelectGetActiveCamera and xglStudioGetCamera all compute
 * 3*i << 9). StudioLight (0xF0) plus the 0x90 bytes after it put the cameras at +0x180,
 * and XglCamera is 0x600 bytes (a 0x5F8 camera object plus padding).
 */
typedef struct Studio {
    StudioLight light;
    u8 unmodeled_f0[0x90];
    XglCamera cameras[8];
} Studio;

extern StudioLight *xglStudioGetLight2(void);
static Studio *pCurrentStudio;

int D_0069E078 = 0;

void func_001B2724(void) {
    D_0069E078 = 1;
}

void func_001B274C(void) {
    D_0069E078 = 0;
}

/* The studio records, 0x3180 bytes each. */
extern unsigned char asStudioSource[];

/* Makes studio_index the current studio: pCurrentStudio points at its record. */
void xglStudioChange(int studio_index)
{
    pCurrentStudio = (Studio *) (asStudioSource + studio_index * 0x3180);
}

/* The studio index that xglStudioChangeActive selects and xglStudioMove compares with. */
static int activeStudioIndex;

void xglStudioSetActiveIndex(int studio_index)
{
    activeStudioIndex = studio_index;
}

void xglStudioGetCamera(XglCamera **camera, int camera_index)
{
    *camera = &pCurrentStudio->cameras[camera_index];
}

XglCamera *xglStudioGetActiveCamera(void)
{
    XglCamera *camera;
    int camera_index;

    for (camera_index = 0; camera_index < 8; camera_index++) {
        camera = xglStudioGetCamera2(camera_index);
        if (camera->active != 0)
            return camera;
    }
    return 0;
}

XglCamera *xglStudioSelectGetActiveCamera(int studio_index)
{
    XglCamera *camera;
    int camera_index;

    for (camera_index = 0; camera_index < 8; camera_index++) {
        camera = &((Studio *)(asStudioSource + studio_index * 0x3180))->cameras[camera_index];
        if (camera->active != 0)
            return camera;
    }
    return 0;
}

void xglStudioFlushActiveCamera(void)
{
    int studio_index;
    XglCamera *camera;

    for (studio_index = 0; studio_index < 4; studio_index++) {
        camera = xglStudioSelectGetActiveCamera(studio_index);
        if (camera != 0)
            xglCameraMove(camera);
    }
}

int studioCameraUpdateEnabled = 1;

void xglStudioDisableCameraUpdate(void) {
    studioCameraUpdateEnabled = 0;
}

void xglStudioEnableCameraUpdate(void) {
    studioCameraUpdateEnabled = 1;
}

StudioLight *xglStudioGetLight2(void)
{
    return &pCurrentStudio->light;
}

XglCamera *xglStudioGetCamera2(int camera_index)
{
    return &pCurrentStudio->cameras[camera_index];
}

void xglStudioMainCameraInit(void)
{
    XglCamera *camera;
    int studio_index;

    for (studio_index = 0; studio_index < 4; studio_index++) {
        xglStudioChange(studio_index);
        camera = xglStudioGetCamera2(0);
        camera->active = 1;
        camera->state = 1;
    }
    xglStudioChange(0);
}

void xglStudioChangeActive(void) {
    int result;

    xglStudioChange(activeStudioIndex);
    /* xglStudioGetCamera2(0) is the active studio's first camera; the value is not used here. */
    result = xglStudioGetCamera2(0);
}

int StudioEntrySkip = 0;

void xglStudioLightReset(void *state);
void xglCameraInit(XglCamera *camera);

/* Resets every studio: its light block and its eight cameras, then selects studio 0. */
void xglStudioInit(void)
{
    int studio_index;
    int camera_index;

    StudioEntrySkip = 0;
    func_001B274C();
    for (studio_index = 0; studio_index < 4; studio_index++) {
        xglStudioChange(studio_index);
        xglStudioLightReset(xglStudioGetLight2());
        for (camera_index = 0; camera_index < 8; camera_index++) {
            xglCameraInit(xglStudioGetCamera2(camera_index));
        }
    }
    xglStudioChange(0);
    studioCameraUpdateEnabled = 1;
    activeStudioIndex = 0;
}

void xglCameraUpdate(XglCamera *camera);

void xglStudioMove(void)
{
    XglCamera *camera;
    int studio_index;
    int camera_index;

    for (studio_index = 0; studio_index < 4; studio_index++) {
        xglStudioChange(studio_index);
        for (camera_index = 0; camera_index < 8; camera_index++) {
            camera = xglStudioGetCamera2(camera_index);
            if (camera->active == 1) {
                if (studio_index == activeStudioIndex && studioCameraUpdateEnabled)
                    xglCameraUpdate(camera);
                xglCameraMove(camera);
            }
        }
    }
    xglStudioChange(0);
}

void xglStudioSetup(void) {
    xglStudioInit();
    xglStudioMainCameraInit();
}

void xglStudioUpdate(void) {
    if (StudioEntrySkip == 0) {
        xglStudioChangeActive();
        xglStudioMove();
    }
}
