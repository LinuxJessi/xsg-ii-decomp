#include "common.h"

int LanOpen(void);
int func_0029FD90(void *, int, int, int);
int func_0029FFB0(void *, int, int, const void *, int, int *, int);
int LanReceive(int, void *, int, void (*)(void));
unsigned int strlen(const char *);
int printf(const char *, ...);
char *strcpy(char *, const char *);
void xglFontDebugPrintf(int, int, const char *, ...);
extern void (*netStatusHandler)(void);
extern int netWork;
extern const char D_0068E448[];
extern const char D_0068E458[];
extern const char D_0068E480[];
extern const char D_0068E490[];
extern unsigned char netCloseCommand[];
extern unsigned char netGetCommand[];
extern unsigned char netRecvCommand[];
extern unsigned char netContext[];
extern const char netHostName[];
extern int netInitState;
extern int netHostConfigured;
extern int D_0069E390;

/* Shows a step label through the debug font, then runs the status handler (netStatusHandler) when one is set. */
void LanReportStatus(const char *message) {
    if (netStatusHandler != 0) {
        xglFontDebugPrintf(16, 16, message);
        netStatusHandler();
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu027", LanOpen);

/* Closes the handle with a one second timeout; prints the error when the close fails. */
void LanCloseHandle(int handle) {
    int status;

    status = func_0029FD90(netContext, netWork, handle, 1000);
    if (status < 0) {
        printf(D_0068E448, status);
    }
}

/* Sends the CLOSE verb, then closes the handle. */
void LanClose(int handle) {
    int result;

    LanReportStatus(D_0068E458);
    result = 0;
    func_0029FFB0(netContext, netWork, handle, netCloseCommand, 6, &result, -1);
    LanCloseHandle(handle);
}

/* Receives the requested bytes in chunks of at most 0x4000, calling the optional hook after each chunk. */
INCLUDE_ASM("asm/main/nonmatchings/tu027", LanReceive);

INCLUDE_ASM("asm/main/nonmatchings/tu027", UsbRead_001A6CA0);

/* GET: sends the path, reads the file size, then receives the file into buffer. */
int UsbRead_001A6D80(const char *path, void *buffer) {
    int result;
    int size;
    int handle;

    size = -1;
    handle = LanOpen();
    if (handle > 0) {
        LanReportStatus(D_0068E480);
        result = 0;
        func_0029FFB0(netContext, netWork, handle, netGetCommand, 4, &result, -1);
        result = 0;
        func_0029FFB0(netContext, netWork, handle, path, (int) strlen(path) + 1, &result, -1);
        LanReportStatus(D_0068E490);
        LanReceive(handle, &size, 4, 0);
        LanReceive(handle, buffer, size, netStatusHandler);
        LanClose(handle);
    }
    return size;
}

/* PUT: sends the path, the size and the body in chunks of at most 0x4000. */
INCLUDE_ASM("asm/main/nonmatchings/tu027", UsbWrite);

/* SEND: the PUT protocol with the SEND verb. */
INCLUDE_ASM("asm/main/nonmatchings/tu027", UsbSend);

/* RECV: sends the path, then receives at most requested bytes into buffer and discards the rest. */
int UsbRecv(const char *path, void *buffer, int requested) {
    int result;
    int size;
    int discard;
    int handle;

    handle = LanOpen();
    if (handle > 0) {
        result = 0;
        func_0029FFB0(netContext, netWork, handle, netRecvCommand, 5, &result, -1);
        result = 0;
        func_0029FFB0(netContext, netWork, handle, path, (int) strlen(path) + 1, &result, -1);
        LanReceive(handle, &size, 4, 0);
        discard = size - requested;
        if (requested >= size) {
            discard = 0;
            requested = size;
        }
        if (size > 0) {
            LanReceive(handle, buffer, requested, 0);
            if (discard > 0) {
                LanReceive(handle, 0, discard, 0);
            }
        }
        LanClose(handle);
    }
    return size;
}

/* Returns the initialization state: zero until reg_handler has completed. */
int UsbGetInitState(void) {
    return netInitState;
}

INCLUDE_ASM("asm/main/nonmatchings/tu027", func_001A7278);

INCLUDE_ASM("asm/main/nonmatchings/tu027", UsbSetHost);

/* Returns the configured flag; when it is set, copies the host name and its value out. */
int UsbGetHost(char *name, int *value) {
    if (netHostConfigured != 0) {
        strcpy(name, netHostName);
        *value = D_0069E390;
    } else {
        name[0] = '\0';
        *value = 0;
    }
    return netHostConfigured;
}

/* Stores the status handler, loads the network IRX modules and starts the stack once. */
INCLUDE_ASM("asm/main/nonmatchings/tu027", reg_handler);
