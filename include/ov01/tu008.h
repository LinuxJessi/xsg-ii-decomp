#ifndef INCLUDE_OV01_TU008_H
#define INCLUDE_OV01_TU008_H

/* Functions of src/ov01/tu008.c that other files call. */

struct DataSlot;
struct DataStreamEntry;

void dataBatDatLoad(void);
void dataBattleSeLoad(void);
void dataBattleSeRelease(void);
void dataFaceFileLoad(void);
void dataSlotUnload(struct DataSlot *slot);
int dataStatBufGet(void);
void dataStatFileLoad(void);
/* Streams and effect slots: dataStreamOpen returns the stream record, unitEffectSlotAcquire takes an effect key. */
struct DataStreamEntry *dataStreamOpen(int count, int small_buffer);
int unitEffectSlotAcquire(int key);

#endif /* INCLUDE_OV01_TU008_H */
