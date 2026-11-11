#include "common.h"
#include "main/sdv_033.h"
#include "main/tu045.h"
#include "gs.h"
#include "shared.h"

/* A studio camera. */


/* The .sdata block of this unit, 0x69E0D8-0x69E12C, defined in address order so
 * that every access below is $gp-relative. */
int xglStudioSelectedIndex = 0;
int pxyCurrentDrawPass = 0;
XglCamera *xglStudioSelectedCamera = 0;
int xglPacketMode = -1;
struct PxyPacket *pxyBoneCachePacket = 0;
struct XglPacketTable *pxyTextureTableCache = 0;
int pxyTextureChannelCache = -1;
struct PxyPacket *pxyUploadedPacket = 0;
int pxyUploadMode = 0;
int pxyPartStyle = 0;
int pxySubmitCount = 0;
int xglDirectQueueRefOffset = 0;
int pxyReflectionType = 0;
int D_0069E10C = 0;
int pxyFlushDone = 0;
int xglPacketMode3Flag = 0;
int xglPacketMode4Flag = 0;
int pxyAlphaRef = 117;
int pxyRotateFreeze = 0;
int xglDirectQueueCount = 0;
XglDirectQueueEntry *xglDirectQueueBuffer = 0;


typedef int s32;

void xglMatrixUnit(void *object);
void xglVectorZero_001D4508(register Vector4 *destination);
extern s32 pxyReflectionMatrix;
extern s32 D_00989A40;

void func_001CCAA0(void)
{
    xglMatrixUnit(&pxyReflectionMatrix);
    xglVectorZero_001D4508(&D_00989A40);
}

extern u8 D_00673150[];
extern u8 D_00673160[];
extern u8 D_00677000[];
extern u8 D_00677010[];
extern u8 D_0067A880[];
extern u8 D_0067A890[];
extern u8 D_0067D9A0[];
extern u8 D_0067D9B0[];
extern u8 D_006819B0[];
extern u8 D_006819C0[];
extern u8 D_2C90[];
extern u8 D_3290[];
extern u8 D_3478[];
extern u8 D_39D0[];
extern u8 D_3B28[];
extern u8 D_D48[];
extern u8 D_D60[];
extern u8 D_D70[];
extern u8 D_D88[];
extern u8 D_DB0[];
extern u8 D_DD8[];

/* Microcode table lookup for the current xglPacketMode (0..4): kind 0 = program data, 1 = program size, 2 = model kick address,
 * 3 = GS kick address, 4 = wide GS kick address (the last three >> 3, for MSCAL). Packet mode 0 plain, 1 env, 2 texture pass,
 * 3 = xglPacketMode3Flag, 4 = xglPacketMode4Flag (drop pass in X1). */
u8 *xglPacketMicrocodeEntry(int kind)
{
    switch (kind) {
    case 0:
        if (xglPacketMode == 0) {
            return D_00673160;
        }
        if (xglPacketMode == 1) {
            return D_00677010;
        }
        if (xglPacketMode == 2) {
            return D_0067A890;
        }
        if (xglPacketMode == 3) {
            return D_0067D9B0;
        }
        if (xglPacketMode == 4) {
            return D_006819C0;
        }
        break;
    case 1:
        if (xglPacketMode == 0) {
            return *(u8 **) D_00673150;
        }
        if (xglPacketMode == 1) {
            return *(u8 **) D_00677000;
        }
        if (xglPacketMode == 2) {
            return *(u8 **) D_0067A880;
        }
        if (xglPacketMode == 3) {
            return *(u8 **) D_0067D9A0;
        }
        if (xglPacketMode == 4) {
            return *(u8 **) D_006819B0;
        }
        break;
    case 2:
        if (xglPacketMode == 0) {
            return (u8 *) ((u32) D_39D0 >> 3);
        }
        if (xglPacketMode == 1) {
            return (u8 *) ((u32) D_3478 >> 3);
        }
        if (xglPacketMode == 2) {
            return (u8 *) ((u32) D_2C90 >> 3);
        }
        if (xglPacketMode == 3) {
            return (u8 *) ((u32) D_3B28 >> 3);
        }
        if (xglPacketMode == 4) {
            return (u8 *) ((u32) D_3290 >> 3);
        }
        break;
    case 3:
        if (xglPacketMode == 0) {
            return (u8 *) ((u32) D_D60 >> 3);
        }
        if (xglPacketMode == 1) {
            return (u8 *) ((u32) D_DB0 >> 3);
        }
        if (xglPacketMode == 2) {
            return (u8 *) ((u32) D_D60 >> 3);
        }
        if (xglPacketMode == 3) {
            return (u8 *) ((u32) D_D60 >> 3);
        }
        if (xglPacketMode == 4) {
            return (u8 *) ((u32) D_D48 >> 3);
        }
        break;
    case 4:
        if (xglPacketMode == 0) {
            return (u8 *) ((u32) D_D88 >> 3);
        }
        if (xglPacketMode == 1) {
            return (u8 *) ((u32) D_DD8 >> 3);
        }
        if (xglPacketMode == 2) {
            return (u8 *) ((u32) D_D88 >> 3);
        }
        if (xglPacketMode == 3) {
            return (u8 *) ((u32) D_D88 >> 3);
        }
        if (xglPacketMode == 4) {
            return (u8 *) ((u32) D_D70 >> 3);
        }
        break;
    }
    return 0;
}


/*
 * Sets the rotation freeze flag: pxyUpdateReflectionRotation adds no angle step while the flag is
 * nonzero. pxyFlush passes pxyFreezeAnimation.
 */
void pxySetRotateFreeze(s32 freeze)
{
    pxyRotateFreeze = freeze;
}


void func_001CCEB4(s32 value)
{
    pxyAlphaRef = value;
}


void pxySetFlushDone(void)
{
    pxyFlushDone = 1;
}

void pxyClearFlushDone(void)
{
    pxyFlushDone = 0;
}


void xglPacketSetMode3Flag(s32 value)
{
    xglPacketMode3Flag = value;
}


void xglPacketSetMode4Flag(s32 value)
{
    xglPacketMode4Flag = value;
}

extern Vector4 D_009899F0;

void func_001CCF84(const Vector4 *colorScale)
{
    D_009899F0 = *colorScale;
}

extern f32 pxyReflectionAngle;
void xglMatrixStackUnit(void);
void xglMatrixStackRotX(f32 angle);
void xglMatrixStackRotY(f32 angle);
void xglMatrixStackSave(void *object);
f32 sinf(f32 x);

/* Reflection mapping rotation (X1 s_inReflRotX/Y): advances pxyReflectionAngle by 0.1 unless frozen, wraps at 2*pi and saves
 * RotY(angle) * RotX(sin(angle)) in pxyReflectionMatrix. */
void pxyUpdateReflectionRotation(s32 reflect)
{
    if (reflect != 0) {
        pxyReflectionType = 1;
    } else {
        pxyReflectionType = 0;
    }
    if (pxyRotateFreeze == 0) {
        pxyReflectionAngle += 0.1f;
    }
    if (pxyReflectionAngle >= 6.283184f) {
        pxyReflectionAngle -= 6.283184f;
    }
    xglMatrixStackUnit();
    xglMatrixStackRotX(sinf(pxyReflectionAngle));
    xglMatrixStackRotY(pxyReflectionAngle);
    xglMatrixStackSave(&pxyReflectionMatrix);
}

XglCamera *xglStudioSelectGetActiveCamera(s32 index);

/* Stores the studio index and caches that studio's active camera (xglStudioSelectedCamera). */
void xglStudioSelect(s32 index)
{
    xglStudioSelectedIndex = index;
    xglStudioSelectedCamera = xglStudioSelectGetActiveCamera(index);
}


void func_001CD108(s32 value)
{
    pxyCurrentDrawPass = value;
}


void func_001CD134(void)
{
    pxyTextureTableCache = 0;
    pxyTextureChannelCache = -1;
}


void func_001CD160(void)
{
    pxyBoneCachePacket = 0;
}

void xglPacketModeReset(void)
{
    xglPacketMode = -1;
}

typedef struct PxyPacket PxyPacket;

/* The model packet whose state pxyUploadPacketState last uploaded (0 after a reset). */

void func_001CD1AC(void)
{
    pxyUploadedPacket = 0;
}

void xglPacketInitVuStateArg(int value)
{
    xglPacketInitVuState();
}

void xglPacketInitVuState(void);

void xglPacketStateReset(void)
{
    xglPacketModeReset();
    func_001CD134();
    func_001CD1AC();
    func_001CD160();
    xglPacketInitVuState();
}

/* Scratchpad words that pxyUploadPacketState hands to the packet callees: the current model packet, and a cleared flag. */
#define SPR_CURRENT_PXY (*(PxyPacket **) 0x70000010)
#define SPR_PXY_FLAG (*(s32 *) 0x7000001C)
#define SPR_PXY_CHANNEL (*(PxyChannel **) 0x7000001C)

typedef unsigned int Quadword __attribute__((mode(TI)));

typedef struct PxyHeader {
    u8 unk00[8];
    u32 flags; /* +0x08 */
    u8 unk0C[0x5C - 0x0C];
    s32 matrixCount; /* +0x5C */
    u8 unk60[0x90 - 0x60];
    float minX; /* +0x90 */
    u8 unk94[0x98 - 0x94];
    float minZ; /* +0x98 */
    u8 unk9C[0xA0 - 0x9C];
    float maxX; /* +0xA0 */
    u8 unkA4[0xA8 - 0xA4];
    float maxZ; /* +0xA8 */
} PxyHeader;

typedef union PxyQword {
    u64 reg[2];
    Vector4 vec;
} PxyQword;

typedef struct PxyClipPlanes {
    Vector4 plane[2];  /* +0x00 */
    float depth;       /* +0x20 */
    float range;       /* +0x24 */
} PxyClipPlanes;

/* The matrix bank of a model: view rows, eight rows whose w words hold the translation, then light axes and colours. */
typedef struct XglMatrixBank {
    u8 unmodeled_00[0x30];
    Vector4 viewRows[4];    /* +0x30 */
    Vector4 rows[8];        /* +0x70 */
    u8 unmodeled_f0[0x120 - 0xF0];
    Vector4 lightAxis[3];   /* +0x120 */
    Vector4 lightColor[3];  /* +0x150 */
} XglMatrixBank;

/* The 64 bytes of a matrix as eight doublewords, so a copy moves them with ld/sd. */
typedef struct XglMatrixWords {
    u64 w[8];
} XglMatrixWords;

/* The model packet. The scratchpad word at 0x70000010 points at the one being drawn; every function below views the same
 * object (render state, bounds header, matrix banks, fog colour, shadow position). */
struct PxyPacket {
    u32 renderStatus;       /* +0x00 */
    int camera;             /* +0x04 */
    int drawMode;           /* +0x08 */
    u8 unk0C[0x10 - 0x0C];
    PxyHeader *header;      /* +0x10 */
    u8 unk14[0x1C - 0x14];
    u8 *channelTable;       /* +0x1C: 0x70-byte record per matrix index, its bone ids from +0x30 */
    u8 unk20[0x2C - 0x20];
    u8 *matrixTable;              /* +0x2C */
    u8 unk30[0x38 - 0x30];
    struct XglPacketTable *textureTable; /* +0x38 */
    struct PxyTableEntry *entries; /* +0x3C */
    u8 unk40[0x44 - 0x40];
    float alpha;            /* +0x44 */
    u8 unk48[0x4C - 0x48];
    float glowScale;            /* +0x4C */
    u8 unk50[0x58 - 0x50];
    float glowIntensity;            /* +0x58 */
    Matrix4 *boneMatrices;  /* +0x5C */
    u8 unk60[0x68 - 0x60];
    s32 (*levelTable)[4];            /* +0x68 */
    XglMatrixBank *matrixBank;   /* +0x6C */
    float unk70[4];         /* +0x70 */
    Matrix4 *shadowCamera;  /* +0x80: matrices of the shadow camera, the view at index 3 */
    u32 drawFlags;              /* +0x84 */
    u8 unk88[0x94 - 0x88];
    float shadowY;            /* +0x94 */
    void *shadowMatrix;            /* +0x98 */
    u8 unk9C[0xA4 - 0x9C];
    s16 planeCount;         /* +0xA4 */
    s16 reflectMode;              /* +0xA6 */
    PxyClipPlanes *planes;  /* +0xA8 */
    u8 unkAC[0xB0 - 0xAC];
    float reflectBlend;            /* +0xB0 */
    u8 *reflectMask;              /* +0xB4 */
    float reflectScale;            /* +0xB8 */
    float reflectExtra;            /* +0xBC */
    u8 unkC0[0xD8 - 0xC0];
    float uniformScale;            /* +0xD8 */
    u8 unkDC[0xE0 - 0xDC];
    Matrix4 localMatrix;    /* +0xE0 */
    XglMatrixWords worldMatrix;  /* +0x120 */
    u8 unk160[0x180 - 0x160];
    s32 cameraLevel[4];          /* +0x180: per studio camera, out of 4080 */
    float fogColorR;        /* +0x190 */
    float fogColorG;        /* +0x194 */
    float fogColorB;        /* +0x198 */
    u8 unk19C[0x1A0 - 0x19C];
    float unk1A0[4];        /* +0x1A0 */
    Vector4 shadowPosition;  /* +0x1B0 */
    u8 unk1C0[0x1E0 - 0x1C0];
    float unk1E0[8][4];     /* +0x1E0 */
    float unk260[4][4];     /* +0x260 */
};

