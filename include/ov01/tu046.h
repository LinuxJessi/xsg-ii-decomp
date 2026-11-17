#ifndef INCLUDE_OV01_TU046_H
#define INCLUDE_OV01_TU046_H

/* Functions of src/ov01/tu046.c that other files call: the argument readers of the script natives
 * (each takes the running thread, see include/ov01/tu044.h). */

struct SbThread;

int VM_PLUGIN_CNT_GET(struct SbThread *thread);
int vmArgGet(struct SbThread *thread, int argIndex);
int vmArgGetHigh(struct SbThread *thread, int argIndex);
int vmArgTagIsZero(struct SbThread *thread, int argIndex);
int vmKeyedEntryGet(struct SbThread *thread, int key);
void *vmEntryGet(struct SbThread *thread, int index);
int vmSubEntryGet(struct SbThread *thread, int index, int subIndex);
void func_00A9F2E0(struct SbThread *thread, int pc);

int monsTblGet(void);
int posDataGet(void);
int cameraSettingGet(void);

#endif /* INCLUDE_OV01_TU046_H */
