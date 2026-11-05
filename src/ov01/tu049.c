#include "common.h"
#include "shared.h"

/* grGpInit's argument: the VIF1 DIRECT packet grPacketSend transmits, a
 * data buffer and its length. grGpInit points data at the scratchpad
 * (0x70000000). */
typedef struct GrPacket {
    void *data;  /* +0x0 */
    int count;   /* +0x4 */
} GrPacket;

/* GIF packet words are written as doubleword descriptors by the openers and
 * as 32-bit lanes by the vertex writers. Both are the same packet storage. */
typedef union GrGifWord {
    unsigned long long bits;
    unsigned int lanes[2];
} GrGifWord;

typedef struct GrGifOpenTag {
    GrGifWord control;
    GrGifWord register_id;
} GrGifOpenTag;

static inline void grGifOpenRegister(GrPacket *packet, unsigned long long register_id)
{
    GrGifOpenTag *tags = packet->data;
    tags[packet->count].register_id.bits = register_id;
}

void GS_REG_SET(GrPacket *packet, long long register_id, long long control)
{
    GrGifOpenTag *tags;
    int *count;
    int tag_index;

    grGifOpenRegister(packet, register_id);
    count = &packet->count;
    tags = packet->data;
    tag_index = *count;
    tags[tag_index].control.bits = control;
    *count = tag_index + 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu049", func_00AA2D18);

INCLUDE_ASM("asm/nonmatchings/ov01/tu049", func_00AA2D50);

INCLUDE_ASM("asm/nonmatchings/ov01/tu049", func_00AA2D90);

INCLUDE_ASM("asm/nonmatchings/ov01/tu049", func_00AA2DD0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu049", func_00AA2E10);

INCLUDE_ASM("asm/nonmatchings/ov01/tu049", func_00AA2E28);

INCLUDE_ASM("asm/nonmatchings/ov01/tu049", func_00AA2F60);

INCLUDE_ASM("asm/nonmatchings/ov01/tu049", func_00AA32F0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu049", func_00AA3330);