/* The channel (per-part record) at scratchpad 0x7000001C. */
typedef struct PxyChannel {
    u8 unk00[4];
    s16 entryIndex; /* +0x04 */
    u8 unk06[2];
    signed char channelIndex; /* +0x08 */
    u8 unk09[0x10 - 0x09];
    s16 matrixIndex; /* +0x10 */
    u8 unk12[0x18 - 0x12];
    u32 flags; /* +0x18 */
    signed char styleLevel; /* +0x1C */
} PxyChannel;

typedef union GsDisplay64 {
    u64 data;
    struct {
        unsigned int dx : 12;
        unsigned int dy : 11;
        unsigned int magh : 4;
        unsigned int magv : 2;
        unsigned int unmodeled_29 : 3;
        unsigned int dw : 12;
        unsigned int dh : 11;
        unsigned int unmodeled_55 : 9;
    };
} GsDisplay64;

typedef struct DispEnvView {
    u8 unmodeled_00[0x18];
    GsDisplay64 display1; /* +0x18 */
} DispEnvView;

extern DispEnvView DispEnv;
extern s16 cameraScreenWidth;
extern s16 cameraScreenHeight;
extern u16 sRenderDepthBase;


XglPacket *xglPacketGetCurrent(void);
void sceVif1PkCnt(XglPacket *packet, int count);
void func_0012B6E0(XglPacket *packet, int address, int mode, int a, int b);
void func_0012B770(XglPacket *packet);
void func_0012B9F8(XglPacket *packet, Quadword data);
void func_0012BCE8(XglPacket *packet, void *data, int count);
void pxyUploadViewMatrices(void);
void pxyUploadRenderFlags(s32 mode);
void pxyUploadPartStateDefault(void);

/* GS TEST_1 with only the depth test enabled (ZTE = 1) at depth-test method `ztst`. */
#define TEST_ZONLY(ztst) GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ztst)
/* Same, plus the destination alpha test (DATE, DATM = 1). */
#define TEST_ZDEST(date, ztst) GS_SET_TEST(0, 0, 0, 0, date, 1, 1, ztst)

void pxyUploadPacketState(s32 mode, PxyPacket *model)
{
    XglPacket *vif;

    vif = xglPacketGetCurrent();
    pxyUploadMode = mode;
    pxyPartStyle = 0;
    if (model->renderStatus & 0x20000000) {
        pxyPartStyle = 4;
    } else if (model->renderStatus & 0x10000000) {
        pxyPartStyle = 3;
    } else if (model->renderStatus & 0x1000000) {
        pxyPartStyle = 5;
    } else if (mode & 8) {
        pxyPartStyle = 2;
    } else if (mode & 0x1000) {
        pxyPartStyle = 6;
    }
    sceVif1PkCnt(vif, 0);
    if (pxyUploadedPacket != model) {
        if (model->renderStatus & 1) {
            func_0012B6E0(vif, 0x39C, 0x6C, 1, 1);
            func_0012B9F8(vif, *(Quadword *) model->unk1A0);
            func_0012B770(vif);
        }
        if (model->header->flags & 4) {
            func_0012B6E0(vif, 0x370, 0x6C, 1, 1);
            func_0012BCE8(vif, model->unk1E0, 8);
            func_0012B770(vif);
        }
        if (model->header->flags & 0x10) {
            func_0012B6E0(vif, 0x37C, 0x6C, 1, 1);
            func_0012BCE8(vif, model->unk260, 4);
            func_0012B770(vif);
        }
        if (model->renderStatus & 0x80) {
            func_0012B6E0(vif, 0x3DB, 0x6C, 1, 1);
            func_0012B9F8(vif, *(Quadword *) model->unk70);
            func_0012B770(vif);
        }
        {
        AlignedVector4 viewConstants = { 1.0f, 1.0f, 1.0f, 0.0f };
        PxyQword gsWords[6];

        func_0012B6E0(vif, 0x39D, 0x6C, 1, 1);
        func_0012B9F8(vif, *(Quadword *) &viewConstants);
        func_0012B770(vif);
        viewConstants.x = 4096.0f / (cameraScreenWidth + 4.0f);
        viewConstants.y = (4096.0f / (cameraScreenHeight + 4.0f))
                    / (12.272727f / (54.0f / (DispEnv.display1.magh + 1)));
        viewConstants.z = 1.001f;
        func_0012B6E0(vif, 0x3E7, 0x6C, 1, 1);
        func_0012B9F8(vif, *(Quadword *) &viewConstants);
        func_0012B770(vif);
        if (model->planes != 0
            && ((model->renderStatus & 0x2000 && model->renderStatus & 4) || model->renderStatus & 0x4000
                || model->renderStatus & 0x8000)) {
            Vector4 *row;
            Vector4 *plane;

            row = &gsWords[0].vec;
            plane = model->planes->plane;
            row[0].w = model->planes->plane[0].x;
            row[1].w = model->planes->plane[0].y;
            row[2].w = model->planes->plane[0].z;
            row[3].w = model->planes->plane[0].w;
            row[4].w = model->planes->depth;
            row[5].w = model->planes->range;
            row[5].z = 1.0f / model->planes->range;
            if (model->planeCount == 2) {
                plane = &model->planes->plane[1];
            }
            row[0].z = plane->x;
            row[1].z = plane->y;
            row[2].z = plane->z;
            row[3].z = plane->w;
            func_0012B6E0(vif, 0x352, 0x6C, 1, 1);
            func_0012BCE8(vif, gsWords, 6);
            func_0012B770(vif);
        }
        SPR_CURRENT_PXY = model;
        SPR_PXY_FLAG = 0;
        pxyUploadViewMatrices();
        pxyUploadRenderFlags(mode);
        pxyUploadPartStateDefault();
        {
            s32 date;
            s32 ztst;

            date = 1;
            ztst = 2;
            if (mode & 8) {
                date = 0;
            }
            if (model->drawFlags & 0x200) {
                ztst = 1;
            }
            if (model->renderStatus & 0x10000000) {
                gsWords[0].reg[0] = TEST_ZONLY(1);
                gsWords[1].reg[0] = TEST_ZONLY(1);
                gsWords[2].reg[0] = TEST_ZONLY(1);
                gsWords[3].reg[0] = TEST_ZONLY(1);
                gsWords[4].reg[0] = GS_SET_ZBUF(sRenderDepthBase, 1, 1);
                gsWords[5].reg[0] = GS_SET_ZBUF(sRenderDepthBase, 1, 1);
            } else if (model->renderStatus & 0x1000000) {
                gsWords[0].reg[0] = TEST_ZONLY(ztst);
                gsWords[1].reg[0] = TEST_ZONLY(ztst);
                gsWords[2].reg[0] = TEST_ZONLY(ztst);
                gsWords[3].reg[0] = TEST_ZONLY(ztst);
                gsWords[4].reg[0] = GS_SET_ZBUF(sRenderDepthBase, 1, 1);
                gsWords[5].reg[0] = GS_SET_ZBUF(sRenderDepthBase, 1, 1);
            } else {
                if (mode & 8) {
                    gsWords[1].reg[0] = GS_SET_TEST(1, 5, pxyAlphaRef, 0, 0, 0, 1, ztst);
                    gsWords[3].reg[0] = GS_SET_TEST(1, 5, pxyAlphaRef, 0, date, 1, 1, 1);
                    gsWords[5].reg[0] = GS_SET_ZBUF(sRenderDepthBase, 1, 0);
                } else if (mode & 4) {
                    gsWords[1].reg[0] = TEST_ZONLY(ztst);
                    gsWords[3].reg[0] = TEST_ZDEST(date, 1);
                    gsWords[5].reg[0] = GS_SET_ZBUF(sRenderDepthBase, 1, 1);
                } else if (pxyCurrentDrawPass == 1) {
                    gsWords[1].reg[0] = GS_SET_TEST(1, 2, pxyAlphaRef, 0, 0, 0, 1, ztst);
                    gsWords[3].reg[0] = GS_SET_TEST(1, 2, pxyAlphaRef, 0, date, 1, 1, 1);
                    gsWords[5].reg[0] = GS_SET_ZBUF(sRenderDepthBase, 1, 1);
                } else {
                    gsWords[1].reg[0] = GS_SET_TEST(1, 5, pxyAlphaRef, 0, 0, 0, 1, ztst);
                    gsWords[3].reg[0] = GS_SET_TEST(1, 5, pxyAlphaRef, 0, date, 1, 1, 1);
                    gsWords[5].reg[0] = GS_SET_ZBUF(sRenderDepthBase, 1, 0);
                }
                gsWords[0].reg[0] = TEST_ZONLY(ztst);
                gsWords[2].reg[0] = TEST_ZDEST(date, ztst);
                if (pxyCurrentDrawPass == 1 && !(pxyUploadMode & 8)) {
                    gsWords[4].reg[0] = GS_SET_ZBUF(sRenderDepthBase, 1, 1);
                } else {
                    gsWords[4].reg[0] = GS_SET_ZBUF(sRenderDepthBase, 1, 0);
                }
            }
            gsWords[0].reg[1] = GS_TEST_1;
            gsWords[1].reg[1] = GS_TEST_1;
            gsWords[2].reg[1] = GS_TEST_1;
            gsWords[3].reg[1] = GS_TEST_1;
            gsWords[4].reg[1] = GS_ZBUF_1;
            gsWords[5].reg[1] = GS_ZBUF_1;
            func_0012B6E0(vif, 0x3DC, 0x6C, 1, 1);
            func_0012BCE8(vif, gsWords, 6);
            func_0012B770(vif);
        }
        }
        pxyUploadedPacket = model;
    }
    pxyAlphaRef = 0x75;
}

/* One word of a 16-byte record, viewed as float, signed or unsigned. */
typedef union XglScratchWord {
    float f;
    s32 i;
    u32 u;
} XglScratchWord;

/* A 16-byte record: the four words, or the whole quadword for a by-value call. */
typedef union XglScratchRecord {
    Quadword quad;
    XglScratchWord words[4];
} XglScratchRecord;

#define SCRATCH_WORD (*(u32 *) 0x70000028)

/* One 4x4 float matrix as a struct, so a whole matrix copies with one assignment. */
typedef struct XglMatrix {
    Matrix4 m;
} XglMatrix;

void _CurApplyMatrix(void *out, void *m);
void func_001B7DEC(void *dst, void *src);
void xglMatrixInverse(void *dst, void *src);
void xglMatrixMul(void *dst, void *a, void *b);
void xglMatrixCopy_001D44C4(Matrix4 *destination, const Matrix4 *source);
void xglVectorSubXYZ_001D4530(AlignedVector4 *dst, AlignedVector4 *minuend, AlignedVector4 *subtrahend);
void xglVectorNormal_001A9200(void *dst, void *src);

extern s32 D_00989A30;
extern float D_00989A0C;
extern float D_00989A1C;
extern s32 D_00989A2C;
extern s32 D_00989A3C;

