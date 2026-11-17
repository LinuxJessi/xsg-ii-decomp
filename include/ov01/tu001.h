#ifndef INCLUDE_OV01_TU001_H
#define INCLUDE_OV01_TU001_H

/* Functions of src/ov01/tu001.c that other files call. */

/* Claims a task record of ov01TaskTable (the claim bit of tu000.h). Returned as void *: the fade, screen and message
 * tasks each read the claimed record as their own struct (FadeTask, ScreenFlashTask, ScreenFillTask, ...). */
void *ov01TaskClaim(void);
void ov01TaskClearAll(void);
void ov01TaskRelease(void *task);
void ov01TaskRunDraw(void);
void ov01TaskRunUpdate(void);

#endif /* INCLUDE_OV01_TU001_H */
