#ifndef INCLUDE_OV01_TU043_H
#define INCLUDE_OV01_TU043_H

/* Functions of src/ov01/tu043.c that other files call. */

/* Data box access: the quantity of item id in category 1 to 4 (dataBoxPtrGet answers NULL for any other category). */
u16 *dataBoxPtrGet(int category);
int dataBoxChk(int category, int id);
int dataBoxInc(int category, int id);
int dataBoxDec(int category, int id);
int dataBoxChkCategory1(int id);
void dataBoxDecCategory1(int id);
void dataBoxClearAll(void);

#endif /* INCLUDE_OV01_TU043_H */
