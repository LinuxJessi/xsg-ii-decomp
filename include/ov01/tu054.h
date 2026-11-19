#ifndef INCLUDE_OV01_TU054_H
#define INCLUDE_OV01_TU054_H

/* Functions of src/ov01/tu054.c that other files call. */

void arrowRemove(void);

struct Ov01Object;

/* Makes the object that shows amount at the unit (tu012 passes HP and EP magnitudes with their kind). */
void menuPopNumCreate(struct Ov01Object *object, int amount, int kind, int sign);

#endif /* INCLUDE_OV01_TU054_H */
