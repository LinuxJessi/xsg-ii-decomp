/*
 * OV01 original TU 71: 0x00aa90c8..0x00aa9340 (7 functions)
 */
#include "common.h"
#include "main/tu022.h"
#include "main/tu063.h"
#include "ov01/tu008.h"
#include "ov01/tu012.h"
#include "ov01/tu071.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu071", seObjectVoiceStart);

#include "shared.h"

void seObjectVoiceStart(void *, int, int);

/* Starts the object's voice for `code` at the default SE volume (volume -1). */
void seObjectVoiceStartDefault(void *object, int code)
{
    seObjectVoiceStart(object, code, -1);
}

void adxSetSlotFile(int slot, const char *file);

/* Sets the file of stream slot `slot` to `handle`. */
void streamSlotSetFile(int slot, const char *file)
{
    adxSetSlotFile(slot, file);
}


/* Stops a stream slot (bgmStop passes 0). The original forwards its argument to soundSlotStop. */
void streamSlotStop(int index)
{
    soundSlotStop(index);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu071", bgmPlayTrack);

INCLUDE_ASM("asm/nonmatchings/ov01/tu071", func_00AA91B8);

/* Plays the battle action stream of the unit's key in mode (dataBattleActionPathGet) on slot 1. */
void streamPlayByKey(Ov01Object *unit, int mode)
{
    char *path;

    path = dataBattleActionPathGet(unit->stats->key, mode);
    if (path != 0) {
        soundSlotReopen(1);
        func_001EDC60(1);
        streamSlotSetFile(1, path);
    }
}
