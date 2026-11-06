#include "common.h"
#include "shared.h"
#include "main/tu119.h"
#include "main/yg2path.h"

void segAddrOnClearAll(void);

void yg2PathResetAll(void)
{
    yg2PathClearProgress();
    segAddrOnClearAll();
}

/* Eighteen signed-char flags, one per segment address (index = address number - 1): written by the
   seg.addrOn native (segSetAddrOn), read by seg.getAddrOn (segGetAddrOn), and cleared by yg2PathResetAll. */
extern signed char segAddrOnFlags[18];

void segSetAddrOn(u32 index, int value)
{
    if (index < 18) {
        segAddrOnFlags[index] = value;
    }
}

signed char segGetAddrOn(u32 index)
{
    if (index >= 18) {
        return 0;
    }
    return segAddrOnFlags[index];
}

void segAddrOnClearAll(void)
{
    signed char *cursor;
    int i;

    cursor = segAddrOnFlags;
    cursor += 17;
    for (i = 0; i < 18; i++) {
        *cursor = 0;
        cursor--;
    }
}
