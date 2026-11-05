#include "common.h"
#include "shared.h"

typedef struct XglPacket XglPacket;

extern XglPacket *xglPacketGetCurrent(void);

/*
 * The VIF1 direct-mode packet builder object. xglPacketInit constructs two
 * of them back to back inside asPacketSource, 0x28 bytes apart (main
 * 0x0022c458-0x0022c4b4 writes both entries' +0x20 and +0x24 at that
 * stride), so sizeof(XglPacket) is 0x28. Only the members the functions of
 * this TU touch are named; everything the sceVif1Pk* library alone reads or
 * writes stays unmodeled.
 */
struct XglPacket {
    /* xglFontReloadTexture decrements this forward cursor by four words. */
    u32 *write_cursor;
    /*
     * +0x04: the address xglPacketSend hands to xglDmaDirectSrcChain as the
     * DMA source-chain start whenever a packet is pending (main
     * 0x0022c520-0x0022c548). No function of this TU writes it -- neither
     * xglPacketInit nor xglPacketMove touches +0x04 -- so it is set by the
     * sceVif1Pk* library: sceVif1PkInit (main 0x0020ac68) stores its base
     * argument at +0x00 and +0x04, and xglPacketInit passes it each entry's
     * buffer base (0x00c00000 and 0x00e00000).
     */
    u32 start;
    /*
 * +0x08 and +0x0c: xglPrimAddGifTagDirect (main 0x00222c00 lw v0,8(s0);
 * main 0x00222c0c lw v0,0xc(s0)) tests both for non-zero to decide
 * whether a direct-mode block the sceVif1Pk* library opened is still
 * open, so it appends to that block instead of resetting/opening/
 * closing a new one (compare SGsSendPacket, src/main/sv_get_image_
 * item.c, which always resets/opens/closes without this check).
 * Neither word is written by any function of this TU, so both are set
 * by the sceVif1Pk* library.
 */
    u32 directCode;
    u32 *directState;
    unsigned char unmodeled_10[0x10];
    /*
     * +0x20: the packet's fixed top-of-buffer address. xglPacketInit sets it
     * once per entry after both sceVif1PkInit calls (main 0x0022c49c for the
     * first entry, value 0x00e00000; main 0x0022c4a0 for the second, value
     * 0x01000000): the end of the buffer whose base went to sceVif1PkInit.
     */
    u32 limit;
    /*
     * +0x24: the write cursor. xglPacketInit seeds it equal to `limit` (main
     * 0x0022c48c/0x0022c490) and xglPacketMove reloads `limit` into it at
     * the start of every frame (main 0x0022c4e4/0x0022c4f0), so writers that
     * fill the packet backward from the top always restart from the same
     * address. main/tu107's nmlPacketSetAttributeData and
     * nmlPacketSetAttributeAlloc16N read it, decrement it by the size being
     * requested and store it back, then use the new value as the memcpy
     * destination or return it as the allocation (main 0x00238e28-0x00238e3c
     * and 0x00238fe0-0x00238ff4),
     * which is why the member is byte-addressed instead of scaled.
     */
    u8 *cursor;
};

/* EE DMAC channel numbers: the index into the channel table (VIF1 = 1, GIF = 2). */
#define XGL_DMA_CHANNEL_VIF1 1

void xglDmaDirectSrcChain(u32 channel, u32 address);

extern XglPacket *pCurrentPacket;
extern XglPacket *pSendPacket;

typedef float Matrix4[4][4];

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

extern void xglMatrixStackUnit(void);

void xglMatrixStackInverse(void);
void xglMatrixStackTrans(const float translation[4]);

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

typedef struct XglCamera XglCamera;
typedef struct XglCameraScreen XglCameraScreen;

/* DISPLAY register of the GS display environment at +0x18. */
typedef struct CameraDisplayEnv {
    u8 unmodeled_00[0x18];
    long display;
    u8 unmodeled_20[0x30];
} CameraDisplayEnv;

extern CameraDisplayEnv DispEnv;

void xglCameraViewScreen(StudioCamera *studio, const Vector4 *offset);
void xglCameraViewTravel(StudioCamera *studio, Vector4 *offset);
void xglCameraViewVolume(StudioCamera *studio, Vector4 *offset);

extern float tanf(float angle);
extern void xglMatrixStackInverse(void);
extern void xglMatrixStackMul(Matrix4 matrix);

extern void xglMatrixStackFrustum(float left, float right, float bottom,
                                  float top, float near, float far);

extern void xglVectorMulMat(Vector4 *destination, Matrix4 matrix,
                            const Vector4 *vector);

extern void xglPlaneParameter(Vector4 *destination, const Vector4 *p0,
                              const Vector4 *p1, const Vector4 *p2);

extern void xglVectorScale(Vector4 *destination, const Vector4 *source,
                           float scale);

extern signed char aaiVertexIndex[6][3];

typedef union CameraVector {
    Vector4 vector;
    float element[4];
} CameraVector;

typedef struct CameraTravel {
    float focus;
    float fov;
    float fov_velocity;
    float scale;
    Vector4 rotation;
    Vector4 angle_velocity;
    Vector4 angle_delta;
    Vector4 position;
    Vector4 place_velocity;
    Vector4 place_delta;
    float chase_yaw;
    float chase_distance;
    unsigned char unmodeled_78[8];
    Vector4 chase_reference;
    Vector4 chase_position;
    Vector4 frustum[4];
} CameraTravel;

struct XglCameraScreen {
    unsigned int depthMax;
    unsigned int depthMin;
    unsigned char unmodeled_08[8];
    Vector4 clipRange;
    Vector4 portEffect;
    Vector4 resolution;
    Vector4 projection;
    Vector4 window;
    Vector4 screenOffset;
    Vector4 screenScale;
};

typedef struct CameraVolume {
    CameraVector vertex[16];
    CameraVector plane[12];
    Matrix4 frustum;
    Matrix4 viewMatrix;
    unsigned char unmodeled_240[0x40];
    float planeSet[4][4][4];
} CameraVolume;

struct XglCamera {
    unsigned int active;
    unsigned int state;
    unsigned char unmodeled_08[8];
    XglCameraScreen screen;
    CameraTravel travel;
    Matrix4 transform;
    Matrix4 inverseTransform;
    unsigned char unmodeled_1f0[0x80];
    CameraVolume volume[1];
};

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001AF494);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001AF4D4);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001AF58C);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001AF5D4);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001AF714);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001AF884);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001AF8FC);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001AFAC8);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001AFCF0);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001B03D4);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001B0EEC);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001B0F40);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", xglCameraViewScreen);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", xglCameraViewTravel);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", xglCameraViewVolume);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001B1CD0);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", xglCameraMove);

void xglCameraMoveOffset(StudioCamera *camera, Vector4 *offset)
{
    xglCameraViewScreen(camera, offset);
    xglCameraViewTravel(camera, offset);
    xglCameraViewVolume(camera, offset);
}

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001B1E18);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", xglCameraClipRangeDefault);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001B2008);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001B2034);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001B2078);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001B20B0);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001B20E8);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001B2120);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001B22C4);

XglPacket *xglPacketGetCurrent(void)
{
    return pCurrentPacket;
}

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001B23DC);

INCLUDE_ASM("asm/main/nonmatchings/sdv_033", func_001B2634);

void xglPacketSend(void)
{
    if (pSendPacket != 0) {
        xglDmaDirectSrcChain(XGL_DMA_CHANNEL_VIF1, pSendPacket->start);
    }
}