/* Uploads the view, light and part matrices of the current part to VU1 (addresses 0x384..0x3D4). */
void pxyUploadViewMatrices(void)
{
    XglPacket *vif;
    XglMatrix lightMatrix;
    XglMatrix viewMatrix;
    XglScratchRecord transformed;
    XglMatrix worldMatrix;
    XglMatrix modelMatrix;
    XglMatrix shadowMatrix;
    void *cameraMatrix;
    u64 unused360;
    float scale;
    float invScale;
    float extra;

    vif = xglPacketGetCurrent();
    sceVif1PkCnt(vif, 0);
    if (xglStudioSelectedCamera != 0) {
        *(XglMatrixWords *) &worldMatrix = SPR_CURRENT_PXY->worldMatrix;
        if (SPR_PXY_CHANNEL != 0 && (SPR_PXY_CHANNEL->flags & 8) != 0) {
            xglMatrixInverse(&worldMatrix.m, &SPR_CURRENT_PXY->boneMatrices[SPR_PXY_CHANNEL->matrixIndex]);
            xglMatrixMul(&worldMatrix.m, &SPR_CURRENT_PXY->worldMatrix, &worldMatrix.m);
        }
        _CurMatrixSet(&worldMatrix.m);
        func_001B7DEC(&viewMatrix.m, SPR_CURRENT_PXY->matrixBank);
        _CurApplyMatrix(&transformed, &xglStudioSelectedCamera->travel.position);
        viewMatrix.m[0][3] = transformed.words[0].f;
        viewMatrix.m[1][3] = transformed.words[1].f;
        viewMatrix.m[2][3] = transformed.words[2].f;
        if ((SPR_CURRENT_PXY->renderStatus & 0x40) != 0) {
            _CurApplyMatrix(lightMatrix.m[3], &SPR_CURRENT_PXY->matrixBank->lightAxis[0]);
            lightMatrix.m[0][0] = lightMatrix.m[3][0];
            lightMatrix.m[1][0] = lightMatrix.m[3][1];
            lightMatrix.m[2][0] = lightMatrix.m[3][2];
            _CurApplyMatrix(lightMatrix.m[3], &SPR_CURRENT_PXY->matrixBank->lightAxis[1]);
            lightMatrix.m[0][1] = lightMatrix.m[3][0];
            lightMatrix.m[1][1] = lightMatrix.m[3][1];
            lightMatrix.m[2][1] = lightMatrix.m[3][2];
            _CurApplyMatrix(lightMatrix.m[3], &SPR_CURRENT_PXY->matrixBank->lightAxis[2]);
            lightMatrix.m[0][2] = lightMatrix.m[3][0];
            lightMatrix.m[1][2] = lightMatrix.m[3][1];
            lightMatrix.m[2][2] = lightMatrix.m[3][2];
            func_0012B6E0(vif, 980, 108, 1, 1);
            func_0012BCE8(vif, &lightMatrix, 4);
            func_0012B770(vif);
            func_0012B6E0(vif, 984, 108, 1, 1);
            func_0012BCE8(vif, SPR_CURRENT_PXY->matrixBank->lightColor, 3);
            func_0012B770(vif);
        }
        func_0012B6E0(vif, 900, 108, 1, 1);
        func_0012BCE8(vif, &viewMatrix, 3);
        func_0012B770(vif);
        func_0012B6E0(vif, 830, 108, 1, 1);
        func_0012B9F8(vif, transformed.quad);
        func_0012B770(vif);
        xglVectorSubXYZ_001D4530(&transformed, &xglStudioSelectedCamera->travel.position, &SPR_CURRENT_PXY->shadowPosition);
        xglVectorNormal_001A9200(&transformed, &transformed);
        _CurMatrixSet(&worldMatrix.m);
        func_001B7DEC(&viewMatrix.m, SPR_CURRENT_PXY->matrixBank->viewRows);
        _CurApplyMatrix(&transformed, &transformed);
        func_0012B6E0(vif, 903, 108, 1, 1);
        func_0012BCE8(vif, &viewMatrix, 3);
        func_0012B770(vif);
        SPR_CURRENT_PXY->matrixBank->rows[0].w = transformed.words[0].f;
        SPR_CURRENT_PXY->matrixBank->rows[1].w = transformed.words[1].f;
        SPR_CURRENT_PXY->matrixBank->rows[2].w = transformed.words[2].f;
        func_0012B6E0(vif, 906, 108, 1, 1);
        func_0012BCE8(vif, SPR_CURRENT_PXY->matrixBank->rows, 8);
        func_0012B770(vif);
        cameraMatrix = xglStudioSelectedCamera->volume[0].viewMatrix;
        if ((SPR_CURRENT_PXY->renderStatus & 0x10000000) != 0) {
            cameraMatrix = SPR_CURRENT_PXY->shadowCamera[3];
        }
        xglMatrixCopy_001D44C4(&modelMatrix, SPR_CURRENT_PXY->localMatrix);
        if (SPR_PXY_CHANNEL != 0) {
            xglMatrixMul(&modelMatrix.m, &modelMatrix.m, &SPR_CURRENT_PXY->boneMatrices[SPR_PXY_CHANNEL->matrixIndex]);
        }
        func_0012B6E0(vif, 935, 108, 1, 1);
        func_0012BCE8(vif, &modelMatrix, 4);
        func_0012B770(vif);
        if ((SPR_CURRENT_PXY->renderStatus & 0x1000000) != 0) {
            xglMatrixMul(&shadowMatrix.m, cameraMatrix, SPR_CURRENT_PXY->shadowMatrix);
            xglMatrixMul(&modelMatrix.m, &shadowMatrix.m, &modelMatrix.m);
        } else {
            xglMatrixMul(&modelMatrix.m, cameraMatrix, &modelMatrix.m);
        }
        func_0012B6E0(vif, 918, 108, 1, 1);
        func_0012BCE8(vif, &modelMatrix, 4);
        func_0012B770(vif);
        if ((SPR_CURRENT_PXY->renderStatus & 0x8000) != 0 && SPR_CURRENT_PXY->reflectMask != 0
            && SPR_PXY_CHANNEL != 0) {
            register u8 *partBitmask;

            xglVectorSubXYZ_001D4530(&D_00989A30, &SPR_CURRENT_PXY->shadowPosition,
                                     (AlignedVector4 *) SPR_CURRENT_PXY->localMatrix[3]);
            D_00989A0C = SPR_CURRENT_PXY->reflectBlend;
            D_00989A1C = 1.0f;
            if (SPR_CURRENT_PXY->reflectMode == 2) {
                D_00989A1C = -1.0f;
            }
            D_00989A2C = SPR_CURRENT_PXY->reflectMode;
            D_00989A3C = 0;
            partBitmask = SPR_CURRENT_PXY->reflectMask;
            /* One bit per matrix index, packed into 32-bit words. */
            if ((*(u32 *) (partBitmask + ((s16) (SPR_PXY_CHANNEL->matrixIndex >> 5) << 2))
                 & (1 << (SPR_PXY_CHANNEL->matrixIndex & 0x1F))) != 0) {
                D_00989A3C = 1;
            }
            func_0012B6E0(vif, 969, 108, 1, 1);
            func_0012BCE8(vif, &pxyReflectionMatrix, 4);
            func_0012B770(vif);
            scale = SPR_CURRENT_PXY->reflectScale;
            if (scale < 1.0f) {
                scale = 1.0f;
            }
            invScale = 1.0f / scale;
            extra = SPR_CURRENT_PXY->reflectExtra;
            func_0012B6E0(vif, 973, 108, 1, 1);
            func_0012BCE8(vif, &scale, 1);
            func_0012B770(vif);
        }
    }
}


void pxyUploadPartState(void)
{
    XglPacket *packet;
    XglScratchRecord params[2];
    XglScratchRecord *style;

    packet = xglPacketGetCurrent();
    style = &params[0];
    style->words[2].f = 4.0f;
    style->words[3].i = pxyPartStyle;
    style->words[0].f = -1.0f;
    style->words[1].i = 0;
    if (style->words[3].i == 0) {
        if (SPR_CURRENT_PXY->alpha == 0.0f || (SPR_CURRENT_PXY->drawFlags & 0x800) != 0) {
            style->words[3].i = 1;
        }
    }
    if (SPR_PXY_CHANNEL->styleLevel < 4) {
        style->words[1].i = SPR_PXY_CHANNEL->styleLevel;
    }
    if (SPR_PXY_CHANNEL->styleLevel >= 2) {
        style->words[0].f = 1.0f;
    }
    sceVif1PkCnt(packet, 0);
    func_0012B6E0(packet, 0x3C6, 0x6C, 1, 1);
    func_0012B9F8(packet, style->quad);
    func_0012B770(packet);
    style->words[0].u = SCRATCH_WORD;
    style->words[1].i = 0;
    style->words[2].i = 0;
    style->words[3].i = 0;
    func_0012B6E0(packet, 0x3FC, 0x6C, 1, 1);
    func_0012B9F8(packet, style->quad);
    func_0012B770(packet);
}

typedef union PxyStateQuad {
    Quadword quad;
    u32 word[4];
    struct {
        f32 x;
        s32 y;
        f32 z;
        s32 w;
    } field;
} PxyStateQuad;

void pxyUploadPartStateDefault(void)
{
    XglPacket *packet;
    PxyStateQuad buffer;
    PxyStateQuad reserved;
    PxyStateQuad *state;

    packet = xglPacketGetCurrent();
    state = &buffer;
    state->field.z = 4.0f;
    state->field.w = pxyPartStyle;
    state->field.x = -1.0f;
    state->field.y = 0;
    sceVif1PkCnt(packet, 0);
    func_0012B6E0(packet, 966, 108, 1, 1);
    func_0012B9F8(packet, *(Quadword *) state);
    func_0012B770(packet);
    state->word[0] = 0;
    state->word[1] = 0;
    state->word[2] = 0;
    state->word[3] = 0;
    func_0012B6E0(packet, 1020, 108, 1, 1);
    func_0012B9F8(packet, *(Quadword *) state);
    func_0012B770(packet);
}

typedef struct XglDrawParams {
    u32 flagBits;
    s32 cameraLevel;
    float alpha;
    float scale;
} XglDrawParams;

/* Uploads the VU1 flag word (params.flagBits) and the alpha/scale of the current model. */
void pxyUploadRenderFlags(s32 mode)
{
    XglPacket *packet;
    s32 cameraLevel;
    XglDrawParams params;
    AlignedVector4 uniformCopy;
    AlignedVector4 uniform;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    cameraLevel = SPR_CURRENT_PXY->cameraLevel[xglStudioSelectedIndex];
    params.scale = 1.0f;
    if ((mode & 8) == 0) {
        params.scale = params.scale * SPR_CURRENT_PXY->glowScale;
    }
    if ((mode & 0x1000000) != 0) {
        params.scale = 1.0f - SPR_CURRENT_PXY->glowIntensity * SPR_CURRENT_PXY->glowScale;
    }
    params.alpha = SPR_CURRENT_PXY->glowScale * SPR_CURRENT_PXY->alpha;
    if (SPR_CURRENT_PXY->levelTable != 0 && SPR_PXY_CHANNEL != 0) {
        register s32 *levelRow;

        levelRow = SPR_CURRENT_PXY->levelTable[SPR_PXY_CHANNEL->matrixIndex];
        cameraLevel = *(s32 *) ((u8 *) levelRow + (xglStudioSelectedIndex << 2));
    }
    if ((SPR_CURRENT_PXY->renderStatus & 1) != 0 && (SPR_CURRENT_PXY->header->flags & 1) == 0) {
        params.alpha = params.alpha * ((float) cameraLevel / 4080.0f);
    }
    params.cameraLevel = cameraLevel;
    params.flagBits = 0;
    if ((mode & 8) != 0) {
        params.flagBits |= 4;
    }
    if ((mode & 4) != 0) {
        params.flagBits |= 8;
    }
    if (pxyCurrentDrawPass == 1) {
        params.flagBits |= 0x10;
    }
    if ((SPR_CURRENT_PXY->renderStatus & 1) != 0) {
        params.flagBits |= 2;
    }
    if ((SPR_CURRENT_PXY->renderStatus & 0x20000000) != 0) {
        params.flagBits |= 0x20;
    }
    if ((SPR_CURRENT_PXY->renderStatus & 0x40) != 0) {
        params.flagBits |= 0x40;
    }
    if ((SPR_CURRENT_PXY->renderStatus & 0x80) != 0) {
        params.flagBits |= 0x80;
    }
    if ((SPR_CURRENT_PXY->drawFlags & 1) != 0) {
        params.flagBits |= 0x100;
    }
    if ((SPR_CURRENT_PXY->renderStatus & 0x210000) != 0) {
        params.flagBits |= 0x400;
    }
    if (D_0069E10C != 0) {
        params.flagBits |= 0x800;
    }
    if (((SPR_CURRENT_PXY->renderStatus & 0x2000) != 0 && (SPR_CURRENT_PXY->renderStatus & 4) != 0)
        || (SPR_CURRENT_PXY->renderStatus & 0x4000) != 0) {
        params.flagBits |= 0x200;
    }
    if ((SPR_CURRENT_PXY->renderStatus & 0x8000) != 0) {
        params.flagBits |= 0x1000;
    }
    func_0012B6E0(packet, 965, 108, 1, 1);
    func_0012B9F8(packet, *(Quadword *) &params);
    func_0012B770(packet);
    uniform.x = SPR_CURRENT_PXY->uniformScale;
    uniform.y = SPR_CURRENT_PXY->uniformScale;
    uniform.z = SPR_CURRENT_PXY->uniformScale;
    uniform.w = 0.0f;
    uniformCopy = uniform;
    func_0012B6E0(packet, 974, 108, 1, 1);
    func_0012B9F8(packet, *(Quadword *) &uniformCopy);
    func_0012B770(packet);
}

