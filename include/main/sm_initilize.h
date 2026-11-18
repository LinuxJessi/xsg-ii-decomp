#ifndef INCLUDE_MAIN_SM_INITILIZE_H
#define INCLUDE_MAIN_SM_INITILIZE_H

/* Functions of src/main/sm_initilize.c that other files call. */

/* An allocation of size bytes (rounded up to 0x800) from the large heap, 0 when it is full. */
void *smAlloc(int size);
/* A small-heap allocation of size bytes (rounded up to 0x40), 0 when the heap is full. */
void *smAllocSmall(int size);
void smFreeHandle(void *handle);

#endif /* INCLUDE_MAIN_SM_INITILIZE_H */
