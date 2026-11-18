#ifndef INCLUDE_OV01_TU049_H
#define INCLUDE_OV01_TU049_H

/* Functions of src/ov01/tu049.c that other files call. */

int menuPanelRowAlloc(void);
int menuPanelRowFree(int y);
void menuTexFlushTaskStart(void);
void menuPanelRowsReset(void);

#endif /* INCLUDE_OV01_TU049_H */
