#include "common.h"
#include "shared.h"

#include "vu0_macro.h"

void sefInitClipViewVolume(void *view);
void sdvDrawAlters(int layer);

extern int _draw3D;

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002551D0);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00255320);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00255358);

void sefInitClipViewVolume(void *view) {
    Vu0SetClipMatrices(view);
}

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", sefClipViewVolumeA);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002554F8);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00255648);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002557D0);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00255960);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00255BF8);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00255D20);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00255D50);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00255DA0);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00256218);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002566B8);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002568F0);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00256B38);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00256E48);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00257190);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00257338);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00257500);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00257608);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00257638);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002580D0);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00258120);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002581A8);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00258640);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002587D8);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00258970);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002589B8);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00258A28);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00258CF0);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00258D28);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00258F98);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00259070);

void svDrawAlters(int layer)
{
    if (_draw3D == 0) {
        sdvDrawAlters(layer);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002590F0);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002593A0);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_002593D0);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_161", func_00259400);
