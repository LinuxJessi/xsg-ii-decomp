#ifndef INCLUDE_OV01_TU071_H
#define INCLUDE_OV01_TU071_H

/* Functions of src/ov01/tu071.c that other files call. */

#include "ov01/tu012.h"

void seObjectVoiceStartDefault(Ov01Object *object, int code);
void streamSlotSetFile(int slot, const char *file);
void streamSlotStop(int index);
void bgmPlayTrack(int track);
void streamPlayByKey(struct Ov01Object *unit, int mode);
/* Starts the stream of the unit's action (the technique's gateFlag picks it). */
void unitPlayActionSound(struct Ov01Object *unit, int action);

#endif /* INCLUDE_OV01_TU071_H */
