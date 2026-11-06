#include "common.h"
#include "main/srs.h"
#include "main/tu173.h"
#include "main/xgl_cd.h"

INCLUDE_ASM("asm/main/nonmatchings/tu173", loadWaitAdd);

INCLUDE_ASM("asm/main/nonmatchings/tu173", loadWaitPoll);

INCLUDE_ASM("asm/main/nonmatchings/tu173", loadWaitBusy);

int loadWaitBusy(void);

/* Returns 1 while the load wait table (loadWaitBusy) holds an id or the srs read counter is positive. */
int loadWaitPending(void)
{
    if (loadWaitBusy() != 0) {
        return 1;
    }
    return srsLeaveCdRead() > 0;
}


/* Returns once the CD status test xglCdServiceIfPending reports idle. */
void xglCdWaitUntilIdle(void)
{
    while (xglCdServiceIfPending() != 0) {
    }
}
