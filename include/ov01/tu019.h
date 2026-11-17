#ifndef INCLUDE_OV01_TU019_H
#define INCLUDE_OV01_TU019_H

/* Functions of src/ov01/tu019.c that other files call. */

struct MonsSlot;

/* Makes a MonsSlot record (monsSlotNew) and inserts it in the active table; 0 when no record is free. */
struct MonsSlot *monsSlotCreate(void);

#endif /* INCLUDE_OV01_TU019_H */
