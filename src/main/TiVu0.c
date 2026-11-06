#include "common.h"
#include "main/tu026.h"

extern char D_0068F5F0[];
extern char tiVu0AssertFile[];

/* Unconditional assert(0) at TiVu0.euc.c line 739: a fail-only path with no role shown. */
void func_002933D0(void)
{
    assertFailed(D_0068F5F0, tiVu0AssertFile, 739); /* assert line 739 */
}

/* Unconditional assert(0) at TiVu0.euc.c line 758: a fail-only path with no role shown. */
void func_002933F8(void)
{
    assertFailed(D_0068F5F0, tiVu0AssertFile, 758); /* assert line 758 */
}

/* Reads one curve key's value by key type: 0 gives 0.0, 2 a float at +4, 3 a u16 at +6 times 1e-4, 5 a float at +8; other types assert.
 * The assert file names TiXacAnimCurve.euc.c, so this reader belongs to that family. */
INCLUDE_ASM("asm/main/nonmatchings/TiVu0", func_00293420);

/* Same type switch as func_00293420 with another key layout: 3 reads a u16 at +4 times 1e-4, 2 and 5 read a float at +4. */
INCLUDE_ASM("asm/main/nonmatchings/TiVu0", func_002934C0);
