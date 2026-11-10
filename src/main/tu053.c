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
 * Returns the key entry at `index` (proposed footMaterialKey). enefoot and seFootBanksLoad copy its three bytes into
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
int footMaterialFind(const char *name) {
    const char *entry = (const char *)footMaterialKeys;
    int index = 0;

    while (entry[0] != 0) {
        if (entry[0] == name[0] && entry[1] == name[1] && entry[2] == name[2]) {
            return index;
        }
        entry += 4;
        index++;
    }
    return 0;
}
