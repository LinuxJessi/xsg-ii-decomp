#include "common.h"
#include "main/sef.h"

/* Both return 0; their callers are not read in this pass. */
int func_00259628(void) {
    return 0;
}

int func_00259630(void) {
    return 0;
}

/* Address of entry entry_index in the table sefEntryCounts (SefEntryCounts, 20 bytes each). */
void *func_00259638(int entry_index) {
    return &sefEntryCounts[entry_index];
}
