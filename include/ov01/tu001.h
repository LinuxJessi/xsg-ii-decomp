#ifndef INCLUDE_OV01_TU001_H
#define INCLUDE_OV01_TU001_H

/* Functions of src/ov01/tu001.c that other files call. */

void ov01TaskClearAll(void);
void ov01TaskRelease(void *task);
void ov01TaskRunDraw(void);
void ov01TaskRunUpdate(void);

#endif /* INCLUDE_OV01_TU001_H */
