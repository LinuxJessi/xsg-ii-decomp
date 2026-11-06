#ifndef INCLUDE_MAIN_XGL_CD_H
#define INCLUDE_MAIN_XGL_CD_H

/* Functions of src/main/xgl_cd.c that other files call. */

void xglCdArcCheck(void);
int xglCdReadFile(const char *name, void *buffer, int mode, int flags);
int xglCdServiceIfPending(void);
void xglCdSetReadTag(int value);

#endif /* INCLUDE_MAIN_XGL_CD_H */
