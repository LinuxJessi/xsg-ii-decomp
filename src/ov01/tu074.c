/*
 * OV01 original TU 74: 0x00aaab80..0x00aaad58 (3 functions)
 */
#include "common.h"

/* Scene driver (proposed ov01SceneRun): setup chain, then the frame loop. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu074", ov01SceneRun);

/* Studio camera setup (proposed ov01StudioCameraSetup): four groups of eight cameras. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu074", ov01StudioCameraSetup);

/* Empty function; the dossier records no caller and no string. */
void func_00AAAD50(void)
{
}
