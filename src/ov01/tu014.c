/*
 * OV01 original TU 14: 0x00a862d8..0x00a865d8 (8 functions)
 */
#include "common.h"

/* Always reports done. No caller evidence for a name. */
int func_00A862D8(void)
{
    return 1;
}

int func_00A87508();
void objMotionCopyLookup();

/*
 * Character target, run mode (func_00A87508 with mode 1 and flag 0, as the
 * X1 unitCmdRun(unit, 1, 0) call); on success finishes the move with
 * objMotionCopyLookup(handle, 0), the unitCmdMoveNext(unit, 0) call.
 */
int unitCmdMoveChaRun(int handle)
{
    if (func_00A87508(handle, 1, 0) != 0) {
        objMotionCopyLookup(handle, 0);
        return 1;
    }
    return 0;
}

/* Position target, mode 4 with flag 1 (func_00A876E0), then objMotionCopyLookup(handle, 1). Called from unitCmdMovePos. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu014", func_00A86328);

/* Character target, mode 3 with flag 0 (func_00A876E0), then objMotionCopyLookup(handle, 0). Called from unitCmdMoveCha. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu014", func_00A86370);

int func_00A87930();

/*
 * Character target, hover mode (func_00A87930 with mode 5, flag 0, the X1
 * unitCmdHover(unit, 5, 0) call); finishes with objMotionCopyLookup(handle, 0).
 */
int unitCmdMoveChaHover(int handle)
{
    if (func_00A87930(handle, 5, 0) != 0) {
        objMotionCopyLookup(handle, 0);
        return 1;
    }
    return 0;
}

/*
 * Position target, hover mode (func_00A87930 with mode 6, flag 1, the X1
 * unitCmdHover(unit, 6, 1) call); finishes with objMotionCopyLookup(handle, 1).
 */
int unitCmdMovePosHover(int handle)
{
    if (func_00A87930(handle, 6, 1) != 0) {
        objMotionCopyLookup(handle, 1);
        return 1;
    }
    return 0;
}

/*
 * Character-target dispatcher: func_00A87030(object, 3, 0) (the X1
 * unitCmdDstSet(unit, 3, 0) call), then branches on the kind byte at +0x15 of
 * the record at object+0x144 and stores the command code into step->status.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu014", unitCmdMoveCha);

/*
 * Position-target dispatcher: func_00A87030(object, 4, 1) (the X1
 * unitCmdDstSet(unit, 4, 1) call), then the same kind dispatch as
 * unitCmdMoveCha with its own codes; sets the flag bit 0x1000 on exit.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu014", unitCmdMovePos);
