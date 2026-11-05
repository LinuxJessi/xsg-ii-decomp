#include "common.h"
#include "shared.h"

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E0B10);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E0B90);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E0BC8);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E0C78);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E0CC8);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E0E60);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E0E78);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E0E98);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E0EB0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E0F18);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E0FD0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E11C8);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E11F0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1218);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E12B8);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1440);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1508);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1680);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E16A0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1830);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1978);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1A40);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1B10);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1B18);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1BC0);

u8 *next_line(u8 *cursor)
{
    for (;;) {
        while (*cursor != '\n' && *cursor != '\r')
            cursor++;
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r')
            cursor++;
        if (*cursor != ';')
            return cursor;
    }
}

u8 *skip_space(u8 *cursor)
{
    while (*cursor == ' ' || *cursor == '\t')
        cursor++;
    return cursor;
}

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1D10);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1EE8);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E1FE0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", act);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", map);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", mot);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", mobj);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", effect);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", mapse);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", runtime_001E2300);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", bgmvol);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", foot);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", enefoot);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", disk);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", save);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E2978);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E29A0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E2E08);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E2ED0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E3000);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E3028);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E3308);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E3630);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E36F8);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E3838);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E3910);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E3930);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E39D0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E3FD8);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E4000);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E40E0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E4178);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E4220);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E43F8);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E44F0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E4700);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E47E0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E4890);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E4AE0);

INCLUDE_ASM("asm/main/nonmatchings/res_get_path", func_001E4B60);
