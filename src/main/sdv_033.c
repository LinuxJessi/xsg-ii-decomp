#include "common.h"
#include "main/sdv_033.h"
#include "main/pad.h"
#include "shared.h"
#include "main/xgl_2.h"
#include "main/xgl_dma.h"


extern XglPacket *xglPacketGetCurrent(void);


/* EE DMAC channel numbers: the index into the channel table (VIF1 = 1, GIF = 2). */
#define XGL_DMA_CHANNEL_VIF1 1

static XglPacket *pCurrentPacket; /* .sbss 0x69E3A0 */
static XglPacket *pSendPacket; /* .sbss 0x69E3A4 */

void xglMatrixStackInverse(void);
extern void xglMatrixStackRotX(float angle);
extern void xglMatrixStackRotY(float angle);
extern void xglMatrixStackRotZ(float angle);
extern void xglMatrixStackSave(float matrix[4][4]);
void xglVectorScale(Vector4 *destination, const Vector4 *source, float scale);

#include "quadword.h"
#include "vu0_macro.h"
/* The camera draws into the GS 4096x4096 primitive space centred on 2048. */
#define CAMERA_GS_COORD_SIZE 4096.0f
#define CAMERA_GS_COORD_CENTER 2048.0f



/* DISPLAY register of the GS display environment at +0x18. */
typedef struct CameraDisplayEnv {
    u8 unmodeled_00[0x18];
    unsigned long display;
    u8 unmodeled_20[0x30];
} CameraDisplayEnv;

extern CameraDisplayEnv DispEnv;
/* The camera view functions; xglCameraMove is their caller. */
void xglCameraViewScreen(XglCamera *camera, const Vector4 *offset);
/* Builds the frustum corners and the view/inverse transforms from the screen * and travel state, then saves them into the camera. */
void xglCameraViewTravel(XglCamera *camera, Vector4 *offset);
void xglCameraViewVolume(XglCamera *camera, Vector4 *offset);
extern float tanf(float angle);
extern void xglMatrixStackMul(Matrix4 matrix);
extern void xglMatrixStackFrustum(float left, float right, float bottom,
                                  float top, float near, float far);
extern void xglVectorMulMat(Vector4 *destination, Matrix4 matrix,
                            const Vector4 *vector);
extern void xglPlaneParameter(Vector4 *destination, const Vector4 *p0,
                              const Vector4 *p1, const Vector4 *p2);
extern signed char aaiVertexIndex[6][3];


void *memset(void *, int, unsigned int);

/* Pad state forced by xglCameraSetPadOverride for one frame. */
typedef struct CameraPadOverride {
    unsigned short held;
    unsigned short pressed;
    unsigned short unknown_04;
    unsigned char unmodeled_06[2];
    signed char rightX;
    signed char rightY;
    signed char leftX;
    signed char leftY;
    int active;                  /* +0xC (cameraPadOverrideActive) */
} CameraPadOverride;

extern CameraPadOverride cameraPadOverride;

/* Clears the per-frame pad override record (cameraPadOverride, flag at +0xC). */
void xglCameraClearPadOverride(void) {
    memset(&cameraPadOverride, 0, sizeof(cameraPadOverride));
}


/* Stores seven arguments into the pad override record and raises its flag. */
void xglCameraSetPadOverride(unsigned short held, unsigned short pressed, unsigned short unknown04, unsigned char rightX, unsigned char rightY, unsigned char leftX, unsigned char leftY) {
    cameraPadOverride.held = held;
    cameraPadOverride.pressed = pressed;
    cameraPadOverride.unknown_04 = unknown04;
    cameraPadOverride.rightX = rightX;
    cameraPadOverride.rightY = rightY;
    cameraPadOverride.leftX = leftX;
    cameraPadOverride.leftY = leftY;
    cameraPadOverride.active = 1;
}

/* Resets the camera's active flag, state and pad port (+0, +4, +8). */
void xglCameraReset(XglCamera *camera) {
    XglCamera *cam = camera;
    cam->active = 0;
    cam->state = 0;
    cam->padPort = 1;
}

/* Default-data externs of this TU: Vector4 constants copied by xglVectorCopy_001B2008 (a quadword copy), and the display-size shorts. */
extern Vector4 D_00645370;
extern Vector4 D_00645380;
extern Vector4 D_00645410;
extern Vector4 D_00645420;
extern short cameraScreenWidth;
extern short cameraScreenHeight;
/* Copies a Vector4 (quadword) through $2 with a 128-bit load and store. Non-static inline: the * out-of-line definition keeps the parameters in registers, as the original does. Byte-identical * to xglVectorCopy in tu032 (proposed xglVectorCopy); named once there. */ inline void xglVectorCopy_001B2008(Vector4 *destination, const Vector4 *source);

/* Fills the XglCameraScreen block at camera+0x10 with the default screen. */
void xglCameraScreenInit(XglCamera *camera) {
    XglCameraScreen *screen = &camera->screen;

    screen->depthMax = 0x00FFFFFF;
    screen->depthMin = 0;
    xglVectorCopy_001B2008(&screen->clipRange.vector, &D_00645370);
    xglVectorCopy_001B2008(&screen->portEffect, &D_00645380);
    screen->resolution.x = cameraScreenWidth;
    screen->resolution.y = cameraScreenHeight;
    screen->resolution.z = cameraScreenWidth;
    screen->resolution.w = cameraScreenHeight;
    screen->window.x = 0.0f;
    screen->window.y = 0.0f;
    screen->window.z = cameraScreenWidth;
    screen->window.w = cameraScreenHeight;
}