/* Uploads `count` quadwords from `data` to VU1 data memory at `address`. */
void xglPacketUploadVu1Data(void *data, int count, int address)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    func_0012B6E0(packet, (unsigned short)address, 108, 1, 1);
    func_0012BCE8(packet, data, count);
    func_0012B770(packet);
}

void xglDirectQueueBegin(void);
void xglPacketSelectDefaultMode(void);
void xglDirectQueueFlush(void);
float func_002A1D08(float x);
void *memset(void *s, int c, unsigned int n);
extern u8 D_0068F0D0[];
extern u8 D_0068F1D0[];
extern u8 D_0068F2D0[];
extern u8 D_0068F2F0[];
extern u8 D_0068F310[];
extern u8 D_0068F330[];

#define GS_ZBUF_2 0x4F

/* One 16-byte A+D entry of the register list: two data words, then the register and its high word. */
typedef struct GsRegEntry {
    u32 data[2];
    u32 reg;
    u32 regHigh;
} GsRegEntry;

/* One 16-byte record of the cursor packet: a count, an address, a float and a spare word. */
typedef struct CursorRecord {
    u32 count;
    u32 address;
    float value;
    u32 spare;
} CursorRecord;

/* One quadword of a GIF packet, as four words (the tag) or as two register-data pairs. */
typedef union GifQword {
    u32 word[4];
    u64 reg[2];
    float value[4];
} GifQword;

/* Initializes the VU1 constants and GS state for the selected camera, then sets the default packet mode. */
void xglPacketInitVuState(void)
{
    XglPacket *vif;

    vif = xglPacketGetCurrent();
    sceVif1PkCnt(vif, 0);
    func_0012B6E0(vif, 832, 108, 1, 1);
    func_0012B9F8(vif, *(Quadword *) &xglStudioSelectedCamera->screen.screenOffset);
    func_0012B770(vif);
    sceVif1PkCnt(vif, 0);
    func_0012B6E0(vif, 831, 108, 1, 1);
    func_0012B9F8(vif, *(Quadword *) &xglStudioSelectedCamera->screen.screenScale);
    func_0012B770(vif);
    {
        GifQword gif[2];
        GifQword *tag;

        tag = gif;
        tag->word[0] = 0x8001;
        tag->word[1] = 0x10000000;
        tag->word[2] = 14;
        tag->word[3] = 0;
        tag[1].reg[0] = 0;
        tag[1].reg[1] = 63;
        func_0012B6E0(vif, 813, 108, 1, 1);
        func_0012BCE8(vif, tag, 2);
        func_0012B770(vif);
        func_0012B6E0(vif, 826, 108, 1, 1);
        func_0012BCE8(vif, tag, 2);
        func_0012B770(vif);
    }
    {
        GsRegEntry regs[6];
        GsRegEntry *regList;
        s32 regCount;

        regList = regs;
        regCount = 0;
        memset(regList, 0, 96);
        regList[regCount++].reg = 6;
        regList[regCount++].reg = 8;
        regList[regCount].regHigh = 7;
        regList[regCount++].reg = 66;
        regList[regCount++].reg = 67;
        regList[regCount++].reg = 20;
        regList[regCount++].reg = 21;
        func_0012B6E0(vif, 834, 108, 1, 1);
        func_0012BCE8(vif, regList, regCount);
        func_0012B770(vif);
    }
    {
        PxyQword regPair;

        regPair.reg[0] = ((u64) 128 << 32) | 0x68;
        regPair.reg[1] = 67;
        func_0012B6E0(vif, 959, 108, 1, 1);
        func_0012B9F8(vif, *(Quadword *) &regPair);
        func_0012B770(vif);
        regPair.reg[0] = ((u64) 128 << 32) | 0x62;
        func_0012B6E0(vif, 960, 108, 1, 1);
        func_0012B9F8(vif, *(Quadword *) &regPair);
        func_0012B770(vif);
    }
    {
        u8 *vuTable;
        u8 *smallTable;

        vuTable = D_0068F0D0;
        smallTable = D_0068F2D0;
        if (pxyReflectionType != 0) {
            vuTable = D_0068F1D0;
            smallTable = D_0068F2F0;
        }
        func_0012B6E0(vif, 860, 108, 1, 1);
        func_0012BCE8(vif, vuTable, 16);
        func_0012B770(vif);
        func_0012B6E0(vif, 954, 108, 1, 1);
        func_0012BCE8(vif, smallTable, 2);
        func_0012B770(vif);
        func_0012B6E0(vif, 956, 108, 1, 1);
        func_0012BCE8(vif, D_0068F310, 2);
        func_0012B770(vif);
        func_0012B6E0(vif, 828, 108, 1, 1);
        func_0012BCE8(vif, D_0068F330, 1);
        func_0012B770(vif);
    }
    {
        u32 buf[16];

        buf[3] = 0;
        buf[7] = 0x6000;
        buf[11] = 0;
        buf[10] = 4095;
        buf[15] = 0x7000;
        buf[14] = 0x7FFF;
        func_0012B6E0(vif, 808, 108, 1, 1);
        func_0012BCE8(vif, buf, 4);
        func_0012B770(vif);
    }
    {
        Vector4 scaleVec;

        if (xglStudioSelectedCamera->travel.fov < 0.69813174f) {
            scaleVec.w = func_002A1D08(0.34906587f) /
                func_002A1D08(xglStudioSelectedCamera->travel.fov * 0.5f);
        } else {
            scaleVec.w = 1.0f;
        }
        scaleVec.x = scaleVec.w;
        scaleVec.y = scaleVec.w;
        scaleVec.z = scaleVec.w;
        xglPacketUploadVu1Data(&scaleVec, 1, 825);
        xglPacketUploadVu1Data(&D_009899F0, 1, 833);
    }
    {
        CursorRecord records[5];
        CursorRecord *record;

        record = records;
        record->count = 2;
        record->address = 2080;
        (record++)->value = -1.0f;
        record->count = 1;
        record->address = 520;
        (record++)->value = -1.0f;
        record->count = 1;
        record->address = 260;
        (record++)->value = 1.0f;
        record->count = 0;
        record->address = 130;
        (record++)->value = -1.0f;
        record->count = 0;
        record->address = 65;
        (record++)->value = 1.0f;
        xglPacketUploadVu1Data(records, 5, 994);
    }
    xglPacketSelectDefaultMode();
    xglDirectQueueBegin();
    {
        u64 *p;
        u64 *start;
        u32 unk[12];

        p = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
        start = p;
        GS_PUT(p, GS_ZBUF_2, GS_SET_ZBUF(sRenderDepthBase, 1, 1));
        xglDirectQueueCount += ((u32) p - (u32) start) >> 4;
    }
    xglDirectQueueFlush();
}

extern void sceVif1PkRef(XglPacket *packet, const void *environment, int count, int mode, int offset, int flags);
u8 *xglPacketMicrocodeEntry(int mode);
void func_0012B9A0(XglPacket *packet, u32 value);

void xglPacketSelectDefaultMode(void)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    xglPacketMode = 0;
    sceVif1PkRef(packet, xglPacketMicrocodeEntry(0), xglPacketMicrocodeEntry(1), 0, 0, 0);
    sceVif1PkCnt(packet, 0);
    func_0012B9A0(packet, (u32) xglPacketMicrocodeEntry(3) | 0x14000000);
    xglDirectQueueCount = 0;
}

void xglPacketSelectMode(PxyPacket *model)
{
    XglPacket *packet;
    s32 mode;

    packet = xglPacketGetCurrent();
    mode = 0;
    if (model->renderStatus & 0x8000) {
        mode = 2;
    } else if (model->header->flags & 1) {
        mode = 1;
    } else if (xglPacketMode4Flag) {
        mode = 4;
    } else if (xglPacketMode3Flag) {
        mode = 3;
    }
    if (xglPacketMode != mode) {
        xglPacketMode = mode;
        sceVif1PkRef(packet, xglPacketMicrocodeEntry(0), xglPacketMicrocodeEntry(1), 0, 0, 0);
        sceVif1PkCnt(packet, 0);
    }
}

/* Uploads the part's bone matrices (4 quadwords each) to VU1 address 0x228; skipped when this packet's matrices are already there. */
void pxyUploadBoneMatrices(void)
{
    XglPacket *packet;
    s32 i;
    u8 *base;
    u8 *matrixIds;

    packet = xglPacketGetCurrent();
    if (SPR_CURRENT_PXY->header->matrixCount == 0) {
        return;
    }
    if (SPR_CURRENT_PXY->header->matrixCount < 0x41) {
        if (pxyBoneCachePacket != SPR_CURRENT_PXY && SPR_CURRENT_PXY->matrixTable != 0) {
            i = SPR_CURRENT_PXY->header->matrixCount;
            sceVif1PkCnt(packet, 0);
            if (i == 0x40) {
                func_0012B6E0(packet, 0x228, 0x6C, 1, 1);
                func_0012BCE8(packet, SPR_CURRENT_PXY->matrixTable, 0xFC);
                func_0012B770(packet);
                func_0012B6E0(packet, 0x324, 0x6C, 1, 1);
                func_0012BCE8(packet, SPR_CURRENT_PXY->matrixTable + 0xFC0, 4);
                func_0012B770(packet);
            } else {
                func_0012B6E0(packet, 0x228, 0x6C, 1, 1);
                func_0012BCE8(packet, SPR_CURRENT_PXY->matrixTable, i * 4);
                func_0012B770(packet);
            }
        }
        pxyBoneCachePacket = SPR_CURRENT_PXY;
    } else {
        i = 0;
        base = SPR_CURRENT_PXY->channelTable + ((SPR_PXY_CHANNEL->matrixIndex * 7) << 4);
        matrixIds = base + 0x30;
        sceVif1PkCnt(packet, 0);
        while (*matrixIds != 0) {
            func_0012B6E0(packet, (u16) (i * 4 + 0x228), 0x6C, 1, 1);
            func_0012BCE8(packet, SPR_CURRENT_PXY->matrixTable + (*matrixIds << 6) - 0x40, 4);
            func_0012B770(packet);
            matrixIds++;
            i++;
        }
    }
}

void func_0012A8C0(void *dst, s16 x, s16 w, int a, int b, int c, s16 d, s16 e);
void func_0012B7C0(XglPacket *packet, int mode);
void func_0012B818(XglPacket *packet);
void sceVif1PkAddDirectDataN(XglPacket *packet, const void *data, int qwc);
int xglPsmBufferWidth(int value, int mode);

