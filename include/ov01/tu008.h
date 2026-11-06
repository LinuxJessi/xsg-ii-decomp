#ifndef INCLUDE_OV01_TU008_H
#define INCLUDE_OV01_TU008_H

/* Functions of src/ov01/tu008.c that other files call. */

struct DataSlot;

void dataBatDatLoad(void);
void dataBattleSeLoad(void);
void dataBattleSeRelease(void);
void dataFaceFileLoad(void);
void dataSlotUnload(struct DataSlot *slot);
int dataStatBufGet(void);
void dataStatFileLoad(void);

#endif /* INCLUDE_OV01_TU008_H */
