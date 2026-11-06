/*
 * OV01 original TU 28: 0x00a8f658..0x00a90368 (11 functions)
 */
#include "common.h"

extern void *memset(void *destination, int value, unsigned int count);

/*
 * 0x50-byte battle result block: word totals at +0x00, +0x04 and +0x08; up to
 * eight (code, amount) pairs from +0x0C and +0x2C; the pair count at +0x4C.
 * Evidence is the layout the callers write, not a named struct.
 */
extern unsigned char battleResult[];

/* Clears the battle result block. */
void battleResultReset(void) {
    memset(battleResult, 0, 0x50);
}

/* Returns the battle result block. */
unsigned char *battleResultGet(void) {
    return battleResult;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu028", func_00A8F690);

/*
 * Party snapshots: clears partySnapshots (0x5D0 bytes) and partySnapshotPtrs, copies each
 * party unit (unitTblGet side 0) into a 0x1F0-byte slot of partySnapshots and
 * stores that slot's address in partySnapshotPtrs.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu028", partySnapshotBuild);

/* Three pointers, one per party member, to the snapshots built by partySnapshotBuild. */
extern unsigned char partySnapshotPtrs[];

/* Stores the snapshot pointer table in *snapshots and returns the party size, 3. */
int partySnapshotGet(void **snapshots) {
    *snapshots = partySnapshotPtrs;
    return 3;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu028", func_00A8F948);

INCLUDE_ASM("asm/nonmatchings/ov01/tu028", func_00A8FA18);

INCLUDE_ASM("asm/nonmatchings/ov01/tu028", func_00A8FC30);

INCLUDE_ASM("asm/nonmatchings/ov01/tu028", func_00A8FE68);

INCLUDE_ASM("asm/nonmatchings/ov01/tu028", unitExpGain);

INCLUDE_ASM("asm/nonmatchings/ov01/tu028", func_00A90118);
