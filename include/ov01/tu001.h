#ifndef INCLUDE_OV01_TU001_H
#define INCLUDE_OV01_TU001_H

/* Functions of src/ov01/tu001.c that other files call. */

/* The task record of ov01TaskTable as the screen tasks (tu067 flash, tu068 fill) read it: +0x04 update, +0x08 draw,
 * +0x0C framesLeft (the flash only). The update slot is typed with the record, so the flash tick takes a ScreenTask. */
typedef struct ScreenTask ScreenTask;
struct ScreenTask {
    unsigned char unmodeled_00[4];
    void (*update)(ScreenTask *task);           /* +0x04 */
    void (*draw)(void);                         /* +0x08 */
    int framesLeft;                             /* +0x0C */
};

/* Claims a task record of ov01TaskTable (the claim bit of tu000.h). Returned as void *: the fade, screen and message
 * tasks each read the claimed record as their own struct (FadeTask, ScreenTask, ...). */
void *ov01TaskClaim(void);
void ov01TaskClearAll(void);
void ov01TaskRelease(void *task);
void ov01TaskRunDraw(void);
void ov01TaskRunUpdate(void);

#endif /* INCLUDE_OV01_TU001_H */
