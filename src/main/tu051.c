#include "common.h"
#include "main/tu051.h"
#include "main/xgl_2_031.h"

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001D9280);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraSaveState);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraRestoreState);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001D9580);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001D95F0);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraInit);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraReset);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraSetMode);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraSetIndex);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraSlotSetup);

#include "shared.h"

/* One of the four camera slots: indexA/indexB and the enable flags, set by cameraSlotSetup. */
struct CameraSlot {
    unsigned char unmodeled_00[2];
    signed char indexA;                 /* +0x02: stored by cameraSlotSetup when >= 0 */
    signed char indexB;                 /* +0x03: stored by cameraSlotSetup when >= 0 */
    unsigned char unmodeled_04[4];
    float enabled;                      /* +0x08: the enable flag as a float; a slot is active when nonzero */
    float disabled;                     /* +0x0C: 1.0 when the enable flag was 0 */
};

/* The camera state block at gameWork. */
struct CameraState {
    unsigned char unmodeled_000[0xD0];
    unsigned short flags;               /* +0xD0: 0x10 set by cameraApplySlots, 0x20 by cameraSetIndex and cameraApplyAllSlots, 0x40 by cameraSetMode */
    unsigned char unmodeled_0D2[3];
    signed char mode;                   /* +0xD5: set by cameraSetMode */
    unsigned char unmodeled_0D6[6];
    signed char trackIndex;             /* +0xDC: set by cameraSetIndex */
    unsigned char unmodeled_0DD[0x63];
    struct CameraSlot slot[4];
    unsigned char unmodeled_180[0x20];
    /* src_* are staged vectors and dst_* the applied copies that cameraApplySlots writes per slot;
       the meaning of each vector is not established yet. */
    float src_1a0;
    float src_1a4;
    float src_1a8;
    unsigned char unmodeled_1AC[0x14];
    float src_1c0;
    float src_1c4;
    float src_1c8;
    float src_1cc;
    float src_1d0;
    float src_1d4;
    float src_1d8;
    float src_1dc;
    float src_1e0;
    float src_1e4;
    unsigned char unmodeled_1E8[0x4];
    float src_1ec;
    float dst_1f0;
    float dst_1f4;
    float dst_1f8;
    float dst_1fc;
    float dst_200;
    float dst_204;
    float dst_208;
    float dst_20c;
    float dst_210;
    float dst_214;
    unsigned char unmodeled_218[0x4];
    float dst_21c;
    unsigned char unmodeled_220[0x30];
    float dst_250;
    float dst_254;
    float dst_258;
};

extern struct CameraState gameWork;
extern unsigned char D_002B5ED0[];
extern unsigned char D_002B5EE0[];
extern unsigned char D_002B5EF0[];
extern unsigned char D_002B5F00[];
int cameraSlotSetup(void *block, int index_a, int index_b, int enable_flag);

/* Applies the slots in slot_mask (zero: every slot whose enabled flag is set) and copies each slot's staged
   vectors to its applied fields; sets flag 0x10 when enable_flag is nonzero. */