extern Vector4 D_00645390;
extern Vector4 D_006453A0;
extern Vector4 D_006453B0;
extern Vector4 D_006453C0;
extern Vector4 D_006453D0;
extern Vector4 D_006453E0;
extern Vector4 D_006453F0;
extern Vector4 D_00645400;

/* Fills the CameraTravel block at camera+0x90 with the defaults (focus 5.0). */
void xglCameraTravelInit(XglCamera *camera) {
    CameraTravel *travel = &camera->travel;

    travel->focus = 5.0f;
    travel->unknown_78 = 0;
    travel->fov = 0.69813174f;
    travel->fov_velocity = 0.0f;
    travel->scale = 0.0f;
    xglVectorCopy_001B2008(&travel->rotation, &D_00645390);
    xglVectorCopy_001B2008(&travel->angle_velocity, &D_006453A0);
    xglVectorCopy_001B2008(&travel->angle_delta, &D_006453B0);
    xglVectorCopy_001B2008(&travel->position, &D_006453C0);
    xglVectorCopy_001B2008(&travel->place_velocity, &D_006453D0);
    xglVectorCopy_001B2008(&travel->place_delta, &D_006453E0);
    travel->chase_yaw = travel->rotation.y;
    travel->chase_distance = travel->position.z;
    xglVectorCopy_001B2008(&travel->chase_reference, &D_006453F0);
    xglVectorCopy_001B2008(&travel->chase_position, &D_00645400);
}

void xglCameraResetDeltas(XglCamera *camera) {
    CameraTravel *travel = &camera->travel;

    travel->scale = 0.0f;
    xglVectorCopy_001B2008(&travel->angle_delta, &D_00645410);
    xglVectorCopy_001B2008(&travel->place_delta, &D_00645420);
}

/* Zoom step from the pad: focus +-1 within 1..9, scale from two constants. */
/* One 0x20-byte block of a pad port record (PadData, 0x80 bytes per port). */
typedef struct PadBlock {
    unsigned char deadZone[4];   /* +0x0 in the analog block */
    signed char rightX;          /* +0x4 in the analog block */
    signed char rightY;
    signed char leftX;
    signed char leftY;
    unsigned short held;         /* +0x8 in the button block */
    unsigned short pressed;      /* +0xA in the button block */
    int unmodeled_0c[5];
} PadBlock;

typedef struct PadPort {
    PadBlock block[4];           /* [1] buttons, [3] analog sticks */
} PadPort;

#define PAD_BUTTONS 1
#define PAD_ANALOG 3

extern PadPort PadData[2];
/* Button halfwords of the second pad port, as separate data labels (D_0063EE28 = PadData[1] +0x28). */
extern unsigned short D_0063EE28;
extern unsigned short D_0063EE2A;
extern int cameraPadOverrideActive;
extern unsigned short D_0095A6A2;

void xglCameraApplyPadZoom(XglCamera *camera) {
    CameraTravel *travel = &camera->travel;
    unsigned short held;
    unsigned short pressed;

    held = D_0063EE28 | PadData[camera->padPort].block[PAD_BUTTONS].held;
    pressed = D_0063EE2A | PadData[camera->padPort].block[PAD_BUTTONS].pressed;
    if (cameraPadOverrideActive) {
        held = cameraPadOverride.held;
        pressed = D_0095A6A2;
    }
    if ((held & (PAD_R2 | PAD_L1 | PAD_R1)) == 0) {
        if (pressed & PAD_CIRCLE) {
            if (travel->focus < 9.0f) {
                travel->focus = travel->focus + 1.0f;
            }
        }
        if (pressed & PAD_SQUARE) {
            if (travel->focus > 1.0f) {
                travel->focus = travel->focus - 1.0f;
            }
        }
    }
    if (held & PAD_R1) {
        travel->scale = 0.0011170107f;
    }
    if (held & PAD_R2) {
        travel->scale = -0.0011170107f;
    }
}

void xglVectorScaleXYZ(Vector4 *destination, const Vector4 *source, float scale);
/* Component-wise product of two vectors (vmul.xyzw on vf20/vf21). */ void xglVectorMul(register AlignedVector4 *destination, register const AlignedVector4 *first, register const AlignedVector4 *second);
/* Byte-identical to xglVectorAddXYZ in tu032 (xglVectorAddXYZ); named once there. */ void xglVectorAddXYZ_001B20B0(register AlignedVector4 *destination, register AlignedVector4 *first, register AlignedVector4 *second);
void xglVectorScaleAddXYZ(Vector4 *destination, const float *source, float scale, const Vector4 *addend);
void xglMatrixStackUnit(void);
extern float func_002A1D08(float angle);
extern void xglMatrixStackTrans(Vector4 *translation);

/* Scales travel scale and deltas by focus-dependent factors, x4 while button 0x40 is held. */
void xglCameraScaleDeltas(XglCamera *camera) {
    CameraTravel *travel = &camera->travel;
    unsigned short held;

    held = D_0063EE28 | PadData[camera->padPort].block[PAD_BUTTONS].held;
    if (cameraPadOverrideActive) {
        held = cameraPadOverride.held;
    }
    if (held & PAD_CROSS) {
        travel->scale = travel->scale * 4.0f;
        xglVectorScaleXYZ(&travel->angle_delta, &travel->angle_delta, 4.0f);
        xglVectorScaleXYZ(&travel->place_delta, &travel->place_delta, 4.0f);
        travel->unknown_78 = travel->unknown_78 + 1;
    } else {
        travel->unknown_78 = 0;
    }
    travel->scale = travel->scale * ((travel->focus - 5.0f) * 0.08f + 1.0f);
    xglVectorScaleXYZ(&travel->angle_delta, &travel->angle_delta,
                      (travel->focus - 5.0f) * 0.08f + 1.0f);
    xglVectorScaleXYZ(&travel->place_delta, &travel->place_delta,
                      (travel->focus - 5.0f) * 0.24f + 1.0f +
                          travel->unknown_78 * 0.01f);
}

