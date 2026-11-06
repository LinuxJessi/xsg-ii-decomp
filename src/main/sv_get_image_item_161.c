#include "common.h"
#include "shared.h"
#include "vu0_macro.h"

void sefInitClipViewVolume(void *view);
void sdvDrawAlters(int layer);
extern int _draw3D;

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002551D0);

typedef struct XglPacket XglPacket;

/* SvDrawItem: the fields the functions of this file read (0x180, 0x1CC, 0x8A4, 0x8AA). */
typedef struct SvDrawItem {
    unsigned char unmodeled_00[0x180];
    short *entries;
    unsigned char unmodeled_184[0x48];
    unsigned short flags;
    unsigned char unmodeled_1CE[0x8AA - 0x1CE];
    short fallbackKind;
} SvDrawItem;

void sceVif1PkRef(XglPacket *packet, const void *environment, int count, int mode, int offset, int flags);
void sceVif1PkCnt(XglPacket *packet, int count);
XglPacket *xglPacketGetCurrent(void);
void svLoadImageList(void *mapper);
int svDrawListBegin(int);
void svDrawScene(void);
f32 MMathVectorDotProduct(void);
void func_00257500(int, short, int, int, int);
extern unsigned char svMapperPacketEnv[];
extern int svActiveMapperIndex;
extern int D_0069E2EC;
extern int D_0069DB1C;
extern unsigned char _imageMapper[];

/* Refers the current packet to the three-quadword environment block svMapperPacketEnv; runs after every mapper switch. */
void svRefMapperPacket(void) {
    sceVif1PkRef(xglPacketGetCurrent(), svMapperPacketEnv, 3, 0, 0, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00255358);

void sefInitClipViewVolume(void *view) {
    Vu0SetClipMatrices(view);
}

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", sefClipViewVolumeA);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002554F8);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00255648);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002557D0);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", svSetModelRenderState);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", svProjectVertex);

/* Signed distance of a point (its w) from a clip plane whose offset comes from MMathVectorDotProduct. */
f32 sefClipPlaneDistance(Vector4 *vector) {
    return -(MMathVectorDotProduct() + vector->w);
}

/* Intersects the segment p1-p2 with a plane (VU0 dot products, one divide); writes the crossing point. */
void sefClipEdgeToPlane(Vector4 *out, const Vector4 *plane, const Vector4 *p1, const Vector4 *p2) {
    __asm__ __volatile__("vmove.xyzw vf4, vf0" : : : "memory");
    __asm__ __volatile__("lqc2 vf1, 0(%0)" : : "r"(p1) : "memory");
    __asm__ __volatile__("lqc2 vf2, 0(%0)" : : "r"(p2) : "memory");
    __asm__ __volatile__("lqc2 vf3, 0(%0)" : : "r"(plane) : "memory");
    __asm__ __volatile__("vsub.xyz vf4, vf1, vf2" : : : "memory");
    __asm__ __volatile__("vmul.xyz vf5, vf3, vf1" : : : "memory");
    __asm__ __volatile__("vaddy.x vf5, vf5, vf5y" : : : "memory");
    __asm__ __volatile__("vaddz.x vf5, vf5, vf5z" : : : "memory");
    __asm__ __volatile__("vmul.xyz vf6, vf3, vf4" : : : "memory");
    __asm__ __volatile__("vaddy.x vf6, vf6, vf6y" : : : "memory");
    __asm__ __volatile__("vaddz.x vf6, vf6, vf6z" : : : "memory");
    __asm__ __volatile__("vaddw.x vf5, vf5, vf3w" : : : "memory");
    __asm__ __volatile__("vsub.x vf5, vf0, vf5" : : : "memory");
    __asm__ __volatile__("vdiv Q, vf5x, vf6x" : : : "memory");
    __asm__ __volatile__("vwaitq" : : : "memory");
    __asm__ __volatile__("vmulq.xyz vf4, vf4, Q" : : : "memory");
    __asm__ __volatile__("vadd.xyz vf4, vf4, vf1" : : : "memory");
    __asm__ __volatile__("sqc2 vf4, 0(%0)" : : "r"(out) : "memory");
}

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", sefClipPolygon);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00256218);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002566B8);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002568F0);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00256B38);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00256E48);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00257190);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00257338);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00257500);

