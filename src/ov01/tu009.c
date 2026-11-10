/*
 * OV01 original TU 9: 0x00a83d28..0x00a840f0 (15 functions)
 */
#include "common.h"
#include "shared.h"
#include "main/xgl_cd.h"
#include "ov01/tu009.h"

/* The fourth argument is a completion callback: ov01CdReadFileAsync passes ov01CdReadCallback in $7. */
extern void GameResArenaReserve(void);
extern u32 GameResFindID(int key, void *buffer);
/* Scaffold-owned data: the splat names of the words this TU reads. */
/* ov01HeapStart is the overlay heap range: word 0 is the first block, word 1 (ov01HeapEnd) the end. */
extern int ov01CdPendingCount; /* pending CD read count */
extern u32 ov01HeapStart[2];
extern u32 ov01HeapEnd;

/*
 * Header of one heap block, 0x40 bytes; the payload follows it. Word 0 is a
 * flags word (bit 0 set = in use). The scaffold declares it as bytes; the asm
 * reads it as one word. Word 4 is the payload size.
 */
typedef struct Ov01HeapBlock {
    unsigned char flags[4];
    int size;
} Ov01HeapBlock;

/* Pending CD read count. */
int ov01CdPendingReads(void)
{
    return ov01CdPendingCount;
}

/* Clears the pending CD read count; the overlay init calls it. */
void ov01CdPendingReset(void)
{
    ov01CdPendingCount = 0;
}

/* Completion callback of ov01CdReadFileAsync: code 4 ends one read and lowers the pending count. */
void ov01CdReadCallback(int code)
{
    int count;

    if (code >= 0) {
        if (code == 4) {
            count = ov01CdPendingCount - 1;
            if (count < 0) {
                count = 0;
            }
            ov01CdPendingCount = count;
        }
    }
}

/* Starts one asynchronous CD read (mode 1, completion callback ov01CdReadCallback); a successful start raises the pending count. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu009", ov01CdReadFileAsync);

/* Synchronous CD read (mode 0): true when xglCdReadFile reports success (>= 0). */
u64 ov01CdReadFileSync(const char *name, void *buffer)
{
    return xglCdReadFile(name, buffer, 0, 0) >= 0;
}

/* Tail-jumps to GameResArenaReserve: the heap base comes back in $2 (the C prototype says void). */
void ov01HeapBaseGet(void)
{
    GameResArenaReserve();
}

/* Key lookup through GameResFindID: a result inside the heap range gives 0, any other result is returned. */
u32 ov01LookupNonHeap(int key)
{
    u32 buffer[4];
    u32 address;

    address = GameResFindID(key, buffer);
    if (address >= ov01HeapStart[0] && address <= ov01HeapStart[1]) {
        return 0;
    }
    return address;
}

/* Payload pointer to block header: the header sits 0x40 bytes before its payload. */
int ov01HeapBlockOf(int payload)
{
    return payload - 0x40;
}

/* Heap init, called by the overlay init dataFileInit: base from ov01HeapBaseGet, 0xA00000 bytes, one free block. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu009", ov01HeapInit);

/*
 * Next block header after this one. Block sizes are always multiples of 0x40
 * (ov01HeapAlloc rounds them), so the rounding only trims the signed case.
 * NULL past the heap end.
 */
void *ov01HeapNextBlock(Ov01HeapBlock *block)
{
    int payloadSize;
    int roundedSize;
    unsigned char *nextBlock;

    payloadSize = block->size;
    roundedSize = payloadSize;
    if (payloadSize < 0) {
        roundedSize = payloadSize + 63;
    }
    nextBlock = (unsigned char *) block + ((roundedSize >> 6) << 6) + 0x40;
    if ((unsigned int) nextBlock >= ov01HeapEnd) {
        return 0;
    }
    return nextBlock;
}

/* First fit: the first block whose flags AND mask are clear and whose size covers the request; NULL when none does. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu009", ov01HeapFindFree);

/* Allocates size bytes (rounded up to 0x40) from the heap: splits a larger free block and returns the payload, NULL if nothing fits. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu009", ov01HeapAlloc);

/* Frees a payload: clears its in-use bit, then merges each run of free neighbours across the heap. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu009", ov01HeapFree);

/* Allocates 0x2A800 bytes and frees them again: 1 when that allocation fits in the heap, 0 otherwise. */
INCLUDE_ASM("asm/nonmatchings/ov01/tu009", func_00A840A8);

int func_00A840E8(void)
{
    return 1;
}
