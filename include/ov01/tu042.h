#ifndef INCLUDE_OV01_TU042_H
#define INCLUDE_OV01_TU042_H

/* Functions of src/ov01/tu042.c that other files call. */

int msgIsIdle(void);
void partyDataSetSlot(int id, int slot, int value);
void * tableKeyRowGet(int key);

#endif /* INCLUDE_OV01_TU042_H */