/* Pad update, free (manual) travel mode (state 2); X1: xglCameraTravelManual. */
void xglCameraTravelManual(XglCamera *camera) {
    CameraTravel *travel = &camera->travel;
    Matrix4 axes;
    short rightX = PadData[camera->padPort].block[PAD_ANALOG].rightX;
    short leftX = PadData[camera->padPort].block[PAD_ANALOG].leftX;
    short rightY = PadData[camera->padPort].block[PAD_ANALOG].rightY;
    short leftY = PadData[camera->padPort].block[PAD_ANALOG].leftY;
    unsigned char deadZone[4];
    unsigned short held;

    held = PadData[camera->padPort].block[PAD_BUTTONS].held | D_0063EE28;
    deadZone[0] = PadData[camera->padPort].block[PAD_ANALOG].deadZone[0];
    deadZone[1] = PadData[camera->padPort].block[PAD_ANALOG].deadZone[1];
    deadZone[2] = PadData[camera->padPort].block[PAD_ANALOG].deadZone[2];
    deadZone[3] = PadData[camera->padPort].block[PAD_ANALOG].deadZone[3];
    if (cameraPadOverrideActive) {
        held = cameraPadOverride.held;
        rightX = cameraPadOverride.rightX;
        leftX = cameraPadOverride.leftX;
        rightY = cameraPadOverride.rightY;
        leftY = cameraPadOverride.leftY;
        deadZone[0] = PadData[0].block[PAD_ANALOG].deadZone[0];
        deadZone[1] = PadData[0].block[PAD_ANALOG].deadZone[1];
        deadZone[2] = PadData[0].block[PAD_ANALOG].deadZone[2];
        deadZone[3] = PadData[0].block[PAD_ANALOG].deadZone[3];
    }
    /* The second pad takes over when its sticks are deflected more than the first pad's (or the override's). */
    if (((rightX < 0) ? -rightX : rightX) + ((leftX < 0) ? -leftX : leftX)
            + ((rightY < 0) ? -rightY : rightY) + ((leftY < 0) ? -leftY : leftY)
        < (((PadData[1].block[PAD_ANALOG].rightX < 0) ? -PadData[1].block[PAD_ANALOG].rightX : PadData[1].block[PAD_ANALOG].rightX)
           + ((PadData[1].block[PAD_ANALOG].leftX < 0) ? -PadData[1].block[PAD_ANALOG].leftX : PadData[1].block[PAD_ANALOG].leftX)
           + ((PadData[1].block[PAD_ANALOG].rightY < 0) ? -PadData[1].block[PAD_ANALOG].rightY : PadData[1].block[PAD_ANALOG].rightY)
           + ((PadData[1].block[PAD_ANALOG].leftY < 0) ? -PadData[1].block[PAD_ANALOG].leftY : PadData[1].block[PAD_ANALOG].leftY))) {
        rightX = PadData[1].block[PAD_ANALOG].rightX;
        leftX = PadData[1].block[PAD_ANALOG].leftX;
        rightY = PadData[1].block[PAD_ANALOG].rightY;
        leftY = PadData[1].block[PAD_ANALOG].leftY;
        deadZone[0] = PadData[1].block[PAD_ANALOG].deadZone[0];
        deadZone[1] = PadData[1].block[PAD_ANALOG].deadZone[1];
        deadZone[2] = PadData[1].block[PAD_ANALOG].deadZone[2];
        deadZone[3] = PadData[1].block[PAD_ANALOG].deadZone[3];
    }

    xglCameraResetDeltas(camera);
    xglCameraApplyPadZoom(camera);
    if (rightY < 0) {
        travel->angle_delta.x = (rightY + deadZone[1]) * -6.981317e-05f;
    }
    if (rightY > 0) {
        travel->angle_delta.x = (rightY - deadZone[1]) * -6.981317e-05f;
    }
    if (rightX < 0) {
        travel->angle_delta.y = (rightX + deadZone[0]) * -6.981317e-05f;
    }
    if (rightX > 0) {
        travel->angle_delta.y = (rightX - deadZone[0]) * -6.981317e-05f;
    }
    if (held & 0x8000) {
        travel->angle_delta.z = 0.0022340214f;
    }
    if (held & 0x2000) {
        travel->angle_delta.z = -0.0022340214f;
    }
    if (leftX < 0) {
        travel->place_delta.x = (leftX + deadZone[2]) * 0.0002f;
    }
    if (leftX > 0) {
        travel->place_delta.x = (leftX - deadZone[2]) * 0.0002f;
    }
    if (held & 0x1) {
        travel->place_delta.y = -0.0064f;
    }
    if (held & 0x4) {
        travel->place_delta.y = 0.0064f;
    }
    if (leftY < 0) {
        travel->place_delta.z = (leftY + deadZone[3]) * 0.0002f;
    }
    if (leftY > 0) {
        travel->place_delta.z = (leftY - deadZone[3]) * 0.0002f;
    }

    xglCameraScaleDeltas(camera);
    travel->fov_velocity = travel->fov_velocity * 0.8f + travel->scale;
    travel->fov = travel->fov + travel->fov_velocity;
    xglVectorScaleXYZ(&travel->angle_velocity, &travel->angle_velocity, 0.8f);
    xglVectorScaleXYZ(&travel->place_velocity, &travel->place_velocity, 0.8f);
    xglVectorAddXYZ_001B20B0(&travel->angle_velocity, &travel->angle_velocity, &travel->angle_delta);
    xglMatrixStackUnit();
    xglMatrixStackRotY(travel->rotation.y);
    xglMatrixStackSave(axes);
    xglVectorScaleAddXYZ(&travel->place_velocity, axes[0], travel->place_delta.x, &travel->place_velocity);
    travel->place_velocity.y = travel->place_velocity.y + travel->place_delta.y;
    xglVectorScaleAddXYZ(&travel->place_velocity, axes[2], travel->place_delta.z, &travel->place_velocity);
    xglVectorAddXYZ_001B20B0(&travel->rotation, &travel->rotation, &travel->angle_velocity);
    xglVectorAddXYZ_001B20B0(&travel->position, &travel->position, &travel->place_velocity);
}

