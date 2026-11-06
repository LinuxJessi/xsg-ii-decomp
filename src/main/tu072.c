#include "common.h"
#include "main/tu072.h"

INCLUDE_ASM("asm/main/nonmatchings/tu072", runtime_001F4E20);

extern unsigned char runtimeNativeTable[];
extern unsigned char runtimeModuleName[];
void func_A9CDB0(void *, void *);

/* Registers the "runtime" script module: runtimeModuleName holds its name string and
   runtimeNativeTable its table of name/function pointer pairs (setNumber, setLocater,
   setElevator, ...).  Called from scriptRegisterModules. */
void runtimeRegisterNatives(void) {
    func_A9CDB0(runtimeModuleName, runtimeNativeTable);
}

INCLUDE_ASM("asm/main/nonmatchings/tu072", enepc_001F4F28);

INCLUDE_ASM("asm/main/nonmatchings/tu072", enepc_001F5360);

/* Shared by the enepc setParams natives.  For uwamonoRecords[index] whose kind byte (+2)
   is 1 or 3, every argument that is not its sentinel (-1 for the shorts and most floats,
   -1000.0 for the last three floats) is stored in its field; the float that goes to +0x44 is converted from degrees to radians
   (divided by 180 and multiplied by pi).  uwamonoInitState runs when the last argument is 1. */
INCLUDE_ASM("asm/main/nonmatchings/tu072", enepcStoreParams);

INCLUDE_ASM("asm/main/nonmatchings/tu072", enepc_001F5B60);