void cameraApplySlots(int slot_mask, int index_a, int index_b, int enable_flag)
{
    if (slot_mask == 0) {
        if (gameWork.slot[0].enabled != 0.0f) {
            slot_mask = 1;
        }
        if (gameWork.slot[1].enabled != 0.0f) {
            slot_mask |= 2;
        }
        if (gameWork.slot[2].enabled != 0.0f) {
            slot_mask |= 4;
        }
        if (gameWork.slot[3].enabled != 0.0f) {
            slot_mask |= 8;
        }
        if (slot_mask == 0) {
            return;
        }
    }
    if (slot_mask & 1) {
        struct CameraState *state = (struct CameraState *)((unsigned char *)&D_002B5ED0 - 0x140);
        cameraSlotSetup(&D_002B5ED0, index_a, index_b, enable_flag);
        state->dst_200 = state->src_1d0;
        state->dst_204 = state->src_1d4;
        state->dst_208 = state->src_1d8;
    }
    if (slot_mask & 2) {
        struct CameraState *state = (struct CameraState *)((unsigned char *)&D_002B5EE0 - 0x150);
        cameraSlotSetup(&D_002B5EE0, index_a, index_b, enable_flag);
        state->dst_1f0 = state->src_1c0;
        state->dst_1f4 = state->src_1c4;
        state->dst_1f8 = state->src_1c8;
        state->dst_210 = state->src_1e0;
        state->dst_214 = state->src_1e4;
        state->dst_21c = state->src_1ec;
        state->dst_250 = state->src_1a0;
        state->dst_254 = state->src_1a4;
        state->dst_258 = state->src_1a8;
    }
    if (slot_mask & 4) {
        struct CameraState *state = (struct CameraState *)((unsigned char *)&D_002B5EF0 - 0x160);
        cameraSlotSetup(&D_002B5EF0, index_a, index_b, enable_flag);
        state->dst_1fc = state->src_1cc;
    }
    if (slot_mask & 8) {
        struct CameraState *state = (struct CameraState *)((unsigned char *)&D_002B5F00 - 0x170);
        cameraSlotSetup(&D_002B5F00, index_a, index_b, enable_flag);
        state->dst_20c = state->src_1dc;
    }
    if (enable_flag != 0) {
        gameWork.flags |= 0x10;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraAnchorGet);

/* Anchor record returned by cameraAnchorGet: the yaw and pitch used to turn a camera offset. */
struct CameraAnchor {
    unsigned char unmodeled_00[0x50];
    float rotation_x;
    float rotation_y;
};

void func_001AA8F4();
struct CameraAnchor *cameraAnchorGet(Vector4 *, int, int);
void xglMatrixStackRotX(float);
void xglMatrixStackRotY(float);
void xglMatrixStackSave(float *);

/* Moves position by offset. With a valid anchor and a unit offset.w, the offset is turned by the anchor's
   yaw and pitch first; otherwise it is added as it is. Returns the anchor. */
struct CameraAnchor *cameraApplyAnchorOffset(Vector4 *position, int anchorIndex, int anchorSubIndex, Vector4 *offset)
{
    float matrix[16];
    struct CameraAnchor *node;

    node = cameraAnchorGet(position, anchorIndex, anchorSubIndex);
    if (node != 0 && offset != 0 && offset->w == 1.0f) {
        xglMatrixStackUnit();
        xglMatrixStackRotY(node->rotation_y);
        xglMatrixStackRotX(node->rotation_x);
        func_001AA8F4(offset);
        xglMatrixStackSave(matrix);
        position->x += matrix[12];
        position->y += matrix[13];
        position->z += matrix[14];
    } else if (offset != 0) {
        position->x += offset->x;
        position->y += offset->y;
        position->z += offset->z;
    }
    return node;
}

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraApplyAllSlots);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraSlotFind);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraOrbitPosition);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraAnglesBetween);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001D9FD0);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DA210);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DA468);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DAA38);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DABE8);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DAD30);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DADD0);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DAF10);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DB240);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraEase);

/* Linear interpolation from `from` to `to` by `fraction`. */
float cameraLerpFloat(float from, float to, float fraction)
{
    return ((to - from) * fraction) + from;
}

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraLerpAngle);

/* VU0 linear interpolation of two vectors into destination, by fraction. */
void cameraLerpVector(Vector4 *destination, const Vector4 *from, const Vector4 *to, float fraction)
{
    __asm__ __volatile__(
        "lqc2 vf4, 0(%0)\n\t"
        "lqc2 vf2, 0(%1)\n\t"
        "lqc2 vf1, 0(%2)\n\t"
        "qmtc2.ni %3, vf3\n\t"
        "vsub.xyz vf4, vf2, vf1\n\t"
        "vmulx.xyz vf2, vf4, vf3x\n\t"
        "vadd.xyz vf4, vf2, vf1\n\t"
        "sqc2 vf4, 0(%0)"
        : : "r"(destination), "r"(to), "r"(from), "r"(fraction) : "memory");
}

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DB580);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraAdvanceTracks);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraUpdateFrame);

INCLUDE_ASM("asm/main/nonmatchings/tu051", cameraApplyView);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DC378);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DC468);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DC568);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DC860);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DCE68);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DD1D8);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DE738);

INCLUDE_ASM("asm/main/nonmatchings/tu051", func_001DE8F0);

/* Bytes per entry of the table at uwamonoRecords (UwamonoRecord in init_uwamono_sys.c). */
#define UWAMONO_RECORD_SIZE 0x870

extern int uwamonoCurrentIndex;
extern unsigned char uwamonoRecords[];

/* The record at uwamonoCurrentIndex, or null while no record is selected (index below zero). */
void *uwamonoCurrentRecord(void)
{
    if (!(uwamonoCurrentIndex >= 0)) {
        return 0;
    }
    return uwamonoRecords + (uwamonoCurrentIndex * UWAMONO_RECORD_SIZE);
}
