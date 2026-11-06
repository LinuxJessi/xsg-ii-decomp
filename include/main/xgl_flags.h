#ifndef INCLUDE_MAIN_XGL_FLAGS_H
#define INCLUDE_MAIN_XGL_FLAGS_H

/* Functions of src/main/xgl_flags.c that other files call. */

void xglFlagsClear(void);
int xglFlagsSet(int bit_offset, int bit_count, int value);
int xglFlagsSet1(int bit_offset, int value);

#endif /* INCLUDE_MAIN_XGL_FLAGS_H */