/* One 32-byte record of a packet set; its data sits dataOffset bytes past the table. */
typedef struct XglPacketRecord {
    s32 dataOffset;     /* +0x00 */
    u16 originX;        /* +0x04 */
    u8 unk06[2];
    s16 extentA;        /* +0x08 */
    u8 unk0A[2];
    s16 extentB;        /* +0x0C */
    u8 unk0E[6];
    s32 count;          /* +0x14 */
    u8 unk18[8];
} XglPacketRecord;

/* A packet set: its record count at +0x14, records from +0x20. */
typedef struct XglPacketSet {
    u8 unk00[0x14];
    s32 count;                  /* +0x14 */
    u8 unk18[0x20 - 0x18];
    XglPacketRecord records[8]; /* +0x20 */
} XglPacketSet;

/* The table: one offset per index, from +0x10, to a packet set. */
typedef struct XglPacketTable {
    u8 unk00[4];
    s32 setCount;               /* +0x04 */
    u8 unk08[8];
    s32 setOffset[8];           /* +0x10 */
} XglPacketTable;

typedef union XglPacketBlock5 {
    u64 words[12];
    Quadword quads[6];
} XglPacketBlock5;

/* One direct-mode message: a GIF tag quadword, then the block of six quadwords. */
typedef struct XglPacketMessage {
    u32 tag[4];
    XglPacketBlock5 block;
} XglPacketMessage;

void pxyAddEntry(int slot, void *data, int qwc);

void pxyQueueTextureSet(int slot, XglPacketTable *table, int index, int originX)
{
    XglPacketSet *set;
    int i;
    XglPacketMessage msg;
    XglPacketRecord *rec;

    if (table->setCount <= index) {
        return;
    }
    set = (XglPacketSet *) (table->setOffset[index] + (int) table);
    if (set != 0) {
        i = set->count - 1;
        while (i >= 0) {
            rec = &set->records[i];
            func_0012A8C0(&msg.block, (s16) ((u16) originX + rec->originX), (s16) xglPsmBufferWidth(512, 0), 0, 0, 0,
                          rec->extentA, rec->extentB);
            msg.block.words[0] &= ~0x8000;
            msg.tag[3] = 0x50000005;
            msg.tag[0] = 0;
            msg.tag[1] = 0;
            msg.tag[2] = 0;
            pxyAddEntry(slot, &msg, 6);
            pxyAddEntry(slot + 1, (u8 *) table + rec->dataOffset, rec->count);
            i--;
        }
    }
}

void pxyUploadTextureSet(XglPacketTable *table, int index, int originX)
{
    XglPacketBlock5 block;
    XglPacketSet *set;
    XglPacket *packet;
    int i;
    XglPacketRecord *rec;

    set = (XglPacketSet *) (table->setOffset[index] + (int) table);
    if (set != 0) {
        packet = xglPacketGetCurrent();
        i = set->count - 1;
        while (i >= 0) {
            rec = &set->records[i];
            sceVif1PkCnt(packet, 0);
            func_0012B9A0(packet, 0x11000000);
            func_0012A8C0(&block, (s16) ((u16) originX + rec->originX), (s16) xglPsmBufferWidth(512, 0), 0, 0, 0,
                          rec->extentA, rec->extentB);
            block.words[0] &= ~0x8000;
            func_0012B7C0(packet, 0);
            sceVif1PkAddDirectDataN(packet, &block, 5);
            func_0012B818(packet);
            sceVif1PkRef(packet, (u8 *) table + rec->dataOffset, rec->count, 0, 0, 0);
            i--;
        }
        sceVif1PkCnt(packet, 0);
    }
}

extern u16 sRenderTextureBase;


/* GS FOGCOL: the fog colour register that the first A+D pair of the packet writes. */
#define GS_FOGCOL 0x3D

/* Uploads the packet's texture table for the current channel if it changed, then sets GS FOGCOL from the packet's fog colour. */
void pxyUploadModelTextures(void)
{
    u64 *p;
    u64 *start;

    if (SPR_CURRENT_PXY->textureTable != 0 &&
        (pxyTextureTableCache != SPR_CURRENT_PXY->textureTable || pxyTextureChannelCache != SPR_PXY_CHANNEL->channelIndex)) {
        pxyUploadTextureSet(SPR_CURRENT_PXY->textureTable, SPR_PXY_CHANNEL->channelIndex, sRenderTextureBase << 5);
        pxySubmitCount++;
        xglDirectQueueBegin();
        p = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
        start = p;
        p[0] = (u64) (u32) SPR_CURRENT_PXY->fogColorR | ((u64) (u32) SPR_CURRENT_PXY->fogColorG << 8)
            | ((u64) (u32) SPR_CURRENT_PXY->fogColorB << 16);
        p[1] = GS_FOGCOL;
        p += 2;
        GS_PUT(p, GS_TEXFLUSH, 0);
        xglDirectQueueCount += ((u32) p - (u32) start) >> 4;
        xglDirectQueueFlush();
        pxyTextureTableCache = SPR_CURRENT_PXY->textureTable;
        pxyTextureChannelCache = SPR_PXY_CHANNEL->channelIndex;
    }
}

void xglPacketAddFlushE(void);

void xglDirectQueueFlush(void)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    if (xglDirectQueueCount >= 3) {
        packet->cursor += xglDirectQueueCount << 4;
        xglDirectQueueBuffer[0].word[0] = 0;
        xglDirectQueueBuffer[0].word[1] = 0;
        xglDirectQueueBuffer[0].word[2] = 0;
        xglDirectQueueBuffer[0].word[3] =
            ((xglDirectQueueCount - 1) << 16) + 0x6C000330;
        xglDirectQueueBuffer[1].word[0] = xglDirectQueueCount + 0x7FFE;
        xglDirectQueueBuffer[1].word[1] = 0x10000000;
        xglDirectQueueBuffer[1].word[2] = 0xE;
        xglDirectQueueBuffer[1].word[3] = 0;
        sceVif1PkRef(packet, xglDirectQueueBuffer, xglDirectQueueCount, 0, 0, 0);
        sceVif1PkCnt(packet, 0);
        func_0012B9A0(packet, (u32) xglPacketMicrocodeEntry(3) | 0x14000000);
        func_0012B9A0(packet, 0x17000000);
        xglPacketAddFlushE();
        sceVif1PkCnt(packet, 0);
    }
    xglDirectQueueCount = 0;
}

/* Same as xglDirectQueueFlush but unpacks at VU1 0x228 and kicks the wide GS program (kind 4); resets the bone cache. */
void xglDirectQueueFlushWide(void)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    if (xglDirectQueueCount >= 3) {
        packet->cursor += xglDirectQueueCount << 4;
        xglDirectQueueBuffer[0].word[0] = 0;
        xglDirectQueueBuffer[0].word[1] = 0;
        xglDirectQueueBuffer[0].word[2] = 0;
        xglDirectQueueBuffer[0].word[3] =
            ((xglDirectQueueCount - 1) << 16) + 0x6C000228;
        xglDirectQueueBuffer[1].word[0] = xglDirectQueueCount + 0x7FFE;
        xglDirectQueueBuffer[1].word[1] = 0x10000000;
        xglDirectQueueBuffer[1].word[2] = 0xE;
        xglDirectQueueBuffer[1].word[3] = 0;
        sceVif1PkRef(packet, xglDirectQueueBuffer, xglDirectQueueCount, 0, 0, 0);
        sceVif1PkCnt(packet, 0);
        func_0012B9A0(packet, (u32) xglPacketMicrocodeEntry(4) | 0x14000000);
        func_0012B9A0(packet, 0x17000000);
        xglPacketAddFlushE();
        sceVif1PkCnt(packet, 0);
    }
    xglDirectQueueCount = 0;
    func_001CD160();
}

extern u16 sRenderDisplayBase;
extern s16 sRenderPsm;

#define GS_FBA_2 0x4B
#define GS_TEST_2 0x48
#define GS_PSMCT24 0x01

void pxyResetGsState(void)
{
    u64 *p;
    u64 *start;

    xglDirectQueueBegin();
    p = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
    start = p;
    GS_PUT(p, GS_ZBUF_1, GS_SET_ZBUF(sRenderDepthBase, GS_PSMCT24, 0));
    GS_PUT(p, GS_ZBUF_2, GS_SET_ZBUF(sRenderDepthBase, GS_PSMCT24, 0));
    GS_PUT(p, GS_FBA_1, 0);
    GS_PUT(p, GS_FBA_2, 0);
    GS_PUT(p, GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2));
    GS_PUT(p, GS_TEST_2, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2));
    GS_PUT(p, GS_FRAME_1, GS_SET_FRAME(sRenderDisplayBase, (s16) (cameraScreenWidth / 64), sRenderPsm, 0));
    GS_PUT(p, GS_FRAME_2, GS_SET_FRAME(sRenderDisplayBase, (s16) (cameraScreenWidth / 64), sRenderPsm, 0));
    GS_PUT(p, GS_ALPHA_1, GS_SET_ALPHA(0, 2, 2, 1, 0x80));
    xglDirectQueueCount += ((u32) p - (u32) start) >> 4;
    xglDirectQueueFlushWide();
}

#define GS_SCISSOR_2 0x41
/* The selected studio camera's screen rectangle (floats at +0x60..+0x6C), read as the GS scissor. */
#define STUDIO_CAMERA_WINDOW(edge) ((s32) xglStudioSelectedCamera->screen.window.edge)

void xglStudioSetScissor(void)
{
    u64 *p;
    u64 *start;

    if (xglStudioSelectedCamera == 0) {
        return;
    }
    xglDirectQueueBegin();
    p = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
    start = p;
    GS_PUT(p, GS_SCISSOR_1,
           GS_SET_SCISSOR(STUDIO_CAMERA_WINDOW(x), STUDIO_CAMERA_WINDOW(z) - 1,
                          STUDIO_CAMERA_WINDOW(y), STUDIO_CAMERA_WINDOW(w) - 1));
    GS_PUT(p, GS_SCISSOR_2,
           GS_SET_SCISSOR(STUDIO_CAMERA_WINDOW(x), STUDIO_CAMERA_WINDOW(z) - 1,
                          STUDIO_CAMERA_WINDOW(y), STUDIO_CAMERA_WINDOW(w) - 1));
    xglDirectQueueCount += ((u32) p - (u32) start) >> 4;
    xglDirectQueueFlush();
}

void xglSetFullScreenScissor(void)
{
    u32 entry;
    u32 first;

    xglDirectQueueBegin();
    entry = (u32) &xglDirectQueueBuffer[xglDirectQueueCount];
    first = entry;
    ((u64 *) entry)[0] = ((u64) (cameraScreenWidth - 1) << 16) | ((u64) (cameraScreenHeight - 1) << 48);
    ((u64 *) entry)[1] = 64;
    entry += 16;
    ((u64 *) entry)[0] = ((u64) (cameraScreenWidth - 1) << 16) | ((u64) (cameraScreenHeight - 1) << 48);
    ((u64 *) entry)[1] = 65;
    entry += 16;
    xglDirectQueueCount += (entry - first) >> 4;
    xglDirectQueueFlush();
}

typedef struct PxyTableEntry {
    u32 offset;       /* +0x00 */
    u32 count;        /* +0x04 */
    u8 unk08[0x18];
} PxyTableEntry;

/* The scratchpad words the model drawing code shares: the packet being drawn and its channel. */
typedef struct PxyScratch {
    u8 unk00[0x10];
    PxyPacket *model;     /* +0x10 */
    u8 unk14[0x8];
    PxyChannel *channel;  /* +0x1C */
} PxyScratch;

void pxyDrawPartData(void)
{
    XglPacket *packet;
    register PxyScratch *scratch;

    packet = xglPacketGetCurrent();
    scratch = (PxyScratch *) 0x70000000;
    sceVif1PkCnt(packet, 0);
    func_0012B9A0(packet, (u32) xglPacketMicrocodeEntry(2) | 0x14000000);
    sceVif1PkRef(packet, (u8 *) scratch->model->header + scratch->model->entries[scratch->channel->entryIndex].offset,
                 scratch->model->entries[scratch->channel->entryIndex].count, 0, 0, 0);
    xglPacketAddFlushE();
}