/* Reads the kind of the item's entry two places before entries[0]; kinds 0x10 and 0x11 use fallbackKind instead. */
void svInitItemDrawKind(int first, SvDrawItem *item, int third, int fourth, int fifth) {
    short kind;

    kind = item->entries[-2];
    if ((unsigned int) (kind - 0x10) < 2U) {
        kind = item->fallbackKind;
    }
    func_00257500(first, kind, third, fourth, fifth);
}

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", svDrawImageItem);

/* Makes mapper `index` the active one (svActiveMapperIndex caches it) and refers its packet; does nothing when it is already active. */
void svSelectImageMapper(int index) {
    if (svActiveMapperIndex != index) {
        svActiveMapperIndex = index;
        svLoadImageList(_imageMapper + index * 0x241C);
        svRefMapperPacket();
    }
}

/* Chooses the mapper for an item: mapper 0 when its 0x2000 flag is set, else the requested index D_0069E2EC. Does nothing while _draw3D is 2. */
void svSelectImageMapperForItem(SvDrawItem *item) {
    int index;

    if (_draw3D != 2) {
        if (item->flags & 0x2000) {
            if (svActiveMapperIndex != 0) {
                svActiveMapperIndex = 0;
                svLoadImageList(_imageMapper);
                svRefMapperPacket();
            }
        } else {
            index = D_0069E2EC;
            if (svActiveMapperIndex != index) {
                svActiveMapperIndex = index;
                svLoadImageList(_imageMapper + index * 0x241C);
                svRefMapperPacket();
            }
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002581A8);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00258640);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002587D8);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00258970);

void func_0012B6E0(XglPacket *packet, int, int, int, int);
void func_0012B770(XglPacket *packet);
void func_0012B9A0(XglPacket *packet, int);
void func_0012BCE8(XglPacket *packet, int, int);

/* Builds the draw-state packet: a count, a 0x11000000 tag, two sized blocks (0x14 and 0x6C), then 0x10 bytes from D_0069DB1C. */
void svBuildDrawStatePacket(void) {
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    func_0012B9A0(packet, 0x11000000);
    func_0012B6E0(packet, 0x14, 0x6C, 4, 4);
    func_0012BCE8(packet, D_0069DB1C, 0x10);
    func_0012B770(packet);
}

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", svDrawEntry);

/* Transforms a point by the clip matrix (vf21-vf24, set by sefInitClipViewVolume); a null input means the origin. */
void sefTransformClipPoint(Vector4 *out, const Vector4 *in) {
    if (in != 0) {
        __asm__ __volatile__("lqc2 vf1, 0(%0)" : : "r"(in) : "memory");
    } else {
        __asm__ __volatile__("vmove.xyzw vf1, vf0" : : : "memory");
    }
    __asm__ __volatile__("vmulax.xyzw ACC, vf21, vf1x" : : : "memory");
    __asm__ __volatile__("vmadday.xyzw ACC, vf22, vf1y" : : : "memory");
    __asm__ __volatile__("vmaddaz.xyzw ACC, vf23, vf1z" : : : "memory");
    __asm__ __volatile__("vmaddw.xyzw vf1, vf24, vf1w" : : : "memory");
    __asm__ __volatile__("sqc2 vf1, 0(%0)" : : "r"(out) : "memory");
}

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", sefCullSchedulerEntries);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", svDrawEntriesByKind);

/* Makes mapper `index` the active one without touching the cache; indices of 0x100 and above are ignored. */
void svForceImageMapper(unsigned int index) {
    if (index < 0x100U) {
        svLoadImageList(_imageMapper + index * 0x241C);
        svRefMapperPacket();
    }
}

/* Draws the alter layer only in 2D mode (_draw3D == 0). */
void svDrawAlters(int layer)
{
    if (_draw3D == 0) {
        sdvDrawAlters(layer);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", svDrawScene);

/* Sets the 3D flag and draws the scene when svDrawListBegin(0) reports it. */
void svDrawScene3D(void) {
    if (svDrawListBegin(0) != 0) {
        _draw3D = 1;
        svDrawScene();
    }
}

/* Clears the 3D flag and draws the scene when svDrawListBegin(1) reports it. */
void svDrawScene2D(void) {
    _draw3D = 0;
    if (svDrawListBegin(1) != 0) {
        svDrawScene();
    }
}

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00259400);