void xglCameraResetDeltas(XglCamera *camera);
void xglCameraApplyPadZoom(XglCamera *camera);
/* Byte-identical to xglVectorSubXYZ_001AF3F0 in tu032 (xglVectorSubXYZ); named once there. */ void xglVectorSubXYZ_001B20E8(register AlignedVector4 *destination, register AlignedVector4 *minuend, register AlignedVector4 *subtrahend);
extern void xglVectorLength(float *length, Vector4 *vector);
extern void xglVectorNormal_001A9200(Vector4 *destination, Vector4 *source);
extern float xglPointLengthXZ(Vector4 *from, Vector4 *to);
extern float atan2f(float y, float x);
extern float xglSin(float angle);
extern float xglCos(float angle);

/* Pad button bits (PadBlock.held / pressed) that the free camera reads. */
#define CAMERA_BTN_DOWN PAD_L2
#define CAMERA_BTN_UP PAD_L1
#define CAMERA_BTN_ORBIT PAD_CROSS
#define CAMERA_BTN_MOVE_FOCUS PAD_SQUARE
#define CAMERA_BTN_LOCK PAD_START
#define HALF_PI 1.570796f
#define STICK_SCALE 0.0002f /* stick units past the dead zone to camera units */
#define PAN_STEP 0.032f     /* height step per frame, as a fraction of the distance to the focus */
#define ANALOG(port) (PadData[port].block[PAD_ANALOG])
#define BUTTONS(port) (PadData[port].block[PAD_BUTTONS])
#define ABS(x) ((x) < 0 ? -(x) : (x))

