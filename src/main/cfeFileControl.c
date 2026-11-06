#include "common.h"

typedef char *va_list;

#define va_start(ap, last) ((ap) = (va_list)__builtin_next_arg(last) - (8 - __builtin_args_info(2)) * 8)
#define va_end(ap) ((void)0)

int sceClose(int fd);
void func_002AA7F0(int, int);
int vsprintf(char *buffer, const char *format, va_list args);
extern int cfeFileTemp;

/* Closes a descriptor: a thin wrapper over sceClose. */
void cfeFileClose(int fd) {
    sceClose(fd);
}

/* Returns the pFileTemp buffer (cfeFileTemp). */
int cfeFileTempGet(void) {
    return cfeFileTemp;
}

/* Sets pFileTemp to the buffer in a0, clears a1 bytes of it and resets the log indent to 0. */
INCLUDE_ASM("asm/main/nonmatchings/cfeFileControl", cfeFileTempInit);

extern int cfeFileLogIndent;

/* Adds `delta` to the log indent level (cfeFileLogIndent), never below 0. */
void cfeFileLogIndentAdd(int delta) {
    int total;

    total = cfeFileLogIndent + delta;
    cfeFileLogIndent = total;
    if (total < 0) {
        cfeFileLogIndent = 0;
    }
}

extern int cfeFileLogIndentStr;

/* Appends `count` copies of the indent string cfeFileLogIndentStr to pFileTemp. */
void cfeFileLogWriteIndent(int count) {
    int remaining;

    if (count > 0) {
        remaining = count;
        do {
            remaining -= 1;
            func_002AA7F0(cfeFileTemp, cfeFileLogIndentStr);
        } while (remaining != 0);
    }
}

/* Log line: formats into the scratchpad at 0x70000000, appends the indent, then the text to pFileTemp.
   Does nothing when pFileTemp is null. */
void cfeFileLogf(const char *format, ...) {
    va_list args;

    va_start(args, format);
    if (cfeFileTemp != 0) {
        vsprintf((char *) 0x70000000, format, args);
        cfeFileLogWriteIndent(cfeFileLogIndent);
        func_002AA7F0(cfeFileTemp, 0x70000000);
    }
    va_end(args);
}

/* Writes the NUL-terminated pFileTemp text to the file named by a0 (assert: pFileTemp is set). */
INCLUDE_ASM("asm/main/nonmatchings/cfeFileControl", cfeFileTempWrite);

/* Reads the file named by a0 into pFileTemp and returns its size
   (asserts: pFileTemp is set, size < MAX_OF_FILESIZE). */
INCLUDE_ASM("asm/main/nonmatchings/cfeFileControl", cfeFileTempRead);

extern const char D_0068D108[];

/* Opens a block in the log: writes "{" at the current indent, then indents one level. */
void cfeFileLogBlockBegin(void) {
    cfeFileLogf(D_0068D108);
    cfeFileLogIndentAdd(1);
}

extern const char D_0068D110[];

/* Closes a block in the log: unindents one level, then writes "}" at that indent. */
void cfeFileLogBlockEnd(void) {
    cfeFileLogIndentAdd(-1);
    cfeFileLogf(D_0068D110);
}
