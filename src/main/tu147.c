#include "common.h"
#include "shared.h"
#include "main/tu147.h"

/*
 * The global block at gameWork, shared with other translation units that
 * use other views of it. Only the flags word at offset 8 is accessed here;
 * the bytes before it are not modelled here.
 */
typedef struct GlobalFlagBlock {
    unsigned char unmodeled_00[8];
    u32 flags;
} GlobalFlagBlock;

extern GlobalFlagBlock gameWork;
extern int msgBalance;

/* Stores value in msgBalance, read back by msgBalanceGet. */
void msgBalanceSet(int value)
{
    msgBalance = value;
}

/* Returns msgBalance. */
int msgBalanceGet(void)
{
    return msgBalance;
}

/* Clears bit 0x8000 of the flags word of gameWork. */
void msgGameFlagClear(void)
{
    gameWork.flags &= ~0x8000;
}

/* Sets bit 0x8000 of the flags word of gameWork. */
void msgGameFlagSet(void)
{
    gameWork.flags |= 0x8000;
}

/* Stores 10000000 (0x989680) in msgBalance. */
void msgBalanceReset(void)
{
    msgBalanceSet(0x989680);
}
