#include "common.h"

/* Empty callback: nothing to do. */
void func_001EAB08(void) {

}

extern int D_002BD1E8[];

int func_001EAB10(int index) {
    return D_002BD1E8[index];
}

int printf(const char *, ...);
extern const char D_00688900[];

/* Logs a callback id; always returns -1. */
int DebugCallbackLog(int callback_id) {
    printf(D_00688900, callback_id);
    return -1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu061", func_001EAB50);

INCLUDE_ASM("asm/main/nonmatchings/tu061", func_001EAC18);

INCLUDE_ASM("asm/main/nonmatchings/tu061", McFileSectionsReport);

INCLUDE_ASM("asm/main/nonmatchings/tu061", MemcardTest);

INCLUDE_ASM("asm/main/nonmatchings/tu061", CdSlotTableView);

INCLUDE_ASM("asm/main/nonmatchings/tu061", func_001EC8E8);

INCLUDE_ASM("asm/main/nonmatchings/tu061", CdSlotTest);

INCLUDE_ASM("asm/main/nonmatchings/tu061", MovieListTest);
