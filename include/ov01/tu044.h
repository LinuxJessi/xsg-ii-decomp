#ifndef INCLUDE_OV01_TU044_H
#define INCLUDE_OV01_TU044_H

/* Functions of src/ov01/tu044.c that other files call. */

struct SlotChain;

void sbChainSetValue(struct SlotChain *chain, int value);
void sbResetNativeCounts(void);
void sbThreadsRelease(int key);

#endif /* INCLUDE_OV01_TU044_H */
