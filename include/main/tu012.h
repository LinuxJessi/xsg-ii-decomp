#ifndef INCLUDE_MAIN_TU012_H
#define INCLUDE_MAIN_TU012_H

/* Functions of src/main/tu012.c that other files call. */

/* One request of the xglMc queue: the file number, its buffer and size, and
 * a callback the list and old-data scans call per entry (with the result,
 * and the entry bytes for the old-data check).  The callback is unprototyped
 * because the callers in tu060.c differ: DebugCallbackLog takes the result
 * only, func_001EAB50 the result and the entry. */
typedef struct McRequestParam {
    int file;
    void *data;
    int size;
    int (*callback)();
} McRequestParam;

void xglMcReset(void);
char * xglMcSetFullPath(int card, int selector);
int xglMcMain(void);
int xglMcRequest(int port, int command, McRequestParam *param, int *result);

#endif /* INCLUDE_MAIN_TU012_H */
