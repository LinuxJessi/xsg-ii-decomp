#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tya_draw_gauge", tyaDrawGauge);

/* Empty body; no caller is known. */
void func_001EECD8(void)
{
}

/* Loads a uwamono scene: saves the records in use, reads sc0000.sb and the
 * cf\xx\ file whose digits come from the id argument, then resets the records
 * through uwamonoResetRecords(-1, 1). Name not settled; see the naming notes. */
INCLUDE_ASM("asm/main/nonmatchings/tya_draw_gauge", func_001EECE0);

/* gameWork is the uwamono work block (see the main/tu048 proposal). */
extern unsigned char gameWork[];

/* Clears two runs of CLEAR_RUN_LENGTH bytes: one ending CLEAR_RUN_GAP bytes
 * before offset GAUGE_CLEAR_BASE, and one starting at GAUGE_CLEAR_BASE. */
#define CLEAR_RUN_LENGTH 512
#define CLEAR_RUN_GAP 0x200
#define GAUGE_CLEAR_BASE 0x1920

void func_001EEF60(void)
{
    unsigned char *cursor;
    int i;

    cursor = gameWork;
    cursor += GAUGE_CLEAR_BASE;
    for (i = 0; i < CLEAR_RUN_LENGTH; i++) {
        cursor[i - CLEAR_RUN_GAP] = 0;
        cursor[i] = 0;
    }
}

/* Clears CLEAR_RUN_LENGTH bytes walking backwards from offset 0x171F. */
void func_001EEF98(void)
{
    unsigned char *cursor;
    int offset;
    int i;

    offset = 0x171F;
    cursor = gameWork + offset;
    for (i = 0; i < CLEAR_RUN_LENGTH; i++) {
        *cursor = 0;
        cursor--;
    }
}

/* Resets uwamono records: one record (index >= 0) or all 256 (index -1), with
 * mode 1 also releasing the pointer at +0x330 through XenoActorRelease. Calls
 * uwamonoResetPoints. */
INCLUDE_ASM("asm/main/nonmatchings/tya_draw_gauge", uwamonoResetRecords);
