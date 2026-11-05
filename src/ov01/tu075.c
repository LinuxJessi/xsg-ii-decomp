#include "common.h"
#include "shared.h"

/* EE scratchpad RAM. */
#define SCRATCHPAD_BASE 0x70000000

/* Default packet capacity used when no custom buffer is supplied. */
#define MGS_PACKET_DEFAULT_CAPACITY 0x400

/*
 * The GS direct-transfer packet MGsGPInit prepares and MGsGPTerm flushes.
 * By default it targets the scratchpad with a fixed capacity; a caller that
 * supplies its own buffer address also supplies that buffer's capacity
 * (MGsGPInit: movn on both the address and the size against the same
 * condition register).
 *
 *   data      the transfer base address: MGsGPInit's default/custom value,
 *             and the address MGsGPTerm passes to sceVif1PkAddDirectDataN.
 *   capacity  the buffer's byte capacity, set by MGsGPInit; no function in
 *             this TU reads it back.
 *   current   reset to data by both MGsGPInit and MGsGPTerm; the code that
 *             advances it while queuing packet contents is outside this TU.
 *   count     the queued transfer size, passed to sceVif1PkAddDirectDataN
 *             and reset to zero after each flush.
 *   auxCount  reset to zero alongside count by both functions; no further
 *             access is evidenced in this TU.
 */
typedef struct MGsPacket {
    void *data;      /* +0x00 */
    int capacity;    /* +0x04 */
    void *current;   /* +0x08 */
    int count;       /* +0x0C */
    int auxCount;    /* +0x10 */
} MGsPacket;

INCLUDE_ASM("asm/nonmatchings/ov01/tu075", func_00AAAD58);

INCLUDE_ASM("asm/nonmatchings/ov01/tu075", func_00AAADA0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu075", func_00AAADC0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu075", func_00AAADE0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu075", func_00AAADF0);

void MGsGPInit(MGsPacket *packet, void *address, int size)
{
    void *base = (address != 0) ? address : (void *)SCRATCHPAD_BASE;

    packet->data = base;
    packet->capacity = (address != 0) ? size : MGS_PACKET_DEFAULT_CAPACITY;
    packet->current = base;
    packet->count = 0;
    packet->auxCount = 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu075", func_00AAAE28);

INCLUDE_ASM("asm/nonmatchings/ov01/tu075", func_00AAAEC8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu075", MSeFire);
