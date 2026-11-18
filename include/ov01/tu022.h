#ifndef INCLUDE_OV01_TU022_H
#define INCLUDE_OV01_TU022_H

/* Functions of src/ov01/tu022.c that other files call. */

struct DataObject;

/* Data object and state reset: dataObjRemove releases the data object, dataStateReset is the state step of the reset chain (tu037). */
void dataObjDraw(struct DataObject *obj);
void dataObjUpdate(struct DataObject *obj);
void dataObjRemove(void);
void dataStateReset(void);

/* Counters and virtual pad. */
void dataCountersClear(void);
void dataVPadModeSet(int mode);
void dataVPadReset(void);
void dataVPadSet(int virtualPad);
void dataObjCreateAtOrigin(void);

#endif /* INCLUDE_OV01_TU022_H */
