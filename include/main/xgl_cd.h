#ifndef INCLUDE_MAIN_XGL_CD_H
#define INCLUDE_MAIN_XGL_CD_H

/* Functions of src/main/xgl_cd.c that other files call. */

/* xglCdGetFilePos and xglCdCheckFile fill a 0x30-byte file-position record;
 * only its leading lbn word is evidenced by the callers in xgl_cd.c. */
typedef struct CdFilePosition {
    unsigned int lbn;
    int stored_bytes;
    unsigned char unmodeled_08[0x1c];
    unsigned char source;
    unsigned char unmodeled_25;
    unsigned char packed;
    unsigned char unmodeled_27;
    int descriptor;
    int size_bytes;
} CdFilePosition;

/* Probes path and fills a CdFilePosition at record (lsn at +0, size at +4,
 * see tu022.c); -1 when the file is missing, 0 otherwise. The callers pass
 * work buffers they reuse for the record. */
int xglCdCheckFile(const char *path, void *record);
void xglCdArcCheck(void);
int xglCdReadFile(const char *name, void *buffer, int mode, int flags);
int xglCdServiceIfPending(void);
void xglCdSetReadTag(int value);

#endif /* INCLUDE_MAIN_XGL_CD_H */
