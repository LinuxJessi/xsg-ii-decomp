#ifndef INCLUDE_OV01_TU044_H
#define INCLUDE_OV01_TU044_H

/* Functions of src/ov01/tu044.c that other files call. */

struct SbImageHeader;
struct SbThread;

void sbChainSetValue(struct SbThread *thread, int key);
void sbResetNativeCounts(void);
void sbThreadsRelease(struct SbImageHeader *key);

#endif /* INCLUDE_OV01_TU044_H */