void xglCameraUpdateLookAt(XglCamera *camera) {
    CameraTravel *travel = &camera->travel;
    Vector4 lookDirection;
    Vector4 forward;
    float forwardLength; /* unused by the original after xglVectorLength */
    float distance;
    float pitchScale = 1.0f;
    short rightX = ANALOG(camera->padPort).rightX;
    short leftX = ANALOG(camera->padPort).leftX;
    short rightY = ANALOG(camera->padPort).rightY;
    short leftY = ANALOG(camera->padPort).leftY;
    unsigned char deadZone[4];
    unsigned short held;
    unsigned short pressed;
    float yaw;
    float angle;

    held = BUTTONS(camera->padPort).held | BUTTONS(1).held;
    pressed = BUTTONS(camera->padPort).pressed | BUTTONS(1).pressed;
    deadZone[0] = ANALOG(camera->padPort).deadZone[0];
    deadZone[1] = ANALOG(camera->padPort).deadZone[1];
    deadZone[2] = ANALOG(camera->padPort).deadZone[2];
    deadZone[3] = ANALOG(camera->padPort).deadZone[3];
    if (cameraPadOverride.active) {
        held = cameraPadOverride.held;
        pressed = cameraPadOverride.pressed;
        rightX = cameraPadOverride.rightX;
        leftX = cameraPadOverride.leftX;
        rightY = cameraPadOverride.rightY;
        leftY = cameraPadOverride.leftY;
        deadZone[0] = ANALOG(0).deadZone[0];
        deadZone[1] = ANALOG(0).deadZone[1];
        deadZone[2] = ANALOG(0).deadZone[2];
        deadZone[3] = ANALOG(0).deadZone[3];
    }
    /* The second pad takes over when its sticks are deflected more than the first pad's (or the override's). */
    if (ABS(rightX) + ABS(leftX) + ABS(rightY) + ABS(leftY)
        < ABS(ANALOG(1).rightX) + ABS(ANALOG(1).leftX) + ABS(ANALOG(1).rightY) + ABS(ANALOG(1).leftY)) {
        rightX = ANALOG(1).rightX;
        leftX = ANALOG(1).leftX;
        rightY = ANALOG(1).rightY;
        leftY = ANALOG(1).leftY;
        deadZone[0] = ANALOG(1).deadZone[0];
        deadZone[1] = ANALOG(1).deadZone[1];
        deadZone[2] = ANALOG(1).deadZone[2];
        deadZone[3] = ANALOG(1).deadZone[3];
    }

    xglCameraResetDeltas(camera);
    xglCameraApplyPadZoom(camera);
    xglVectorSubXYZ_001B20E8(&forward, &travel->chase_position, &travel->position);
    xglVectorLength(&forwardLength, &forward);
    xglVectorNormal_001A9200(&forward, &forward);
    distance = xglPointLengthXZ(&travel->chase_position, &travel->position);

    if (pressed & CAMERA_BTN_LOCK) {
        if (camera->lookAtLocked) {
            camera->lookAtLocked = 0;
        } else {
            camera->lookAtLocked = 1;
        }
    }
    if ((held & CAMERA_BTN_MOVE_FOCUS) || (held & CAMERA_BTN_ORBIT)) {
        if (held & CAMERA_BTN_DOWN) {
            travel->chase_position.y += distance * -PAN_STEP;
        }
        if (held & CAMERA_BTN_UP) {
            travel->chase_position.y += distance * PAN_STEP;
        }
    }
    if (!(held & CAMERA_BTN_ORBIT)) {
        if (held & CAMERA_BTN_DOWN) {
            travel->position.y += distance * -PAN_STEP;
        }
        if (held & CAMERA_BTN_UP) {
            travel->position.y += distance * PAN_STEP;
        }
    }
    if (leftX < 0) {
        travel->place_delta.x = (leftX + deadZone[2]) * STICK_SCALE;
    }
    if (leftX > 0) {
        travel->place_delta.x = (leftX - deadZone[2]) * STICK_SCALE;
    }
    if (travel->place_delta.x != 0.0f) {
        if ((held & CAMERA_BTN_MOVE_FOCUS) && !(held & CAMERA_BTN_ORBIT)) {
            yaw = atan2f(forward.x, forward.z);
            angle = HALF_PI;
            travel->position.x += xglSin(yaw - angle) * travel->place_delta.x * distance;
            travel->position.z += xglCos(yaw - angle) * travel->place_delta.x * distance;
            travel->chase_position.x += xglSin(yaw - angle) * travel->place_delta.x * distance;
            travel->chase_position.z += xglCos(yaw - angle) * travel->place_delta.x * distance;
        }
        if (!(held & CAMERA_BTN_ORBIT) && !(held & CAMERA_BTN_MOVE_FOCUS)) {
            angle = atan2f(forward.x, forward.z);
            angle = angle + travel->place_delta.x;
            travel->position.x = travel->chase_position.x - xglSin(angle) * distance;
            travel->position.z = travel->chase_position.z - xglCos(angle) * distance;
        }
        if ((held & CAMERA_BTN_ORBIT) && !(held & CAMERA_BTN_MOVE_FOCUS)) {
            angle = atan2f(forward.x, forward.z);
            angle = angle - travel->place_delta.x;
            travel->chase_position.x = travel->position.x + xglSin(angle) * distance;
            travel->chase_position.z = travel->position.z + xglCos(angle) * distance;
        }
    }
    if (leftY < 0) {
        travel->place_delta.z = (leftY + deadZone[3]) * STICK_SCALE;
    }
    if (leftY > 0) {
        travel->place_delta.z = (leftY - deadZone[3]) * STICK_SCALE;
    }
    if ((held & CAMERA_BTN_MOVE_FOCUS) || (held & CAMERA_BTN_ORBIT)) {
        travel->chase_position.x -= forward.x * travel->place_delta.z * distance * pitchScale;
        travel->chase_position.y -= forward.y * travel->place_delta.z * distance * pitchScale;
        travel->chase_position.z -= forward.z * travel->place_delta.z * distance * pitchScale;
    }
    if (!(held & CAMERA_BTN_ORBIT)) {
        travel->position.x -= forward.x * travel->place_delta.z * distance * pitchScale;
        travel->position.y -= forward.y * travel->place_delta.z * distance * pitchScale;
        travel->position.z -= forward.z * travel->place_delta.z * distance * pitchScale;
    }

    xglVectorSubXYZ_001B20E8(&lookDirection, &travel->chase_position, &travel->position);
    xglVectorNormal_001A9200(&lookDirection, &lookDirection);
    lookDirection.z = -lookDirection.z;
    travel->rotation.y = atan2f(lookDirection.x, lookDirection.z);
    travel->rotation.x = atan2f(lookDirection.y, xglSin(travel->rotation.y) * lookDirection.x + xglCos(travel->rotation.y) * lookDirection.z);
    travel->rotation.y = -travel->rotation.y;
}

/* Stores the pad port index at camera+8 (values of 2 or more become 1). */
void xglCameraSetPadPort(XglCamera *camera, int port) {
    camera->padPort = port;
    if (camera->padPort >= 2) {
        camera->padPort = 1;
    }
}

/* Per-frame camera update: pad read, override, state dispatch, then clears the override. */
void xglCameraUpdate(XglCamera *camera) {
    unsigned short held;
    unsigned short pressed;

    held = D_0063EE28 | PadData[camera->padPort].block[PAD_BUTTONS].held;
    pressed = D_0063EE2A | PadData[camera->padPort].block[PAD_BUTTONS].pressed;
    if (cameraPadOverrideActive) {
        held = cameraPadOverride.held;
        pressed = cameraPadOverride.pressed;
    }
    switch (camera->state) {
    case 0:
        return;
    case 1:
        break;
    case 2:
        if (held & 0x100) {
            if (pressed & 0x10) {
                xglCameraTravelInit(camera);
            }
        } else {
            xglCameraTravelManual(camera);
        }
        break;
    case 3:
        if (held & 0x100) {
            if (pressed & 0x10) {
                xglCameraTravelInit(camera);
            }
        } else {
            xglCameraUpdateLookAt(camera);
        }
        break;
    case 4:
        break;
    default:
        break;
    }
    xglCameraClearPadOverride();
}

