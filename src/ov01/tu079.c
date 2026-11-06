/*
 * OV01 original TU 79: 0x00aae0a8..0x00aaee00 (7 functions)
 */
#include "common.h"
#include "main/nml_model_set.h"

typedef int s32;

typedef long long s64;

typedef float f32;

/* The sub-record of an effect object: the two transparency values the
 * stencil draws read. */
typedef struct MEfEffectSub {
    unsigned char unmodeled_00[0x470];
    f32 transparency_a;            /* 0x470 */
    unsigned char unmodeled_474[0x660 - 0x474];
    f32 transparency_b;            /* 0x660 */
} MEfEffectSub;

typedef struct MEfEffectObject {
    unsigned char unmodeled_00[4];
    s32 slot;                      /* 0x04: passed to svGetPrmFromName */
    unsigned char unmodeled_08[0x20 - 0x08];
    MEfEffectSub sub;              /* 0x20 */
} MEfEffectObject;

void func_001BB4AC(s32 value);
void func_001BFD48(s32 value);
void svGetPrmFromName(void *table, s32 slot, s32 *params);
void nmlModelSetTransparency(f32 transparency);
extern s64 D_00AB8C40;
extern unsigned char D_00ABB450[];

/* Four passes of the stencil draw. The parameter record (8 ints) is looked up
 * by slot; params[4] and params[5] feed the two calls after the model setup. */
void MEfDrawStencilA(MEfEffectObject *obj)
{
    MEfEffectSub *sub;
    s32 params[8];
    s32 cursor;
    s32 remaining;

    sub = &obj->sub;
    svGetPrmFromName(D_00ABB450, obj->slot, params);
    cursor = (s32) obj + 0xD0;
    remaining = 4;
    do {
        remaining -= 1;
        nmlModelSetStencil(1);
        nmlModelSetStencilAlpha(D_00AB8C40);
        nmlModelSetTransparency(sub->transparency_a);
        func_001BB4AC(params[5]);
        nmlModelSetDrawMatrix(cursor);
        cursor += 0x40;
        func_001BFD48(params[4]);
    } while (remaining >= 0);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu079", func_00AAE148);

INCLUDE_ASM("asm/nonmatchings/ov01/tu079", func_00AAE478);

INCLUDE_ASM("asm/nonmatchings/ov01/tu079", func_00AAE748);

INCLUDE_ASM("asm/nonmatchings/ov01/tu079", func_00AAE938);

extern s64 D_00AB8C48;
extern unsigned char D_00ABB4D0[];

/* Same four-pass stencil draw as MEfDrawStencilA, with the second table, value
 * and transparency. */
void MEfDrawStencilB(MEfEffectObject *obj)
{
    MEfEffectSub *sub;
    s32 params[8];
    s32 cursor;
    s32 remaining;

    sub = &obj->sub;
    svGetPrmFromName(D_00ABB4D0, obj->slot, params);
    cursor = (s32) obj + 0xC0;
    remaining = 4;
    do {
        remaining -= 1;
        nmlModelSetStencil(1);
        nmlModelSetStencilAlpha(D_00AB8C48);
        nmlModelSetTransparency(sub->transparency_b);
        func_001BB4AC(params[5]);
        nmlModelSetDrawMatrix(cursor);
        cursor += 0x40;
        func_001BFD48(params[4]);
    } while (remaining >= 0);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu079", func_00AAEAE8);
