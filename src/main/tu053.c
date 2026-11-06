#include "common.h"

/* Returns 0. Same 8-byte body as func_001DF088 in main/tu052. No caller found in the binary. */
int func_001DF090(void) {
    return 0;
}

/*
 * Footstep material keys (proposed footMaterialKeys): 4-byte entries holding three letters, the fourth byte zero.
 * The list ends at an entry whose first byte is zero. Read by footMaterialFind and footMaterialKey.
 */
extern int footMaterialKeys[];

/*
 * Returns the key entry at `index` (proposed footMaterialKey). enefoot and func_001E4890 copy its three bytes into
 * their sound command.
 */
void *footMaterialKey(int index) {
    return &footMaterialKeys[index];
}

/*
 * Searches the key list for the three bytes of `name` (proposed footMaterialFind) and returns the matching index.
 * It returns 0 when the list ends without a match, which is also the index of "kon", the default material.
 * foot calls it once per token.
 */
INCLUDE_ASM("asm/main/nonmatchings/tu053", footMaterialFind);
