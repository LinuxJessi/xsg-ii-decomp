#include "common.h"
#include "shared.h"
#include "main/tu048.h"


/* Script natives: argument count, n-th argument, nonzero when argument n is omitted, wrong-count error. */
int func_A9F0A8(int call);
void func_A9CC70(int call, int error_code);
int func_A9F0C0(int call, int index);
int func_A9F1C0(int call, int index);
/* Debug pad state defined in src/main/tu088.c. */
extern int padDebugSector;
extern int padDebugCount;
/* The message panel functions, defined in src/main/tu148.c. */
void msgPanelOpen(void);
void msgPanelClear(void);

/* Natives used by runtime_001F2DF8 (defined elsewhere) and the library compare helpers it calls. */
int func_A9B8D8(int target, int zero);
int func_A9B7D8(int target, int zero);
int func_A9B950(int target, int zero);
int func_A9B9C8(int target, int zero);
int func_A9C7C8(int target);
void func_A9C7E8(int target);
int func_A9C7A8(int target);
int func_A9C828(int target);
void func_A9C848(int target);
int func_A9C808(int target);
int func_A9C8E8(int target);
void func_A9C908(int target);
int func_A9C8C8(int target);
int func_A9C888(int target);
void func_A9C8A8(int target);
int func_A9C868(int target);
void func_A8F3C0(int target);
void func_A8B7E0(int arg, int target);
void func_A8B840(int arg, int target);
void func_A8B720(int target, int arg);
void selectionBlockFill(void);
extern char *strcpy(char *destination, const char *source);
extern char *strcat(char *dest, const char *src);
void cursorBufferDeactivateCurrent(void);
void cursorBufferPush(char *text);
/* Text default and digit buffer: tu068's own data at 0x0069D220 and the rodata run at 0x00689700. */
extern char D_0069D220[];
extern char D_003E7270[];

/* EUC-JP text "00" (two digits written in place) then the rest of the 32-byte buffer, zero-filled. */
const char D_00689700[32] = "\xa4\xf2\x30\x30\xb8\xc4\xbc\xea\xa4\xcb\xc6\xfe\xa4\xec\xa4\xbf\xa1\xaa";

int runtime_001F2DF8(int call)
{
    int kind;
    int target;
    int count;
    int flag;
    int steps;
    char *text;
    char *digits;
    char *dest;
    char buf[256];

    steps = 0;
    digits = D_00689700;
    text = D_0069D220;
    if (func_A9F0A8(call) < 3) {
        func_A9CC70(call, 2);
        return 0;
    }
    kind = func_A9F0C0(call, 1);
    target = func_A9F0C0(call, 2);
    count = func_A9F1C0(call, 3) == 0 ? func_A9F0C0(call, 3) : 0;
    flag = func_A9F1C0(call, 4) == 0 ? (signed char) func_A9F0C0(call, 4) : 0;
    switch (kind) {
    case 1:
        text = func_A9B8D8(target, 0);
        if (count > 0) {
            for (steps = 0; steps < count; steps++) {
                func_A9C7C8(target);
            }
        } else {
            for (steps = 0; steps < __builtin_fabs(count); steps++) {
                func_A9C7E8(target);
            }
        }
        steps = func_A9C7A8(target);
        break;
    case 2:
        text = func_A9B7D8(target, 0);
        if (count > 0) {
            for (steps = 0; steps < count; steps++) {
                func_A9C828(target);
            }
        } else {
            for (steps = 0; steps < __builtin_fabs(count); steps++) {
                func_A9C848(target);
            }
        }
        steps = func_A9C808(target);
        break;
    case 3:
        text = func_A9B950(target, 0);
        if (count > 0) {
            for (steps = 0; steps < count; steps++) {
                func_A9C8E8(target);
            }
        } else {
            for (steps = 0; steps < __builtin_fabs(count); steps++) {
                func_A9C908(target);
            }
        }
        steps = func_A9C8C8(target);
        break;
    case 4:
        text = func_A9B9C8(target, 0);
        if (count > 0) {
            for (steps = 0; steps < count; steps++) {
                func_A9C888(target);
            }
        } else {
            for (steps = 0; steps < __builtin_fabs(count); steps++) {
                func_A9C8A8(target);
            }
        }
        selectionBlockFill();
        steps = func_A9C868(target);
        break;
    case 5:
        func_A8F3C0(target);
        break;
    case 6:
        func_A8B7E0(3, target);
        break;
    case 7:
        func_A8B840(2, target);
        break;
    case 8:
        func_A8B720(target, 63);
        break;
    }
    if (flag == 1) {
        dest = D_003E7270;
        digits[2] = count / 10 + '0';
        digits[3] = count % 10 + '0';
        strcpy(buf, text);
        strcat(buf, digits);
        strcpy(dest, buf);
        cursorBufferDeactivateCurrent();
        cursorBufferPush(dest);
    }
    return steps;
}

