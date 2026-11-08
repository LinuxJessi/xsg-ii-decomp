#include "common.h"
#include "shared.h"
#include "main/xgl_studio.h"

typedef struct StudioCamera {
    u32 active;
    u32 state;
    u8 unmodeled_08[0x18];
    float nearClip;
    float farClip;
    u8 unmodeled_28[0x48];
    Vector4 screenOffset;
    Vector4 screenScale;
    u8 unmodeled_90[4];
    float fov;
    u8 unmodeled_98[8];
    DoublewordVector4 rotation;
    Vector4 angleVelocity;
    u8 unmodeled_c0[0x10];
    DoublewordVector4 position;
    u8 unmodeled_e0[0x40];
    DoublewordVector4 chasePosition;
    u8 unmodeled_130[0x40];
    Matrix4 cameraToWorldMatrix;
    Matrix4 occlusionTransform; /* 0x1B0: read by face_point.c's setup_occlusion/check_occlusion */
    u8 unmodeled_1f0[0x280];
    Matrix4 viewMatrix;
    u8 unmodeled_4b0[0x40];
    Matrix4 clipMatrices[2]; /* 0x4F0: the pair _ModelCalcClipInit loads into vf12..vf19 */
    u8 unmodeled_570[0x80];
} StudioCamera;

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

extern StudioCamera *xglStudioSelectGetActiveCamera(int studio_index);
extern void xglStudioFlushActiveCamera(void);
void xglCameraMove(StudioCamera *camera);

/*
 * StudioCamera (include/shared.h; config/header-canon.json,
 * decisions.StudioCamera) is the active-flag/state/camera-slot layout this
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
 * 3*i << 9). The StudioLight and StudioCamera sizes below put cameras at +0xF0
 * with a 0x5F0 stride, so reconcile this typedef before publication.
 */
typedef struct Studio {
    StudioLight light;
    StudioCamera cameras[8];
} Studio;

extern StudioLight *xglStudioGetLight2(void);
extern Studio *pCurrentStudio;
StudioCamera *xglStudioSelectGetActiveCamera(int studio_index);
int D_0069E078 = 0;

void func_001B2724(void) {
    D_0069E078 = 1;
}

void func_001B274C(void) {
    D_0069E078 = 0;
}

/* The studio records (proposed name asStudioSource, declared above). */
extern unsigned char asStudioSource[];

/* Makes studio_index the current studio: pCurrentStudio points at its record. */
void xglStudioChange(int studio_index)
{
    pCurrentStudio = (Studio *) (asStudioSource + studio_index * 0x3180);
}

/* The studio index that xglStudioChangeActive selects and xglStudioMove compares with. */
extern int activeStudioIndex;

void xglStudioSetActiveIndex(int studio_index)
{
    activeStudioIndex = studio_index;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_studio", xglStudioGetCamera);

INCLUDE_ASM("asm/main/nonmatchings/xgl_studio", xglStudioGetActiveCamera);

INCLUDE_ASM("asm/main/nonmatchings/xgl_studio", xglStudioSelectGetActiveCamera);

void xglStudioFlushActiveCamera(void)
{
    int studio_index;
    StudioCamera *camera;

    for (studio_index = 0; studio_index < 4; studio_index++) {
        camera = xglStudioSelectGetActiveCamera(studio_index);
        if (camera != 0)
            xglCameraMove(camera);
    }
}

extern int studioCameraUpdateEnabled;

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

INCLUDE_ASM("asm/main/nonmatchings/xgl_studio", xglStudioGetCamera2);

INCLUDE_ASM("asm/main/nonmatchings/xgl_studio", xglStudioMainCameraInit);

int xglStudioGetCamera2();

void xglStudioChangeActive(void) {
    int result;

    xglStudioChange(activeStudioIndex);
    /* xglStudioGetCamera2(0) is the active studio's first camera; the value is not used here. */
    result = xglStudioGetCamera2(0);
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_studio", xglStudioInit);

INCLUDE_ASM("asm/main/nonmatchings/xgl_studio", xglStudioMove);

void xglStudioInit(void);
void xglStudioMainCameraInit(void);

void xglStudioSetup(void) {
    xglStudioInit();
    xglStudioMainCameraInit();
}

void xglStudioMove(void);
extern int StudioEntrySkip;

void xglStudioUpdate(void) {
    if (StudioEntrySkip == 0) {
        xglStudioChangeActive();
        xglStudioMove();
    }
}
