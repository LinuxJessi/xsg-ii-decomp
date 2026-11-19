#ifndef INCLUDE_OV01_TU076_H
#define INCLUDE_OV01_TU076_H

/* Functions of src/ov01/tu076.c that other files call; struct HermitePath is defined in tu076.c. */

struct HermitePath;

/* Clears the 0xB0-byte record at record and stores value in its first word. */
void ov01RecordInit(int *record, int value);

/* Key choice of a Hermite path: the previous, start, end and next points of key index. */
void MMathCalcHermiteKeyPrm(struct HermitePath *path, int index);

/* Hermite sample at key record `index` of the table at base. */
void MMathCalcHermiteKey(float *destination, char *base, int index, f32 parameter);

#endif /* INCLUDE_OV01_TU076_H */
