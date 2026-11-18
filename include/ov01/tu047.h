#ifndef INCLUDE_OV01_TU047_H
#define INCLUDE_OV01_TU047_H

/* Functions of src/ov01/tu047.c that other files call. */

struct LightBlocks;
union DoublewordVector4;

struct LightBlocks *lightBlockGet(void);
int bgmIndexGet(void);
/* Hands out the runtime position and direction vectors. */
void runtimeVecPtrGet(union DoublewordVector4 **pos, union DoublewordVector4 **dir);

#endif /* INCLUDE_OV01_TU047_H */
