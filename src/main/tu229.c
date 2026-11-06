#include "common.h"
#include "main/tu229.h"
#include "main/xgl_cd.h"

/* (value * 30) - 30; the unit is not read yet (caller interpAdvance, tu227). */
int interpCursorFrame(int value) {
    return (value * 30) - 30;
}

/* value - 80; the constant's meaning is not read yet (caller func_0027E048, tu225). */
int func_0027E7F8(int value) {
    return value - 80;
}

void talkLoadOmiyaScript(void);

/* Entry that loads the talk script; delegates to talkLoadOmiyaScript. */
void talkStartScriptLoad(void) {
    talkLoadOmiyaScript();
}

void talkCloseScript(void);
extern int talkActive;

/* Closes the loaded script, then clears the talk-active flag talkActive. */
void talkEndScript(void) {
    talkCloseScript();
    talkActive = 0;
}

int func_001E0768(char *name, void **data, unsigned int *size, void (*callback)(int index, void *buffer, unsigned int size, int value));
extern void *talkScriptHandle;
extern char talkOmiyaScriptPath[];

/*
 * Starts loading "omiya\talk01.xtx" (talkOmiyaScriptPath) into the script handle
 * talkScriptHandle, then waits while the CD service reports work pending
 * (xglCdServiceIfPending, xglCdServiceIfPending in tu011).
 */
void talkLoadOmiyaScript(void) {
    func_001E0768(talkOmiyaScriptPath, &talkScriptHandle, 0, 0);
    while (xglCdServiceIfPending() != 0) {
    }
}

int func_001DF818(void *buffer);

/* Releases the script handle talkScriptHandle and clears it. */
void talkCloseScript(void) {
    func_001DF818(talkScriptHandle);
    talkScriptHandle = 0;
}

/* Clears the talk-active flag talkActive. */
void talkDeactivate(void) {
    talkActive = 0;
}

extern int talkPendingValue;

/* Stores a pending value in talkPendingValue. */
void talkSetPending(int value) {
    talkPendingValue = value;
}

/* Returns the pending value in talkPendingValue and clears it. */
int talkTakePending(void) {
    int value;

    value = talkPendingValue;
    talkPendingValue = 0;
    return value;
}

INCLUDE_ASM("asm/main/nonmatchings/tu229", func_0027E8B8);

INCLUDE_ASM("asm/main/nonmatchings/tu229", func_0027E960);

INCLUDE_ASM("asm/main/nonmatchings/tu229", func_0027EA80);

INCLUDE_ASM("asm/main/nonmatchings/tu229", func_0027EB40);

INCLUDE_ASM("asm/main/nonmatchings/tu229", func_0027EC18);

/* Sets the talk-active flag talkActive. */
void talkSetActive(void) {
    talkActive = 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu229", func_0027ED28);

INCLUDE_ASM("asm/main/nonmatchings/tu229", func_0027EE70);
