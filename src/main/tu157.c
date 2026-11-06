#include "common.h"
#include "shared.h"

typedef struct XglPacket XglPacket;

/*
 * The draw-list cursor at svDrawList. entries is the base of a table of
 * 0x10-byte steps; begin_index selects the step svDrawListFlush reads.
 * end_index minus begin_index is the number of queued steps, which
 * svDrawListTryFlush compares with 0x1AE before it calls svDrawListFlush.
 */
typedef struct DrawList {
    u8 *entries;
    u8 unmodeled_04[0x04];
    int end_index;
    int begin_index;
} DrawList;

/* The record at entries + begin_index * 0x10; its words at +0x14 and +0x18 are the operands. */
typedef struct DrawListEntry {
    u8 unmodeled_00[0x14];
    int operand_a;
    int operand_b;
} DrawListEntry;

extern void *memset(void *destination, int value, unsigned int count);
extern void sceVif1PkCnt(XglPacket *packet, int count);
extern void sceVif1PkRef(XglPacket *packet, const void *environment,
                         int count, int mode, int offset, int flags);
extern XglPacket *xglPacketGetCurrent(void);
void func_0012B9A0(XglPacket *packet, u32 word);
void svDrawListAppend(int code, int operand_a, int operand_b);
void svDrawListEmit(XglPacket *packet, DrawListEntry *entry, int pending_count);
extern int svSortCount;
extern unsigned char svSortItems[];
extern unsigned char svSortBuckets[];
extern int D_0069E304;
extern int _draw3D;
extern u32 D_003ED4A0[];
extern DrawList svDrawList;
extern int D_002B3740[];
extern unsigned char D_002B3750[];

/*
 * Clears the depth-sort state: the item count, the 0x200 bucket heads
 * (svSortBuckets, 12 bytes each) and the item table (svSortItems, 12 bytes
 * each). Every slot is set to -1, the empty marker.
 */
void svSortReset(void)
{
    svSortCount = 0;
    memset(svSortBuckets, -1, 0x1800);
    memset(svSortItems, -1, 0x3840);
}

/*
 * Inserts an item (id, flags, depth) into the depth-sort buckets. The bucket
 * is depth * 20, clamped to 0x1FF; inside a bucket the items stay in
 * ascending depth order. Refused once 0x4B0 items are queued.
 */
INCLUDE_ASM("asm/main/nonmatchings/tu157", svSortAdd);

/* Starts the draw list (svDrawList) in one of two buffers, chosen by the flag, with both cursors at zero. */
INCLUDE_ASM("asm/main/nonmatchings/tu157", svDrawListBegin);

/*
 * Appends one draw record at the end cursor: a VIF FLUSH, STCYCL 4,4, an
 * UNPACK to VU address `code` and the operands. The record starts at the
 * old end, which becomes begin_index, and end_index moves on by 2.
 */
INCLUDE_ASM("asm/main/nonmatchings/tu157", svDrawListAppend);

/* Emits the VIF packet for `count` records at `entry`: sceVif1PkRef calls and UNPACK headers, split at 0x100 records. */
INCLUDE_ASM("asm/main/nonmatchings/tu157", svDrawListEmit);

/*
 * Flushes the queued draw records. Runs only with at least three queued,
 * _draw3D != 2 and D_0069E304 > 0: it emits the records, FLUSHes, calls the
 * draw microprogram of the current mode (MSCAL), then appends record 0x28.
 */
void svDrawListFlush(int pending_count)
{
    XglPacket *packet;
    u32 tag;
    DrawListEntry *entry;

    if (pending_count >= 3 && _draw3D != 2 && D_0069E304 > 0) {
        packet = xglPacketGetCurrent();
        svDrawListEmit(packet,
                      (DrawListEntry *) (svDrawList.entries + svDrawList.begin_index * 0x10),
                      pending_count);
        sceVif1PkCnt(packet, 0);
        func_0012B9A0(packet, 0x11000000);
        tag = D_003ED4A0[D_0069E304 * 2];
        func_0012B9A0(packet, (tag >> 3) | 0x14000000);
        func_0012B9A0(packet, 0x11000000);
        entry = (DrawListEntry *) (svDrawList.entries + svDrawList.begin_index * 0x10);
        svDrawListAppend(0x28, entry->operand_a, entry->operand_b);
    }
}

void svDrawListTryFlush(void)
{
    int pending_count;

    pending_count = svDrawList.end_index - svDrawList.begin_index;
    if (pending_count >= 0x1AE) {
        svDrawListFlush(pending_count);
    }
}

void svDrawSendStaticRef(void)
{
    XglPacket *packet;

    packet = xglPacketGetCurrent();
    sceVif1PkCnt(packet, 0);
    func_0012B9A0(packet, 0x11000000);
    sceVif1PkRef(packet, D_002B3750, D_002B3740[0], 0, 0, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/tu157", func_00253800);

INCLUDE_ASM("asm/main/nonmatchings/tu157", func_002539B0);

INCLUDE_ASM("asm/main/nonmatchings/tu157", func_00253A28);
