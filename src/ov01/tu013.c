/*
 * OV01 original TU 13: 0x00a85a80..0x00a862d8 (9 functions)
 */
#include "common.h"
#include "ov01/tu071.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu013", func_00A85A80);

INCLUDE_ASM("asm/nonmatchings/ov01/tu013", func_00A85B68);

INCLUDE_ASM("asm/nonmatchings/ov01/tu013", func_00A85CD8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu013", unitCmdCodeGet);

INCLUDE_ASM("asm/nonmatchings/ov01/tu013", unitCmdModeGet);

/*
 * The unit's work record (the object the step functions receive). Only the
 * flags word is accessed here; the record is larger (see UnitParam pointer at
 * +0x144 in src/ov01/tu015.c).
 */
typedef struct UnitActor {
    unsigned char unmodeled_00[0xC];
    unsigned int flags;
} UnitActor;

/*
 * The step record passed with the object. status (+0) is the command code
 * that the move dispatchers store before they call a move; phase (+4) is zero
 * until the one-shot start has run.
 */
typedef struct UnitStep {
    int status;
    int phase;
} UnitStep;

void func_00A849C8(void *object, int code);
void unitRefreshPhase(void *object);
int unitIsDone(void *object);

/* One-shot start: issues command 0x18 and effect 0xB, then waits for the command to finish. */
int func_00A85EA0(UnitActor *object, UnitStep *step)
{
    if (step->phase == 0) {
        func_00A849C8(object, 0x18);
        seObjectVoiceStartDefault(object, 0xB);
        step->phase = step->phase + 1;
    } else if (unitIsDone(object) != 0) {
        unitRefreshPhase(object);
        object->flags &= ~1;
        return 1;
    }
    return 0;
}

/* Same shape as func_00A85EA0 with command 0x19 and effect 0xC. */
int func_00A85F30(UnitActor *object, UnitStep *step)
{
    if (step->phase == 0) {
        func_00A849C8(object, 0x19);
        seObjectVoiceStartDefault(object, 0xC);
        step->phase = step->phase + 1;
    } else if (unitIsDone(object) != 0) {
        unitRefreshPhase(object);
        object->flags &= ~1;
        return 1;
    }
    return 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu013", func_00A85FC0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu013", func_00A86270);
