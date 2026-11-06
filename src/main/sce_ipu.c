#include "common.h"
#include "shared.h"
#include "main/sce_ipu.h"

/* D_ENABLEW bit 16 (CPND): holds all DMA transfers while a channel register is written. */
#define D_ENABLE_HOLD 0x10000

#define D_ENABLEW ((volatile u32 *) 0x1000f590)
/* DMAC channel 4 feeds the IPU (memory to IPU input); its control register takes the transfer mode and start bit. */
#define D4_CHCR   ((volatile u32 *) 0x1000b400)

extern int DIntr(void);
extern int EIntr(void);

/* Starts or stops the IPU input transfer by writing D4_CHCR with the DMA interrupts held off. */
void setD4_CHCR(u32 value)
{
    DIntr();
    *D_ENABLEW = *D_ENABLEW | D_ENABLE_HOLD;
    *D4_CHCR = value;
    *D_ENABLEW = *D_ENABLEW & ~D_ENABLE_HOLD;
    EIntr();
}