/* Native "TimeAttack": argument 1 is the kind, argument 2 the value (-1 when omitted).  Kind 0 opens
   the message panel and stores the kind and the value; kind 1 clears the panel and stores the kind
   with value 0; kind 2 stores the kind; kind 3 stores 0 as the kind; kind 4 returns -1 when the stored
   kind is 1, else the stored value.  Kinds 0 to 3 return the value, -1 or the stored value as the
   case reads; any other kind returns -1. */
/* Native "TimeAttack": argument 1 is the kind, argument 2 the value (-1 when omitted).  Kind 0 opens
   the message panel and stores the kind and the value; kind 1 clears the panel and stores the kind
   with value 0; kind 2 stores the kind; kind 3 stores 0 as the kind; kind 4 returns -1 when the stored
   kind is 1, else the stored value.  Kinds 0 to 3 return the value, -1 or the stored value as the
   case reads; any other kind returns -1. */
int runtime_001F32E0(int call)
{
    int kind;
    int value;
    int result;

    result = -1;
    if (func_A9F0A8(call) <= 0) {
        func_A9CC70(call, 2);
        return 0;
    }
    kind = func_A9F0C0(call, 1);
    if (func_A9F1C0(call, 2) == 0) {
        value = func_A9F0C0(call, 2);
    } else {
        value = -1;
    }
    switch (kind) {
    case 0:
        msgPanelOpen();
        gameWork.panelMode = kind;
        gameWork.panelValue = value;
        result = value;
        break;
    case 1:
        result = -1;
        msgPanelClear();
        gameWork.panelMode = kind;
        gameWork.panelValue = 0;
        break;
    case 2:
        gameWork.panelMode = kind;
        result = gameWork.panelValue;
        break;
    case 3:
        gameWork.panelMode = 0;
        result = gameWork.panelValue;
        break;
    case 4:
        if (gameWork.panelMode == 1) {
            result = -1;
        } else {
            result = gameWork.panelValue;
        }
        break;
    }
    return result;
}

/* Native "GetAnalogR": argument 1 selects the mode.  Mode 0 clears padDebugSector and
   padDebugCount and returns 0; mode 4 returns padDebugCount; any other mode returns -1. */
int runtime_001F3420(int call)
{
    int result;

    result = -1;
    if (func_A9F0A8(call) <= 0) {
        func_A9CC70(call, 2);
        return 0;
    }
    switch (func_A9F0C0(call, 1)) {
    case 0:
        padDebugSector = 0;
        padDebugCount = 0;
        result = 0;
        break;
    case 4:
        result = padDebugCount;
        break;
    }
    return result;
}

char D_0069D220[8] = { 0 };
char D_0069D228[8] = "bgm";
char D_0069D230[8] = "nextBgm";
char D_0069D238[8] = "bgmFade";
char D_0069D240[8] = "bgmse";
char D_0069D248[8] = "seVol";
char D_0069D250[8] = "sePos";
char D_0069D258[8] = "se";
char D_0069D260[8] = "adxplay";
char D_0069D268[8] = "adxload";
char D_0069D270[8] = "adx";
char D_0069D278[8] = "mpeg2";
char D_0069D280[8] = "System";
char D_0069D288[8] = "setItai";
char D_0069D290[8] = "setFog";
char D_0069D298[8] = "getFlag";
char D_0069D2A0[8] = "setFlag";
char D_0069D2A8[8] = "Fade";
char D_0069D2B0[8] = "jumpCF";
char D_0069D2B8[8] = "message";
char D_0069D2C0[8] = "Item";
