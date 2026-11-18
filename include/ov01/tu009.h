#ifndef INCLUDE_OV01_TU009_H
#define INCLUDE_OV01_TU009_H

/* Functions of src/ov01/tu009.c that other files call. */

/* Header of one heap block (0x40 bytes, the payload follows): word 0 is the flags word (bit 0 set = in use),
   word 1 the payload size. */
typedef struct Ov01HeapBlock {
    unsigned int flags;
    int size;
} Ov01HeapBlock;

/* Overlay heap: ov01HeapAlloc returns a payload, ov01HeapFree takes a payload, ov01HeapBlockOf maps a payload to
   the address of its block header. */
void *ov01HeapAlloc(int size);
void ov01HeapFree(void *payload);
Ov01HeapBlock *ov01HeapBlockOf(void *payload);
void ov01HeapInit(void);

/* CD reads: ov01CdReadFileSync waits for the read; ov01CdReadFileAsync starts one with ov01CdReadCallback and counts it as pending. */
int ov01CdPendingReads(void);
void ov01CdPendingReset(void);
unsigned long long ov01CdReadFileSync(const char *name, void *buffer);
unsigned long long ov01CdReadFileAsync(const char *name, void *buffer);
unsigned int ov01LookupNonHeap(int key);

#endif /* INCLUDE_OV01_TU009_H */
