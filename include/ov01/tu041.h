#ifndef INCLUDE_OV01_TU041_H
#define INCLUDE_OV01_TU041_H

/* Functions of src/ov01/tu041.c that other files call. */

struct EffectPosRecord;

void effectPosTableSet(struct EffectPosRecord *table);
int * effectPosGet(int index);

#endif /* INCLUDE_OV01_TU041_H */
