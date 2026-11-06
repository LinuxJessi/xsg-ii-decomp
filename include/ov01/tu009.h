#ifndef INCLUDE_OV01_TU009_H
#define INCLUDE_OV01_TU009_H

/* Functions of src/ov01/tu009.c that other files call. */

int ov01CdPendingReads(void);
void ov01CdPendingReset(void);
unsigned long long ov01CdReadFileSync(const char *name, void *buffer);
unsigned int ov01LookupNonHeap(int key);

#endif /* INCLUDE_OV01_TU009_H */
