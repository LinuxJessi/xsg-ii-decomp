#include "common.h"
#include "main/tu005.h"

extern const char D_0068D5B8[];
int scriptReadFile(int *slot, char *fileName);

/* Builds "scNNNN.sb" from number (N = thousands, hundreds, tens, units digit) and loads it.
   __builtin_memcpy: plain memcpy() compiles to a jal + explicit call, not the inlined
   ldl/ldr/sdl/sdr copy the original uses for this fixed 10-byte unaligned template. */
int scriptLoadByNumber(int *slot, int number) {
    char fileName[10];

    __builtin_memcpy(fileName, D_0068D5B8, sizeof(fileName));
    fileName[2] = ((number / 1000) % 10) + '0';
    fileName[3] = ((number / 100) % 10) + '0';
    fileName[4] = ((number / 10) % 10) + '0';
    fileName[5] = (number % 10) + '0';
    return scriptReadFile(slot, fileName);
}

int func_002A9350(int size);
void func_002AA7F0(char *destination, char *source);
int sbLinkImage(int buffer);
extern const char D_0068D5C8[];
char *strcpy(char *, const char *);

/* Allocates a buffer in *slot, waits for it, then loads fileName from host0:.../cf/. */
int scriptReadFile(int *slot, char *fileName) {
    char path[0x100];
    const char *hostPath;
    int status;

    hostPath = D_0068D5C8;
    *slot = func_002A9350(100);
    while (*slot == 0) {
    }
    strcpy(path, hostPath);
    func_002AA7F0(path, fileName);
    xglDebFileRead(path, (void *) *slot);
    status = sbLinkImage(*slot);
    if (status != 0) {
        return status;
    }
    for (;;) {
    }
}

void func_002A93A8(int value);

/* Frees the buffer in *slot (allocated by scriptReadFile) and clears the slot. */
void scriptFreeBuffer(int *slot) {
    func_002A93A8(*slot);
    *slot = 0;
}