/* The camera view functions; xglCameraMove is their caller. */
void xglCameraViewScreen(XglCamera *camera, const Vector4 *offset) {
    XglCameraScreen *screen = &camera->screen;
    float scale;
    float depthA;
    float depthB;
    Vector4 scaled;
    float offsetX;
    float offsetY;

    offsetX = (screen->window.z + screen->window.x) * 0.5f - screen->resolution.x * 0.5f;
    offsetY = (screen->window.w + screen->window.y) * 0.5f - screen->resolution.y * 0.5f;
    scale = (135.0f / 11.0f) / (54.0f / (float)(((int)(DispEnv.display >> 23) & 0xF) + 1));
    depthA = (float)screen->depthMin;
    depthB = (float)(unsigned int)(screen->depthMax - screen->depthMin);
    if (screen->resolution.z > screen->resolution.w) {
        xglVectorScale(&scaled, &screen->resolution,
                       (CAMERA_GS_COORD_SIZE - __builtin_fabsf(2.0f * offsetX)) / screen->resolution.z);
    } else {
        xglVectorScale(&scaled, &screen->resolution,
                       (CAMERA_GS_COORD_SIZE - __builtin_fabsf(2.0f * offsetY)) / screen->resolution.w);
    }
    screen->clipRange.vector.z = screen->projection.x * (scaled.z / screen->resolution.z);
    screen->clipRange.vector.w = screen->projection.y * (scaled.w / screen->resolution.w);
    xglVectorCopy_001B2008(&screen->projection, &screen->resolution);
    screen->projection.x = screen->projection.x * scale;
    screen->projection.z = screen->projection.z * scale;
    screen->screenOffset.x = offsetX + CAMERA_GS_COORD_CENTER;
    screen->screenOffset.y = offsetY + CAMERA_GS_COORD_CENTER;
    screen->screenOffset.z = depthA + depthB * 0.5f;
    screen->screenOffset.w = (1.0f - screen->portEffect.z) * 255.0f;
    screen->screenScale.x = (scaled.x * screen->portEffect.x) * 0.5f;
    screen->screenScale.y = -((scaled.y * screen->portEffect.y) * 0.5f);
    screen->screenScale.z = -(depthB * 0.5f);
    screen->screenScale.w = (screen->portEffect.z * screen->portEffect.w) * 255.0f;
}

/* Builds the frustum corners and the view/inverse transforms from the screen
 * and travel state, then saves them into the camera. */
void xglCameraViewTravel(XglCamera *camera, Vector4 *offset) {
    XglCameraScreen *screen = &camera->screen;
    CameraTravel *travel = &camera->travel;
    AlignedVector4 scale;
    AlignedVector4 aspect;
    AlignedVector4 position;

    scale.x = func_002A1D08(travel->fov * 0.5f);
    scale.y = scale.x;
    scale.z = scale.x * (screen->clipRange.vector.w / screen->projection.y);
    scale.w = scale.z;
    aspect.x = screen->projection.x / screen->projection.y;
    aspect.y = aspect.x;
    aspect.z = screen->clipRange.vector.z / screen->clipRange.vector.w;
    aspect.w = aspect.z;
    travel->frustum[3].x = screen->clipRange.vector.x;
    travel->frustum[3].y = screen->clipRange.vector.y;
    travel->frustum[3].z = travel->frustum[3].x;
    travel->frustum[3].w = travel->frustum[3].y;
    xglVectorMul(&travel->frustum[3], &travel->frustum[3], &scale);
    xglVectorScale(&travel->frustum[2], &travel->frustum[3], -1.0f);
    xglVectorMul(&travel->frustum[1], &travel->frustum[3], &aspect);
    xglVectorScale(&travel->frustum[0], &travel->frustum[1], -1.0f);
    xglMatrixStackUnit();
    xglVectorSubXYZ_001B20E8(&position, &travel->position, offset);
    xglMatrixStackTrans(&position);
    xglMatrixStackRotY(travel->rotation.y);
    xglMatrixStackRotX(travel->rotation.x);
    xglMatrixStackRotZ(travel->rotation.z);
    xglMatrixStackSave(travel->transform);
    xglMatrixStackInverse();
    xglMatrixStackSave(travel->inverseTransform);
}

void xglMatrixCopy_001B2034(Matrix4 *destination, const Matrix4 *source);
extern void xglMatrixStackFrustum(float, float, float, float, float, float);

void xglCameraViewVolume(XglCamera *camera, Vector4 *offset) {
    XglCameraScreen *screen;
    CameraTravel *travel;
    CameraVolume *volume;
    AlignedVector4 corner;
    int x;
    int b;
    int y;
    int i;
    int j;
    int k;

    screen = &camera->screen;
    travel = &camera->travel;
    for (i = 0; i <= 0; i++) {
        volume = &camera->volume[i];
        xglMatrixCopy_001B2034(volume->frustum, (Matrix4 *)travel->frustum);
        xglMatrixStackUnit();
        xglMatrixStackFrustum(volume->frustum[0][2], volume->frustum[1][2], volume->frustum[2][2],
                      volume->frustum[3][2], screen->clipRange.vector.x, screen->clipRange.vector.y);
        xglMatrixStackMul(travel->inverseTransform);
        xglMatrixStackSave(volume->viewMatrix);
        for (j = 0; j < 2; j++) {
            for (k = 0; k < 8; k++) {
                x = k & 1;
                b = ((k & 2) >> 1) | 2;
                y = ((k & 4) >> 2) | ((j & 1) << 1);
                corner.x = volume->frustum[x][y];
                corner.y = volume->frustum[b][y];
                corner.z = -screen->clipRange.element[y & 1];
                corner.w = 1.0f;
                xglVectorMulMat(&volume->vertex[j * 8 + k].vector, travel->transform, &corner);
            }
            {
            for (k = 0; k < 6; k++) {
                xglPlaneParameter(&(volume->plane + j * 6)[k].vector,
                                  &volume->vertex[j * 8 + aaiVertexIndex[k][0]].vector,
                                  &volume->vertex[j * 8 + aaiVertexIndex[k][1]].vector,
                                  &volume->vertex[j * 8 + aaiVertexIndex[k][2]].vector);
            }
            }
        }
        {
        for (j = 0; j < 4; j++) {
            for (k = 0; k < 3; k++) {
                volume->planeSet[0][j][k] = volume->plane[k].element[j];
                volume->planeSet[1][j][k] = volume->plane[k + 3].element[j];
                volume->planeSet[2][j][k] = volume->plane[k + 6].element[j];
                volume->planeSet[3][j][k] = volume->plane[k + 9].element[j];
            }
        }
        }
    }
}

