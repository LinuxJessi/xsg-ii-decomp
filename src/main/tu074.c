#include "common.h"
#include "main/tu074.h"

INCLUDE_ASM("asm/main/nonmatchings/tu074", disableDrawFlags);

extern unsigned char D_003E6F58[];
extern unsigned char D_0069D308[];
void func_A9CDB0(void *, void *);

/* Registers the enepc native table D_003E6F58 with the module name "enepc". */
void enepcRegisterNatives(void) {
    func_A9CDB0(D_0069D308, D_003E6F58);
}

INCLUDE_ASM("asm/main/nonmatchings/tu074", uwamono_001F6F10);

INCLUDE_ASM("asm/main/nonmatchings/tu074", uwamono_001F7670);

INCLUDE_ASM("asm/main/nonmatchings/tu074", uwamono_001F7C38);

INCLUDE_ASM("asm/main/nonmatchings/tu074", uwamono_001F7D08);

INCLUDE_ASM("asm/main/nonmatchings/tu074", setCollisionScale);
