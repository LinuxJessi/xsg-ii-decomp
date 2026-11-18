/*
 * OV01 original TU 14: 0x00a862d8..0x00a865d8 (8 functions)
 */
#include "common.h"
#include "ov01/tu012.h"
#include "ov01/tu016.h"

/* Always reports done. No caller evidence for a name. */
int func_00A862D8(void)
{
    return 1;
}

/*
 * Character target, run mode (unitCmdRun with mode 1 and flag 0, as the
 * X1 unitCmdRun(object, 1, 0) call); on success finishes the move with
 * objMotionCopyLookup(object, 0), the unitCmdMoveNext(object, 0) call.
 */
int unitCmdMoveChaRun(Ov01Object *object, Ov01Command *step)
{
    if (unitCmdRun(object, 1, 0) != 0) {
        objMotionCopyLookup(object, 0);
        return 1;
    }
    return 0;
}

/*
 * Command 0x17, position target: mode 4 with flag 1 (unitCmdMoveJump), then
 * objMotionCopyLookup(object, 1). Called from unitCmdMovePos.
 */
int unitCmdMovePosJump(Ov01Object *object, Ov01Command *step)
{
    int done;

    done = unitCmdMoveJump(object, 4, 1);
    if (done != 0) {
        objMotionCopyLookup(object, 1);
        done = 1;
    }
    return done;
}

/*
 * Command 0x18, character target: mode 3 with flag 0 (unitCmdMoveJump), then
 * objMotionCopyLookup(object, 0). Called from unitCmdMoveCha.
 */
int unitCmdMoveChaJump(Ov01Object *object, Ov01Command *step)
{
    int done;

    done = unitCmdMoveJump(object, 3, 0);
    if (done != 0) {
        objMotionCopyLookup(object, 0);
        done = 1;
    }
    return done;
}

/*
 * Character target, hover mode (unitCmdHoverFade with mode 5, flag 0, the X1
 * unitCmdHover(object, 5, 0) call); finishes with objMotionCopyLookup(object, 0).
 */
int unitCmdMoveChaHover(Ov01Object *object, Ov01Command *step)
{
    if (unitCmdHoverFade(object, 5, 0) != 0) {
        objMotionCopyLookup(object, 0);
        return 1;
    }
    return 0;
}

/*
 * Position target, hover mode (unitCmdHoverFade with mode 6, flag 1, the X1
 * unitCmdHover(object, 6, 1) call); finishes with objMotionCopyLookup(object, 1).
 */
int unitCmdMovePosHover(Ov01Object *object, Ov01Command *step)
{
    if (unitCmdHoverFade(object, 6, 1) != 0) {
        objMotionCopyLookup(object, 1);
        return 1;
    }
    return 0;
}

/*
 * Character-target dispatcher: unitCmdDstSet(object, 3, 0) (the X1
 * unitCmdDstSet(object, 3, 0) call), then branches on the kind byte at +0x15 of
 * the record at object+0x144 and stores the command code into step->command.
 */
int unitCmdMoveCha(Ov01Object *object, Ov01Command *step)
{
    int result = 1;

    unitCmdDstSet(object, 3, 0);
    switch (object->stats->subState) {
    case 0:
    case 1:
    case 4:
        step->command = UNIT_CMD_MOVE_CHA_RUN;
        result = unitCmdMoveChaRun(object, step);
        break;
    case 2:
        step->command = UNIT_CMD_MOVE_CHA_JUMP;
        result = unitCmdMoveChaJump(object, step);
        break;
    case 3:
        step->command = UNIT_CMD_MOVE_CHA_HOVER;
        result = unitCmdMoveChaHover(object, step);
        break;
    }
    return result;
}

/*
 * Position-target dispatcher: unitCmdDstSet(object, 4, 1) (the X1
 * unitCmdDstSet(object, 4, 1) call), then the same kind dispatch as
 * unitCmdMoveCha with its own codes; sets the flag bit 0x1000 on exit.
 */
int unitCmdMovePos(Ov01Object *object, Ov01Command *step)
{
    int result;

    unitCmdDstSet(object, 4, 1);
    result = 1;
    switch (object->stats->subState) {
    case 0:
    case 2:
    case 4:
        step->command = UNIT_CMD_MOVE_POS_JUMP;
        result = unitCmdMovePosJump(object, step);
        break;
    case 3:
        step->command = UNIT_CMD_MOVE_POS_HOVER;
        result = unitCmdMovePosHover(object, step);
        break;
    }
    object->flags = (object->flags & ~1) | 0x1000;
    return result;
}
