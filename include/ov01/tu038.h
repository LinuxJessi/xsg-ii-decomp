#ifndef INCLUDE_OV01_TU038_H
#define INCLUDE_OV01_TU038_H

/* Functions of src/ov01/tu038.c that other files call. */

struct DataSlot;

int ov01LoadIsDone(void);
void ov01LoadRequest(struct DataSlot *owner);

#endif /* INCLUDE_OV01_TU038_H */
