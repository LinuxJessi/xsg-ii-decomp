#include "common.h"

/* Both return 0; their callers are not read in this pass. */
int func_00259628(void) {
    return 0;
}

int func_00259630(void) {
    return 0;
}

extern unsigned char D_00804970[];

/* Address of entry entry_index in the table D_00804970, whose entries are 20 bytes each. */
void *func_00259638(int entry_index) {
    return D_00804970 + entry_index * 0x14;
}