/* Per-camera init, called by xglStudioInit: reset, screen, travel, then camera+0x5F4 = 1.0f. */
void xglCameraInit(XglCamera *camera) {
    xglCameraReset(camera);
    xglCameraScreenInit(camera);
    xglCameraTravelInit(camera);
    camera->unknown_5f4 = 1.0f;
}

void xglCameraMove(XglCamera *camera) {
    Vector4 offset;

    offset = (Vector4){0, 0, 0, 0};
    xglCameraViewScreen(camera, &offset);
    xglCameraViewTravel(camera, &offset);
    xglCameraViewVolume(camera, &offset);
}

void xglCameraMoveOffset(XglCamera *camera, Vector4 *offset)
{
    xglCameraViewScreen(camera, offset);
    xglCameraViewTravel(camera, offset);
    xglCameraViewVolume(camera, offset);
}

/* Sets camera->screen.window (x0, y0, x1, y1), sorting each pair and clamping to the screen size. */
void xglCameraSetWindow(XglCamera *camera, int x0, int y0, int x1, int y1) {
    if (camera) {
        int temp;
        XglCameraScreen *screen = &camera->screen;

        if (x0 > x1) {
            temp = x1;
            x1 = x0;
            x0 = temp;
        }
        if (y0 > y1) {
            temp = y1;
            y1 = y0;
            y0 = temp;
        }
        if (x0 < 0) {
            x0 = 0;
        }
        if (x0 >= cameraScreenWidth) {
            x0 = cameraScreenWidth;
        }
        if (x1 < 0) {
            x1 = 0;
        }
        if (x1 >= cameraScreenWidth) {
            x1 = cameraScreenWidth;
        }
        if (y0 < 0) {
            y0 = 0;
        }
        if (y0 >= cameraScreenHeight) {
            y0 = cameraScreenHeight;
        }
        if (y1 < 0) {
            y1 = 0;
        }
        if (y1 >= cameraScreenHeight) {
            y1 = cameraScreenHeight;
        }
        screen->window.x = x0;
        screen->window.y = y0;
        screen->window.z = x1;
        screen->window.w = y1;
    }
}

void xglCameraClipRangeDefault(XglCamera *camera) {
    camera->screen.clipRange.vector.x = 0.08f;
    camera->screen.clipRange.vector.y = 12000.0f;
}

/* Copies a Vector4 (quadword) through $2 with a 128-bit load and store. Non-static inline: the
 * out-of-line definition keeps the parameters in registers, as the original does. Byte-identical
 * to xglVectorCopy in tu032 (proposed xglVectorCopy); named once there. */
inline void xglVectorCopy_001B2008(Vector4 *destination, const Vector4 *source)
{
    QuadwordCopy(destination, source);
}

/* Copies a 4x4 float matrix (four quadwords) through the $2 quadword register. */
void xglMatrixCopy_001B2034(register Matrix4 *destination, register const Matrix4 *source) {
    QuadwordCopyMatrix(destination, source);
}

/* Component-wise product of two vectors (vmul.xyzw on vf20/vf21). */
void xglVectorMul(register AlignedVector4 *destination, register const AlignedVector4 *first,
                  register const AlignedVector4 *second) {
    __asm__ __volatile__(
        "lqc2 vf20, 0(%1)\n"
        "lqc2 vf21, 0(%2)\n"
        "vmul.xyzw vf20, vf20, vf21\n"
        "sqc2 vf20, 0(%0)\n"
        : : "r"(destination), "r"(first), "r"(second) : "memory");
}

/* Byte-identical to xglVectorAddXYZ in tu032 (xglVectorAddXYZ); named once there. */
void xglVectorAddXYZ_001B20B0(register AlignedVector4 *destination, register AlignedVector4 *first, register AlignedVector4 *second)
{
    Vu0AddVectorXYZ(destination, first, second);
}

/* Byte-identical to xglVectorSubXYZ_001AF3F0 in tu032 (xglVectorSubXYZ); named once there. */
void xglVectorSubXYZ_001B20E8(register AlignedVector4 *destination, register AlignedVector4 *minuend, register AlignedVector4 *subtrahend)
{
    Vu0SubVectorXYZ(destination, minuend, subtrahend);
}

/* Texture transfer set (X1: xglPacketTextureTrans): per entry a GS load-image header (BITBLTBUF/TRXREG words) plus a reference to the image data. */
typedef struct XglTextureTransEntry {
    u32 size;       /* low 16 bits: width in pixels; high 16 bits: width in 64-pixel blocks when it does not fit */
    u32 height;
    u32 vramOffset; /* added to the texture buffer base << 11, then >> 6 for the GS block address */
    u32 dataSize;   /* +0x0C: quadword count for sceVif1PkRef */
    u32 dataOffset; /* +0x10: from the set base */
} XglTextureTransEntry;

