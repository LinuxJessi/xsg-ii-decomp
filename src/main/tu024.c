#include "common.h"

/* Interrupt and DMA event registry. Two tables of 16-byte records share one layout:
 * {handle, count, id, handler}. The handle comes back from AddIntcHandler or
 * AddDmacHandler (kernel services 0x10 and 0x12); the count is bumped by the
 * DMAC channel handlers below; the id is the INTC cause or the DMAC channel. */

/* DMAC channel event handler. Seven copies with identical bodies, one per DMAC channel
 * registered in the DMAC table (channels 0, 1, 2, 3, 4, 8 and 9). Each one bumps the
 * count of its channel's record, re-enables interrupts (ei) and returns 0. */
INCLUDE_ASM("asm/main/nonmatchings/tu024", xglDmaEventHandler);

INCLUDE_ASM("asm/main/nonmatchings/tu024", func_001A4A98);

INCLUDE_ASM("asm/main/nonmatchings/tu024", func_001A4AD8);

INCLUDE_ASM("asm/main/nonmatchings/tu024", func_001A4B18);

INCLUDE_ASM("asm/main/nonmatchings/tu024", func_001A4B58);

INCLUDE_ASM("asm/main/nonmatchings/tu024", func_001A4B98);

INCLUDE_ASM("asm/main/nonmatchings/tu024", func_001A4BD8);

/* INTC cause -> its event count, or -1 when no handler is registered for the cause. */
INCLUDE_ASM("asm/main/nonmatchings/tu024", xglIntcGetEventCount);

/* Enables INTC cause `index` through kernel service 0x14 (EnableIntc in the PS2 kernel
 * table; external reference) when the cause has a handler. Returns 1 when it did. */
INCLUDE_ASM("asm/main/nonmatchings/tu024", xglIntcEnable);

/* Disables INTC cause `index` through kernel service 0x15 (DisableIntc) when registered. */
INCLUDE_ASM("asm/main/nonmatchings/tu024", xglIntcDisable);

/* DMAC channel -> its event count, or -1 when no handler is registered for the channel. */
INCLUDE_ASM("asm/main/nonmatchings/tu024", xglDmaGetEventCount);

void func_00121138(unsigned int channel, int value);
void func_001210D0(unsigned int channel, int value);
/* DMAC channel -> slot of its record in the DMAC table; -1 when no handler is registered. */
extern int xglDmaSlotMap[];

/* Enables DMAC channel `channel` through kernel service 0x16 (EnableDmac) when a handler
 * is registered for it. Returns 1 when it did. */
int xglDmaEnable(unsigned int channel)
{
    int valid_channel = channel < 0x10;
    int enabled = 0;
    int *slot = &xglDmaSlotMap[channel];

    if (valid_channel && (*slot >= 0)) {
        func_00121138(channel, 0);
        enabled = 1;
    }
    return enabled;
}

/* Disables DMAC channel `channel` through kernel service 0x17 (DisableDmac) when registered. */
int xglDmaDisable(unsigned int channel)
{
    int valid_channel = channel < 0x10;
    int disabled = 0;
    int *slot = &xglDmaSlotMap[channel];

    if (valid_channel && (*slot >= 0)) {
        func_001210D0(channel, 0);
        disabled = 1;
    }
    return disabled;
}

/* Registers the INTC handlers (AddIntcHandler, 0x10) and the DMAC handlers (AddDmacHandler,
 * 0x12) from their tables, fills in the slot maps and clears three globals. Called by
 * InitializeSystem. */
INCLUDE_ASM("asm/main/nonmatchings/tu024", xglIntrInit);

/* Default callback that does nothing and returns 0. */
int xglHddDummyCallback(void)
{
    return 0;
}
