#ifndef INCLUDE_MAIN_TU005_H
#define INCLUDE_MAIN_TU005_H

/* Functions of src/main/tu005.c that other files call: the host-file debug stubs. */

int xglDebFileSize(const char *name);
int xglDebFileRead(const char *name, void *buffer);
/* cfeFileTempWrite passes the byte count of the text as size. */
int xglDebFileWrite(const char *name, const void *buffer, int size);

#endif /* INCLUDE_MAIN_TU005_H */