/* Sets the current packet's count to 0 (sceVif1PkCnt), then adds VIF FLUSHE with the interrupt bit (0x91000000). */
void xglPacketAddFlushEIrq(void)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    func_0012B9A0(packet, 0x91000000);
}

/* Sets the current packet's count to 0 (sceVif1PkCnt), then adds VIF FLUSHE (0x11000000). */
void xglPacketAddFlushE(void)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    func_0012B9A0(packet, 0x11000000);
}

void xglPacketAddRefOffset(const void *data, int count, int offset)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkRef(packet, data, count, 0, offset, 0);
}

void xglPacketAddRef(const void *environment, int count)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkRef(packet, environment, count, 0, 0, 0);
}

typedef struct XglDirectQueueSourceEntry {
    u64 data;
    u32 address;
    u8 unmodeled_0c[4];
} XglDirectQueueSourceEntry;

void xglDirectQueueAppendFlushWide(XglDirectQueueSourceEntry *source, s32 count)
{
    XglDirectQueueSourceEntry *src;
    XglDirectQueueEntry *entry;
    XglDirectQueueEntry *first;
    s32 i;

    src = source;
    xglDirectQueueBegin();
    entry = &xglDirectQueueBuffer[xglDirectQueueCount];
    first = entry;
    for (i = 0; i < count - 1; i++) {
        entry->data = src[i + 1].data;
    }
    ((u64 *) entry)[1] = src[i + 1].address;
    entry += 1;
    xglDirectQueueCount += ((u32) entry - (u32) first) >> 4;
    xglDirectQueueFlushWide();
}

/* GS primitive coordinates are 12.4 fixed point around the 2048 origin; Q = 1.0f as raw bits. */
#define GS_ORIGIN 0x800
#define GS_Q_ONE 0x3F800000
/* The glow passes copy the 0xFF-pixel square that starts one pixel after the origin. */
#define GS_ORIGIN_1 (GS_ORIGIN + 1)
#define GS_LAST (GS_ORIGIN + 0xFF)
/* XYZ2 vertex at (x, y) in pixels. */
#define XY(x, y) GS_SET_XYZ((x) << 4, (y) << 4, 0)
/* Half the screen size. */
#define SCREEN_HALF_WIDTH ((s16) (cameraScreenWidth / 2))
#define SCREEN_HALF_HEIGHT ((s16) (cameraScreenHeight / 2))

void func_001D0AE0(s16 unused0)
{
    u64 *p;
    u64 *start;

    xglDirectQueueBegin();
    p = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
    start = p;
    GS_PUT(p, GS_FBA_1, 1);
    GS_PUT(p, GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1));
    GS_PUT(p, GS_RGBAQ, (u64) 0xFE00 << 46);
    GS_PUT(p, GS_PRIM, 6);
    GS_PUT(p, GS_XYZF2, XY(GS_ORIGIN - SCREEN_HALF_WIDTH, GS_ORIGIN - SCREEN_HALF_HEIGHT));
    GS_PUT(p, GS_XYZF2, XY(GS_ORIGIN + SCREEN_HALF_WIDTH, GS_LAST - SCREEN_HALF_HEIGHT));
    GS_PUT(p, GS_FBA_1, 0);
    xglDirectQueueCount += ((u32) p - (u32) start) >> 4;
    xglDirectQueueFlushWide();
}

extern s32 D_006457E0;
extern s32 D_006457E4;
extern s32 D_006457E8;
extern s32 D_006457EC;

/*
 * Queues the render state of the frame buffer at frameBase: the frame and Z-buffer registers, a sprite in
 * colour that covers the camera rectangle, and the frame register again for the second pass.
 * flags: 1 clears the colour alpha and the upper frame mask bits, 2 keeps only the alpha frame mask bits,
 * 4 clears the colour, 8 enables Z writes.
 */
void pxyClearFrame(u32 frameBase, u32 flags)
{
    s32 colour[4];
    u32 frameMask;
    u32 zMask;
    u64 *p;
    u64 *start;

    frameMask = 0xFFFFFFFF;
    zMask = 1;
    colour[0] = D_006457E0;
    colour[1] = D_006457E4;
    colour[2] = D_006457E8;
    colour[3] = D_006457EC;
    if (flags & 4) {
        memset(colour, 0, 0x10);
    }
    if (flags & 2) {
        frameMask &= 0xFF000000;
    }
    if (flags & 1) {
        frameMask &= 0xFFFFFF;
        colour[3] = 0;
    }
    if (flags & 8) {
        zMask = 0;
    }

    xglDirectQueueBegin();
    p = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
    start = p;
    GS_PUT(p, GS_FBA_1, 0);
    GS_PUT(p, GS_ZBUF_1, GS_SET_ZBUF(sRenderDepthBase, 1, zMask));
    GS_PUT(p, GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1));
    GS_PUT(p, GS_FRAME_1, GS_SET_FRAME(frameBase, (s16) (cameraScreenWidth / 64), sRenderPsm, frameMask));
    GS_PUT(p, GS_RGBAQ, GS_SET_RGBAQ(colour[0], colour[1], colour[2], colour[3], GS_Q_ONE));
    GS_PUT(p, GS_PRIM, 6);
    GS_PUT(p, GS_XYZF2,
           GS_SET_XYZ((GS_ORIGIN - cameraScreenWidth) << 4, (GS_ORIGIN - cameraScreenHeight) << 4, 1));
    GS_PUT(p, GS_XYZF2,
           GS_SET_XYZ((GS_ORIGIN + cameraScreenWidth) << 4, (GS_ORIGIN + cameraScreenHeight) << 4, 1));
    GS_PUT(p, GS_FRAME_1, GS_SET_FRAME(frameBase, (s16) (cameraScreenWidth / 64), sRenderPsm, 0));
    xglDirectQueueCount += ((u32) p - (u32) start) >> 4;
    xglDirectQueueFlushWide();
}

#define GLOW_LEVEL(packet) \
    ((int) ((packet)->glowIntensity * 128.0f * (packet)->glowScale * \
            (float) (packet)->cameraLevel[xglStudioSelectedIndex] / 4080.0f))

void xglDirectQueueSubmitRef(void);

/*
 * Writes the render-state packet for the texture at texBase and the frame at frameBase, then two
 * screen quads: the first from the UV and XYZ2 of posA, the second from the XYZ2 corner of posB.
 */
void pxyDrawTextureRect(u32 texBase, Vector4 *srcRect, u32 frameBase, Vector4 *dstRect)
{
    u64 *p;
    u64 *start;

    xglDirectQueueBegin();
    p = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
    start = p;
    GS_PUT(p, GS_FRAME_1, GS_SET_FRAME(frameBase, (s16) (cameraScreenWidth / 64), sRenderPsm, 0));
    GS_PUT(p, GS_TEXFLUSH, 0);
    GS_PUT(p, GS_ZBUF_1, GS_SET_ZBUF(sRenderDepthBase, 1, 1));
    GS_PUT(p, GS_CLAMP_1, 0);
    GS_PUT(p, GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1));
    GS_PUT(p, GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, GS_Q_ONE));
    GS_PUT(p, GS_TEX0_1, GS_SET_TEX0(texBase << 5, 8, 0, 9, 9, 1, 0, 0, 0, 0, 0, 1));
    GS_PUT(p, GS_TEX1_1, GS_SET_TEX1(0, 0, 1, 1, 0, 0, 0));
    GS_PUT(p, GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 0, 0, 1, 0, 0));
    GS_PUT(p, GS_UV, GS_SET_UV(((s32) srcRect->x + 1) << 4, ((s32) srcRect->y + 1) << 4));
    GS_PUT(p, GS_XYZF2, XY(GS_ORIGIN - SCREEN_HALF_WIDTH, GS_ORIGIN - SCREEN_HALF_HEIGHT));
    GS_PUT(p, GS_UV, GS_SET_UV((s32) srcRect->z << 4, (s32) srcRect->w << 4));
    GS_PUT(p, GS_XYZF2, XY(GS_ORIGIN + ((s32) dstRect->z - SCREEN_HALF_WIDTH), GS_ORIGIN + ((s32) dstRect->w - SCREEN_HALF_HEIGHT)));
    xglDirectQueueCount += ((u32) p - (u32) start) >> 4;
    xglDirectQueueSubmitRef();
}

/* GS_ALPHA_2 register (0x43) of the glow setup packet. */
#define GS_ALPHA_2 0x43

/*
 * Writes the render-state packet for the texture at texBase and the frame at frameBase, then the
 * screen quad (UV and XYZ2 corners at the origin plus or minus half the screen size).
 */
void pxyDrawTextureFullscreen(u32 texBase, u32 frameBase)
{
    u64 *p;
    u64 *start;

    xglDirectQueueBegin();
    p = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
    start = p;
    GS_PUT(p, GS_FRAME_1, GS_SET_FRAME(frameBase, (s16) (cameraScreenWidth / 64), sRenderPsm, 0));
    GS_PUT(p, GS_TEXFLUSH, 0);
    GS_PUT(p, GS_CLAMP_1, 0);
    GS_PUT(p, GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1));
    GS_PUT(p, GS_ALPHA_2, GS_SET_ALPHA(0, 2, 2, 1, 128));
    GS_PUT(p, GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, GS_Q_ONE));
    GS_PUT(p, GS_TEX0_1, GS_SET_TEX0(texBase << 5, 8, 0, 9, 9, 1, 0, 0, 0, 0, 0, 1));
    GS_PUT(p, GS_TEX1_1, 0);
    GS_PUT(p, GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 1, 0, 1, 0, 0));
    GS_PUT(p, GS_UV, 0);
    GS_PUT(p, GS_XYZF2, XY(GS_ORIGIN - SCREEN_HALF_WIDTH, GS_ORIGIN - SCREEN_HALF_HEIGHT));
    GS_PUT(p, GS_UV, GS_SET_UV(cameraScreenWidth << 4, cameraScreenHeight << 4));
    GS_PUT(p, GS_XYZF2, XY(GS_ORIGIN + SCREEN_HALF_WIDTH, GS_ORIGIN + SCREEN_HALF_HEIGHT));
    xglDirectQueueCount += ((u32) p - (u32) start) >> 4;
    xglDirectQueueSubmitRef();
}

/*
 * Draws the glow of `packet` into the frame buffer at frameBase: a sprite in the glow colour, three
 * passes that copy the buffer onto itself shifted by one texel, and a line-strip border.
 */
