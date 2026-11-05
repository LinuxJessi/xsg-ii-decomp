#include "common.h"
#include "shared.h"

/* D_ENABLEW bit 16 (CPND): holds all DMA transfers while a channel register is written. */
#define D_ENABLE_HOLD 0x10000

#define D_ENABLEW ((volatile u32 *) 0x1000f590)
#define D4_CHCR   ((volatile u32 *) 0x1000b400)

extern int DIntr(void);
extern int EIntr(void);

void setD4_CHCR(u32 value)
{
    DIntr();
    *D_ENABLEW = *D_ENABLEW | D_ENABLE_HOLD;
    *D4_CHCR = value;
    *D_ENABLEW = *D_ENABLEW & ~D_ENABLE_HOLD;
    EIntr();
}
