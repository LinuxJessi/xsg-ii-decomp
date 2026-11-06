#include "common.h"
#include "shared.h"

/* The DMAC takes scratchpad addresses (0x7000xxxx) as an offset with the SPR flag in bit 31. */
#define DMA_SPR_ADDRESS_HIGH 0x7000U

#define DMA_SPR_OFFSET_MASK 0x3fffU
#define DMA_SPR_FLAG 0x80000000U
#define DMA_CHCR_STR 0x100

/* D_ENABLEW bit 16 (CPND): holds all DMA transfers while a channel register is written. */
#define D_ENABLE_HOLD 0x10000

#define D_ENABLEW ((volatile u32 *) 0x1000f590)
#define D3_CHCR   ((volatile u32 *) 0x1000b000)
#define D3_MADR   ((volatile u32 *) 0x1000b010)
#define D3_QWC    ((volatile u32 *) 0x1000b020)

extern int DIntr(void);
extern int EIntr(void);

/* Starts a DMA from the IPU output (DMAC channel 3) into memory; count is in quadwords. */
int myDmaDirectFromIPU(u32 address, u32 count)
{
    if ((address >> 16) != DMA_SPR_ADDRESS_HIGH)
        *D3_MADR = address;
    else
        *D3_MADR = (address & DMA_SPR_OFFSET_MASK) + DMA_SPR_FLAG;
    *D3_QWC = count;

    DIntr();
    *D_ENABLEW = *D_ENABLEW | D_ENABLE_HOLD;
    *D3_CHCR = DMA_CHCR_STR;
    *D_ENABLEW = *D_ENABLEW & ~D_ENABLE_HOLD;
    return EIntr();
}