typedef struct XglTextureTransSet {
    u8 unmodeled_00[8];
    int count;          /* +0x08 */
    u32 entryOffset;    /* +0x0C */
} XglTextureTransSet;

extern u16 sRenderTextureBase;
extern u64 D_00645480;
extern u64 D_006454A0;
extern u32 D_00645450[];
void sceVif1PkAddDataN(XglPacket *packet, void *data, unsigned int qwc);
void sceVif1PkRef(XglPacket *packet, void *data, unsigned int qwc, unsigned int a3, unsigned int a4, unsigned int a5);

void xglPacketTextureTrans(XglTextureTransSet *set) {
    XglTextureTransEntry *entry;
    int i;
    int blockWidth;
    int width;

    entry = (XglTextureTransEntry *)((u8 *)set + set->entryOffset);
    for (i = 0; i < set->count; i++) {
        width = entry->size & 0xFFFF;
        if (entry->size == width) {
            blockWidth = (width + 63) / 64;
        } else {
            blockWidth = entry->size >> 16;
        }
        D_00645480 = ((u64)(((sRenderTextureBase << 11) + entry->vramOffset) >> 6) << 32) | ((u64)blockWidth << 48);
        D_006454A0 = width | ((u64)entry->height << 32);
        sceVif1PkCnt(pCurrentPacket, 0);
        sceVif1PkAddDataN(pCurrentPacket, D_00645450, 28);
        sceVif1PkRef(pCurrentPacket, (u8 *)set + entry->dataOffset, entry->dataSize, 0, 0, 0);
        entry++;
    }
}

extern u16 sRenderDisplayBase;
extern u32 D_006454C0[];
extern u32 D_00645510[];
void sceVif1PkCnt(XglPacket *packet, unsigned int count);
void func_0012B9A0(XglPacket *packet, unsigned int tag);
void sceVif1PkOpenDirectHLCode(XglPacket *packet, unsigned int count);
void sceVif1PkAddDirectDataN(XglPacket *packet, void *data, unsigned int qwc);
void func_0012B818(XglPacket *packet);

/* Direct-mode VIF1 block with a GS tag word; only caller xglRenderMove. X1: xglPacketInterpolate (TEX0_1 from the display buffer when ScanLineInterpolate == 2). */
void xglPacketInterpolate(void) {
    XglPacket *packet = pCurrentPacket;
    u64 *scratch;

    sceVif1PkCnt(packet, 0);
    func_0012B9A0(packet, 0x11000000);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, D_006454C0, 4);
    scratch = (u64 *)0x70000000;
    scratch[0] = ((sRenderDisplayBase << 5) | 0x24020000) | ((u64)0xC800 << 19);
    scratch[1] = 6;
    sceVif1PkAddDirectDataN(packet, scratch, 1);
    sceVif1PkAddDirectDataN(packet, D_00645510, 7);
    func_0012B818(packet);
}

XglPacket *xglPacketGetCurrent(void)
{
    return pCurrentPacket;
}

/* Packet system init: both buffers through func_0012B498, pCurrentPacket = D_0095A6B0, pSendPacket = 0. */
extern XglPacket D_0095A6B0;
extern XglPacket D_0095A6E0;
void func_0012B498(XglPacket *packet, u32 buffer);

void xglPacketSystemInit(u32 buffer, u32 size, float rate)
{
    float ratio = rate;
    int reserve = 0;

    if (ratio < 1.0f) {
        ratio = 1.0f;
    }
    if (ratio > 99.0f) {
        ratio = 99.0f;
    }
    reserve = (size >> 1) * ratio / 100.0f;
    if (reserve & 0xF) {
        reserve = reserve - (reserve & 0xF) + 0x10;
    }
    func_0012B498(&D_0095A6B0, buffer);
    func_0012B498(&D_0095A6E0, (size >> 1) + buffer);
    D_0095A6B0.bufferSize = size >> 1;
    D_0095A6E0.bufferSize = size >> 1;
    D_0095A6B0.buffer = buffer;
    D_0095A6E0.buffer = buffer + (size >> 1);
    D_0095A6B0.limit = (size >> 1) + buffer - reserve;
    D_0095A6E0.limit = (size >> 1) + buffer + (size >> 1) - reserve;
    D_0095A6B0.cursor = (u8 *) D_0095A6B0.limit;
    D_0095A6E0.cursor = (u8 *) D_0095A6E0.limit;
    pCurrentPacket = &D_0095A6B0;
    pSendPacket = 0;
}

void FlushCache(int cacheOp);
void func_0012B4A8(XglPacket *packet);
u32 *func_0012B4B8(XglPacket *packet);
void func_0012B688(XglPacket *packet, unsigned int tagFlags);
extern XglPacket D_0095A6B0;
extern XglPacket D_0095A6E0;

/* Ends the current packet, makes it the send packet and switches to the other buffer. */
void xglPacketSwapBuffers(void) {
    func_0012B688(pCurrentPacket, 0);
    func_0012B4B8(pCurrentPacket);
    FlushCache(0);
    pCurrentPacket->cursor = pCurrentPacket->limit;
    pSendPacket = pCurrentPacket;
    if (pCurrentPacket == &D_0095A6E0) {
        pCurrentPacket = &D_0095A6B0;
    } else {
        pCurrentPacket = &D_0095A6E0;
    }
    func_0012B4A8(pCurrentPacket);
}

void xglPacketSend(void)
{
    if (pSendPacket != 0) {
        xglDmaDirectSrcChain(XGL_DMA_CHANNEL_VIF1, pSendPacket->start);
    }
}