void pxyDrawGlow(u32 frameBase, PxyPacket *packet)
{
    int i;
    u64 *p;
    u64 *q;

    xglDirectQueueBegin();
    p = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
    q = p;
    GS_PUT(p, GS_TEST_1, GS_SET_TEST(1, 0, 0, 1, 1, 0, 1, 1));
    if (packet->drawMode == 4) {
        GS_PUT(p, GS_RGBAQ,
               GS_SET_RGBAQ(GLOW_LEVEL(packet), GLOW_LEVEL(packet), GLOW_LEVEL(packet), 0x80, GS_Q_ONE));
    } else {
        GS_PUT(p, GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80 - GLOW_LEVEL(packet), GS_Q_ONE));
    }
    GS_PUT(p, GS_PRIM, 6);
    GS_PUT(p, GS_XYZF2, XY(GS_ORIGIN - SCREEN_HALF_WIDTH, GS_ORIGIN - SCREEN_HALF_HEIGHT));
    GS_PUT(p, GS_XYZF2, XY(GS_ORIGIN, GS_LAST - SCREEN_HALF_HEIGHT));
    GS_PUT(p, GS_FRAME_1, GS_SET_FRAME(frameBase, (s16) (cameraScreenWidth / 64), sRenderPsm, 0));
    GS_PUT(p, GS_TEX0_1, GS_SET_TEX0(frameBase << 5, 8, 0, 9, 9, 1, 0, 0, 0, 0, 0, 1));
    GS_PUT(p, GS_TEX1_1, GS_SET_TEX1(0, 0, 1, 1, 0, 0, 0));
    GS_PUT(p, GS_TEST_1, GS_SET_TEST(1, 0, 0, 1, 0, 0, 1, 1));
    GS_PUT(p, GS_CLAMP_1, 0);
    GS_PUT(p, GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, GS_Q_ONE));
    xglDirectQueueCount += ((u32) p - (u32) q) >> 4;
    xglDirectQueueFlushWide();

    for (i = 0; i < 3; i++) {
        xglDirectQueueBegin();
        q = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
        p = q;
        GS_PUT(q, GS_TEXFLUSH, 0);
        GS_PUT(q, GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 0, 0, 1, 0, 0));
        GS_PUT(q, GS_UV, GS_SET_UV(0x10, 0x10));
        GS_PUT(q, GS_XYZF2, XY(GS_ORIGIN_1, GS_ORIGIN_1 - SCREEN_HALF_HEIGHT));
        GS_PUT(q, GS_UV, GS_SET_UV(0xFF0, 0xFF0));
        GS_PUT(q, GS_XYZF2, XY(GS_LAST, GS_LAST - SCREEN_HALF_HEIGHT));
        GS_PUT(q, GS_TEXFLUSH, 0);
        GS_PUT(q, GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 0, 0, 1, 0, 0));
        GS_PUT(q, GS_UV, GS_SET_UV((SCREEN_HALF_WIDTH + 1) << 4, 0x10));
        GS_PUT(q, GS_XYZF2, XY(GS_ORIGIN_1 - SCREEN_HALF_WIDTH, GS_ORIGIN_1 - SCREEN_HALF_HEIGHT));
        GS_PUT(q, GS_UV, GS_SET_UV((SCREEN_HALF_WIDTH + 0xFF) << 4, 0xFF0));
        GS_PUT(q, GS_XYZF2, XY(GS_LAST - SCREEN_HALF_WIDTH, GS_LAST - SCREEN_HALF_HEIGHT));
        xglDirectQueueCount += ((u32) q - (u32) p) >> 4;
        xglDirectQueueFlushWide();
    }

    xglDirectQueueBegin();
    q = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
    p = q;
    GS_PUT(q, GS_TEXFLUSH, 0);
    GS_PUT(q, GS_TEST_1, GS_SET_TEST(1, 0, 0, 1, 0, 0, 1, 1));
    GS_PUT(q, GS_RGBAQ, GS_SET_RGBAQ(0, 0, 0, 0x80, GS_Q_ONE));
    GS_PUT(q, GS_PRIM, 2);
    GS_PUT(q, GS_XYZF2, XY(GS_LAST - SCREEN_HALF_WIDTH, GS_ORIGIN - SCREEN_HALF_HEIGHT));
    GS_PUT(q, GS_XYZF2, XY(GS_ORIGIN - SCREEN_HALF_WIDTH, GS_ORIGIN - SCREEN_HALF_HEIGHT));
    GS_PUT(q, GS_XYZF2, XY(GS_ORIGIN - SCREEN_HALF_WIDTH, GS_LAST - SCREEN_HALF_HEIGHT));
    GS_PUT(q, GS_XYZF2, XY(GS_LAST - SCREEN_HALF_WIDTH, GS_LAST - SCREEN_HALF_HEIGHT));
    GS_PUT(q, GS_XYZF2, XY(GS_LAST - SCREEN_HALF_WIDTH, GS_ORIGIN - SCREEN_HALF_HEIGHT));
    xglDirectQueueCount += ((u32) q - (u32) p) >> 4;
    xglDirectQueueFlushWide();
}

extern u16 sRenderFrameBase;

#define GS_BITBLTBUF 0x50
#define GS_TRXPOS 0x51
#define GS_TRXREG 0x52
#define GS_TRXDIR 0x53
#define GS_PSMZ24 0x31
#define GS_SET_BITBLTBUF(sbp, sbw, spsm, dbp, dbw, dpsm) \
    ((u64)(sbp) | ((u64)(sbw) << 16) | ((u64)(spsm) << 24) | ((u64)(dbp) << 32) | ((u64)(dbw) << 48) \
     | ((u64)(dpsm) << 56))
#define GS_SET_TRXREG(width, height) ((u64)(width) | ((u64)(height) << 32))

void xglCopyZBufferToRenderFrame(s32 unused0, s32 unused1)
{
    u64 *p;
    u64 *start;

    xglDirectQueueBegin();
    p = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
    start = p;
    GS_PUT(p, GS_BITBLTBUF,
           GS_SET_BITBLTBUF(sRenderDepthBase << 5, 8, GS_PSMZ24, sRenderFrameBase << 5, 8, GS_PSMZ24));
    GS_PUT(p, GS_TRXPOS, 0);
    GS_PUT(p, GS_TRXREG, GS_SET_TRXREG(cameraScreenWidth, cameraScreenHeight));
    GS_PUT(p, GS_TRXDIR, 2);
    xglDirectQueueCount += ((u32) p - (u32) start) >> 4;
    xglDirectQueueFlush();
}

void xglCopyRenderFrameToZBuffer(s32 unused0, s32 unused1)
{
    u64 *p;
    u64 *start;

    xglDirectQueueBegin();
    p = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
    start = p;
    GS_PUT(p, GS_BITBLTBUF,
           GS_SET_BITBLTBUF(sRenderFrameBase << 5, 8, GS_PSMZ24, sRenderDepthBase << 5, 8, GS_PSMZ24));
    GS_PUT(p, GS_TRXPOS, 0);
    GS_PUT(p, GS_TRXREG, GS_SET_TRXREG(cameraScreenWidth, cameraScreenHeight));
    GS_PUT(p, GS_TRXDIR, 2);
    xglDirectQueueCount += ((u32) p - (u32) start) >> 4;
    xglDirectQueueFlush();
}


void func_002A9E04(void *, void *, int);

u8 *xglPacketAppendData(void *data, int size)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    func_002A9E04(packet->cursor, data, size);
    packet->cursor += size;
    return packet->cursor - size;
}

void xglMatrixCopy_001D44C4(register Matrix4 *destination, register const Matrix4 *source);

u8 *xglPacketAppendMatrix(int matrix)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    xglMatrixCopy_001D44C4(packet->cursor, matrix);
    packet->cursor += 64;
    return packet->cursor - 64;
}

void xglMemCopy64(void *dst, void *src, int count);

u8 *xglPacketAppendMatrices(void *matrices, int count)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    xglMemCopy64(packet->cursor, matrices, count);
    packet->cursor += count << 6;
    return packet->cursor - (count << 6);
}

void xglMemCopy16(void *dst, void *src, int count);

u8 *xglPacketAppendVectors(void *matrices, int count)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    xglMemCopy16(packet->cursor, matrices, count);
    packet->cursor += count << 4;
    return packet->cursor - (count << 4);
}

u8 *xglPacketReserve(s32 count)
{
    XglPacket *packet;
    u8 *cursor;

    packet = xglPacketGetCurrent();
    cursor = packet->cursor;
    if ((u32) packet->cursor - packet->limit >= packet->buffer + packet->bufferSize - packet->limit - 0x8000) {
        return 0;
    }
    packet->cursor = packet->cursor + (count << 4) + 16;
    return cursor;
}

/*
 * Rounds a requested size up to the next power of two: the floor is 2 and
 * any size above 0x800 returns 1.
 */
int xglRoundUpPow2(int size)
{
    size = size;
    if (size < 3)
    {
        return 2;
    }
    if (size < 5)
    {
        return 4;
    }
    if (size < 9)
    {
        return 8;
    }
    if (size < 0x11)
    {
        return 0x10;
    }
    if (size < 0x21)
    {
        return 0x20;
    }
    if (size < 0x41)
    {
        return 0x40;
    }
    if (size < 0x81)
    {
        return 0x80;
    }
    if (size < 0x101)
    {
        return 0x100;
    }
    if (size < 0x201)
    {
        return 0x200;
    }
    if (size < 0x401)
    {
        return 0x400;
    }
    if (size < 0x801)
    {
        return 0x800;
    }
    return 1;
}

int xglRoundUpLog2(int size)
{
    if (size < 3) {
        return 1;
    }
    if (size < 5) {
        return 2;
    }
    if (size < 9) {
        return 3;
    }
    if (size < 0x11) {
        return 4;
    }
    if (size < 0x21) {
        return 5;
    }
    if (size < 0x41) {
        return 6;
    }
    if (size < 0x81) {
        return 7;
    }
    if (size < 0x101) {
        return 8;
    }
    if (size < 0x201) {
        return 9;
    }
    if (size < 0x401) {
        return 10;
    }
    if (size < 0x801) {
        return 11;
    }
    return 1;
}

int func_001D2E90(int size, int psm)
{
    int result;

    result = 1;
    switch (psm) {
    case 20:
        result = size / 128 * 2;
        if ((size & 0x7F) != 0) {
            result += 2;
        }
        break;
    case 19:
        result = size / 128 * 2;
        if ((size & 0x7F) != 0) {
            result += 2;
        }
        break;
    case 0:
        result = size / 128 * 2;
        if ((size & 0x7F) != 0) {
            result += 2;
        }
        break;
    }
    return result;
}

int xglPsmBufferWidth(int width, int psm)
{
    int result;

    result = 1;
    switch (psm) {
    case 20:
        result = width / 256 + 1;
        break;
    case 19:
        result = width / 128 + 1;
        break;
    case 0:
        result = width / 64;
        if ((width & 0x3F) != 0) {
            result += 1;
        }
        break;
    }
    return result;
}

void xglDirectQueueBegin(void)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    xglDirectQueueBuffer = (XglDirectQueueEntry *) packet->cursor;
    xglDirectQueueCount = 0;
    xglDirectQueueBuffer[xglDirectQueueCount].word[0] = 0;
    xglDirectQueueBuffer[xglDirectQueueCount].word[1] = 0;
    xglDirectQueueBuffer[xglDirectQueueCount].word[2] = 0;
    xglDirectQueueBuffer[xglDirectQueueCount++].word[3] = 0x50000000;
    xglDirectQueueBuffer[xglDirectQueueCount].data = 0x1000000000008000ULL;
    xglDirectQueueBuffer[xglDirectQueueCount++].address = 14;
}

void xglDirectQueuePush(u64 address, u64 data)
{
    xglDirectQueueBuffer[xglDirectQueueCount].data = data;
    xglDirectQueueBuffer[xglDirectQueueCount++].address = address;
}

void xglDirectQueuePushCopy(const XglDirectQueueEntry *entry)
{
    xglDirectQueueBuffer[xglDirectQueueCount++] = *entry;
}

extern void pxyAddEntryFull(int mode, void *buffer, int count, int a3, int offset);

void xglDirectQueueDefer(int flag, int key)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    xglDirectQueueBuffer[0].word[3] = xglDirectQueueCount + 0x4fffffff;
    xglDirectQueueBuffer[1].word[0] = xglDirectQueueCount + 0x7ffe;
    pxyAddEntryFull(key | 1, xglDirectQueueBuffer, xglDirectQueueCount, flag, xglDirectQueueRefOffset);
    packet->cursor += xglDirectQueueCount << 4;
    xglDirectQueueCount = 0;
    xglDirectQueueRefOffset = 0;
}


void xglDirectQueueSetRefOffset(s32 value)
{
    xglDirectQueueRefOffset = value;
}

void xglDirectQueueSubmitRef(void)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    xglDirectQueueBuffer[0].word[3] = xglDirectQueueCount + 0x4fffffff;
    xglDirectQueueBuffer[1].word[0] = xglDirectQueueCount + 0x7ffe;
    sceVif1PkRef(packet, xglDirectQueueBuffer, xglDirectQueueCount, 0, xglDirectQueueRefOffset, 0);
    packet->cursor += xglDirectQueueCount << 4;
    xglDirectQueueCount = 0;
    xglDirectQueueRefOffset = 0;
}


void pxyResetSubmitCount(void)
{
    pxySubmitCount = 0;
}

s32 pxyGetSubmitCount(void)
{
    return pxySubmitCount;
}

u32 func_0012B510(XglPacket *packet);

f32 func_001D34C4(void)
{
    XglPacket *packet;
    register f32 ratio;
    register u32 usedBytes;
    register u32 capacity;
    register f32 capacityF;

    packet = xglPacketGetCurrent();
    usedBytes = func_0012B510(packet) << 4;
    if ((s32) usedBytes >= 0) {
        ratio = (f32) (s32) usedBytes;
    } else {
        ratio = (f32) (s32) ((usedBytes & 1) | (usedBytes >> 1));
        ratio += ratio;
    }
    capacity = packet->limit - packet->buffer;
    if ((s32) capacity >= 0) {
        capacityF = (f32) (s32) capacity;
    } else {
        capacityF = (f32) (s32) ((capacity & 1) | (capacity >> 1));
        capacityF += capacityF;
    }
    ratio /= capacityF;
    ratio *= 100.0f;
    return ratio;
}

/* Percentage of the current packet's buffer in use. */
f32 xglPacketBufferUsedPercent(void)
{
    XglPacket *packet;
    register f32 ratio;
    register u32 usedBytes;

    packet = xglPacketGetCurrent();
    usedBytes = (u32) packet->cursor - packet->limit;
    ratio = usedBytes;
    ratio /= packet->buffer + packet->bufferSize - packet->limit;
    ratio *= 100.0f;
    return ratio;
}

void _CurSetViewScaleTrans(Vector4 *first, Vector4 *second);
void xglVectorScaleXYZ(Vector4 *destination, const Vector4 *source, float scale);

/* One screen position of the glow quad: GS X, Y and depth as integers, and the scale of the corner. */
typedef struct PxyScreenVertex {
    s32 x;   /* +0x00 */
    s32 y;   /* +0x04 */
    s32 z;   /* +0x08 */
    float w; /* +0x0C */
} PxyScreenVertex;


int _CurRotTransPersClipScreen(PxyScreenVertex *position, Vector4 *world);

#define GS_ST 0x02
#define GS_SET_ST(s, t) ((u64)(s) | ((u64)(t) << 32))
/* The bit pattern of a float, as the packet words carry it. */
#define FLOAT_BITS(f) (*(s32 *) &(f))

/* Four corner signs of the glow quad, viewed as floats or copied as two doublewords. */
typedef union GlowSigns {
    float f[4];
    u64 words[2];
} GlowSigns;

/*
 * Writes the glow quad of the active camera: four corners at the unit square, projected through the
 * camera's view, then the packet of the four vertices. Does nothing without a selected camera.
 */
/* Draws the blob shadow quad (textured with the circle texture sent by nmlPacketSendCircleTexture) under the model. */
void pxyDrawCircleShadow(PxyPacket *model)
{
    Matrix4 worldCorner;
    union { Matrix4 f; s32 words[4][4]; } texCoord;
    union { PxyScreenVertex v[4]; float f[4][4]; } screen;
    GlowSigns xSign = { { -1.0f, 1.0f, -1.0f, 1.0f } };
    GlowSigns zSign = { { -1.0f, -1.0f, 1.0f, 1.0f } };
    int i;
    float extent;
    u64 *p;
    u64 *start;

    extent = 1.0f;
    if (xglStudioSelectedCamera == 0) {
        return;
    }
    extent = (model->header->maxX - model->header->minX) + (model->header->maxZ - model->header->minZ);
    texCoord.f[0][0] = 0.0f;
    texCoord.f[0][1] = 0.0f;
    texCoord.f[0][2] = 1.0f;
    texCoord.f[1][0] = 1.0f;
    texCoord.f[1][1] = 0.0f;
    texCoord.f[1][2] = 1.0f;
    texCoord.f[2][0] = 0.0f;
    texCoord.f[2][1] = 1.0f;
    texCoord.f[2][2] = 1.0f;
    texCoord.f[3][0] = 1.0f;
    texCoord.f[3][1] = 1.0f;
    texCoord.f[3][2] = 1.0f;
    _CurMatrixSet(&xglStudioSelectedCamera->volume[0].viewMatrix);
    _CurSetViewScaleTrans(&xglStudioSelectedCamera->screen.screenScale,
                          &xglStudioSelectedCamera->screen.screenOffset);
    for (i = 0; i < 4; i++) {
        worldCorner[i][0] = model->shadowPosition.x + xSign.f[i] * extent * 0.35f;
        worldCorner[i][1] = model->shadowY + 0.005f;
        worldCorner[i][2] = model->shadowPosition.z + zSign.f[i] * extent * 0.35f;
        if (_CurRotTransPersClipScreen(&screen.v[i], (Vector4 *) worldCorner[i]) != 0) {
            xglVectorScaleXYZ(texCoord.f[i], texCoord.f[i], screen.f[i][3]);
            texCoord.words[i][3] = GS_XYZ3;
        } else {
            xglVectorScaleXYZ(texCoord.f[i], texCoord.f[i], screen.f[i][3]);
            texCoord.words[i][3] = GS_XYZF2;
        }
    }
    if (texCoord.words[2][3] == GS_XYZ3) {
        texCoord.words[3][3] = GS_XYZ3;
    }
    if (texCoord.words[1][3] == GS_XYZ3) {
        texCoord.words[2][3] = GS_XYZ3;
        texCoord.words[3][3] = GS_XYZ3;
    }
    if (texCoord.words[0][3] == GS_XYZ3) {
        texCoord.words[1][3] = GS_XYZ3;
        texCoord.words[2][3] = GS_XYZ3;
    }

    xglDirectQueueBegin();
    p = (u64 *) &xglDirectQueueBuffer[xglDirectQueueCount];
    start = p;
    GS_PUT(p, GS_TEXFLUSH, 0);
    GS_PUT(p, GS_TEX0_1,
           GS_SET_TEX0(sRenderTextureBase << 5, 1, GS_PSMT8, 6, 6, 1, 0, (sRenderTextureBase << 5) + 128, 0, 0, 0, 1));
    GS_PUT(p, GS_TEX1_1, GS_SET_TEX1(0, 0, 1, 1, 0, 0, 0));
    GS_PUT(p, GS_TEST_1, GS_SET_TEST(1, 0, 0, 1, 0, 0, 1, 3));
    GS_PUT(p, GS_CLAMP_1, 0);
    GS_PUT(p, GS_ALPHA_1, GS_SET_ALPHA(1, 2, 0, 2, 0));
    GS_PUT(p, GS_PRIM, GS_SET_PRIM(4, 1, 1, 0, 1, 0, 0, 0, 0));
    GS_PUT(p, GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, FLOAT_BITS(screen.f[0][3])));
    GS_PUT(p, GS_ST, GS_SET_ST(FLOAT_BITS(texCoord.f[0][0]), FLOAT_BITS(texCoord.f[0][1])));
    GS_PUT(p, texCoord.words[0][3], GS_SET_XYZ(screen.v[0].x, screen.v[0].y, (s32) ((float) screen.v[0].z * 1.01f)));
    GS_PUT(p, GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, FLOAT_BITS(screen.f[1][3])));
    GS_PUT(p, GS_ST, GS_SET_ST(FLOAT_BITS(texCoord.f[1][0]), FLOAT_BITS(texCoord.f[1][1])));
    GS_PUT(p, texCoord.words[1][3], GS_SET_XYZ(screen.v[1].x, screen.v[1].y, (s32) ((float) screen.v[1].z * 1.01f)));
    GS_PUT(p, GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, FLOAT_BITS(screen.f[2][3])));
    GS_PUT(p, GS_ST, GS_SET_ST(FLOAT_BITS(texCoord.f[2][0]), FLOAT_BITS(texCoord.f[2][1])));
    GS_PUT(p, texCoord.words[2][3], GS_SET_XYZ(screen.v[2].x, screen.v[2].y, (s32) ((float) screen.v[2].z * 1.01f)));
    GS_PUT(p, GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, FLOAT_BITS(screen.f[3][3])));
    GS_PUT(p, GS_ST, GS_SET_ST(FLOAT_BITS(texCoord.f[3][0]), FLOAT_BITS(texCoord.f[3][0])));
    GS_PUT(p, texCoord.words[3][3], GS_SET_XYZ(screen.v[3].x, screen.v[3].y, (s32) ((float) screen.v[3].z * 1.01f)));
    xglDirectQueueCount += ((u32) p - (u32) start) >> 4;
    xglDirectQueueSubmitRef();
}

#include "quadword.h"

u8 *xglPacketAppendVectors(void *matrices, int count);

/*
 * Builds the two GIF packets for the environment and the vectors. gifPacket
 * holds 12 quadwords of which the first five are sent.
 */
/* X1 nmlPacketSendCircleTexture: loads the 64x64 PSMT8 circle texture and its 16x16 PSMCT32 palette. */
void nmlPacketSendCircleTexture(const void *indices, void *palette)
{
    XglPacket *packet;
    u64 image[12];

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    func_0012B9A0(packet, 0x11000000);
    func_0012A8C0(image, (short)(sRenderTextureBase << 5), 1, 19, 0, 0, 64, 64);
    image[0] |= 0x8000;
    func_0012B7C0(packet, 0);
    sceVif1PkAddDirectDataN(packet, image, 5);
    func_0012B818(packet);
    sceVif1PkRef(packet, indices, 258, 0, 0, 0);
    sceVif1PkCnt(packet, 0);
    func_0012A8C0(image, (short)((sRenderTextureBase << 5) + 128), 1, 0, 0, 0, 16, 16);
    image[0] |= 0x8000;
    func_0012B7C0(packet, 0);
    sceVif1PkAddDirectDataN(packet, image, 5);
    func_0012B818(packet);
    sceVif1PkRef(packet, xglPacketAppendVectors(palette, 66), 66, 0, 0, 0);
}

void xglPacketUploadVu1Data(void *data, int count, int address);

/*
 * Builds the ten-quadword GIF packet (a tag, the A+D registers and two
 * float-scaled rectangles) in a 12-quadword buffer and hands it to xglPacketUploadVu1Data.
 */
void func_001D4180(void)
{
    GifQword block[12];
    GifQword *cursor;

    cursor = block;
    cursor->word[0] = 0x8006;
    cursor->word[1] = 0x10000000;
    cursor->word[2] = 14;
    cursor->word[3] = 0;
    cursor += 1;
    cursor->reg[0] = 0;
    cursor->reg[1] = 63;
    cursor += 1;
    cursor->reg[0] = 0x2000000640000000ULL | (0x24020000 | ((sRenderTextureBase << 5) + 128));
    cursor->reg[1] = 6;
    cursor += 1;
    cursor->reg[0] = 0xFF000FFFULL;
    cursor->reg[1] = 8;
    cursor += 1;
    cursor->reg[0] = 96;
    cursor->reg[1] = 20;
    cursor += 1;
    cursor->reg[0] = 0x50000;
    cursor->reg[1] = 71;
    cursor += 1;
    cursor->reg[0] = 0x101000000ULL | sRenderDepthBase;
    cursor->reg[1] = 78;
    cursor += 1;
    cursor->value[0] = (float)cameraScreenWidth / 2.0f - 2048.0f;
    cursor->value[1] = (float)cameraScreenHeight / 2.0f - 2048.0f;
    cursor->word[2] = 0;
    cursor->word[3] = 0;
    cursor += 1;
    cursor->value[0] = (256.0f / (float)cameraScreenWidth) / 512.0f;
    cursor->value[1] = (256.0f / (float)cameraScreenHeight) / 512.0f;
    cursor->word[2] = 0;
    cursor->word[3] = 0;
    cursor += 1;
    cursor->value[0] = 1.0f;
    cursor->value[1] = 1.0f;
    cursor->word[2] = 0;
    cursor->word[3] = 0;
    cursor += 1;
    xglPacketUploadVu1Data(block, 10, 840);
}

/* Small-data word at 0x0069E10C, one of the unnamed .sdata cells between xglDirectQueueRefOffset and pxyFlushDone. */

void func_001D4498(int flag)
{
    D_0069E10C = flag;
}

void xglMatrixCopy_001D44C4(register Matrix4 *destination, register const Matrix4 *source)
{
    QuadwordCopyMatrix(destination, source);
}

void xglVectorZero_001D4508(register Vector4 *destination)
{
    QuadwordZero(destination);
}

#include "vu0_macro.h"

void xglVectorSubXYZ_001D4530(register AlignedVector4 *dst, register AlignedVector4 *minuend, register AlignedVector4 *subtrahend)
{
    Vu0SubVectorXYZ(dst, minuend, subtrahend);
}
