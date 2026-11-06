#include "common.h"
#include "shared.h"

typedef struct XglPacket XglPacket;

/* EE scratchpad RAM. */
#define SCRATCHPAD_BASE 0x70000000

extern void sceVif1PkCnt(XglPacket *packet, int count);
extern void sceVif1PkAddCode(XglPacket *packet, unsigned int code);
extern void sceVif1PkOpenDirectHLCode(XglPacket *packet, int mode);
extern void sceVif1PkCloseDirectHLCode(XglPacket *packet);
extern XglPacket *xglPacketGetCurrent(void);

/*
 * The VIF1 direct-mode packet builder object. xglPacketInit constructs two
 * of them back to back inside asPacketSource, 0x28 bytes apart (main
 * 0x0022c458-0x0022c4b4 writes both entries' +0x20 and +0x24 at that
 * stride), so sizeof(XglPacket) is 0x28. Only the members the functions of
 * this TU touch are named; everything the sceVif1Pk* library alone reads or
 * writes stays unmodeled.
 */
struct XglPacket {
    /* xglFontReloadTexture decrements this forward cursor by four words. */
    u32 *write_cursor;
    /*
     * +0x04: the address xglPacketSend hands to xglDmaDirectSrcChain as the
     * DMA source-chain start whenever a packet is pending (main
     * 0x0022c520-0x0022c548). No function of this TU writes it -- neither
     * xglPacketInit nor xglPacketMove touches +0x04 -- so it is set by the
     * sceVif1Pk* library: sceVif1PkInit (main 0x0020ac68) stores its base
     * argument at +0x00 and +0x04, and xglPacketInit passes it each entry's
     * buffer base (0x00c00000 and 0x00e00000).
     */
    u32 start;
    /*
 * +0x08 and +0x0c: xglPrimAddGifTagDirect (main 0x00222c00 lw v0,8(s0);
 * main 0x00222c0c lw v0,0xc(s0)) tests both for non-zero to decide
 * whether a direct-mode block the sceVif1Pk* library opened is still
 * open, so it appends to that block instead of resetting/opening/
 * closing a new one (compare SGsSendPacket, src/main/sv_get_image_
 * item.c, which always resets/opens/closes without this check).
 * Neither word is written by any function of this TU, so both are set
 * by the sceVif1Pk* library.
 */
    u32 directCode;
    u32 *directState;
    unsigned char unmodeled_10[0x10];
    /*
     * +0x20: the packet's fixed top-of-buffer address. xglPacketInit sets it
     * once per entry after both sceVif1PkInit calls (main 0x0022c49c for the
     * first entry, value 0x00e00000; main 0x0022c4a0 for the second, value
     * 0x01000000): the end of the buffer whose base went to sceVif1PkInit.
     */
    u32 limit;
    /*
     * +0x24: the write cursor. xglPacketInit seeds it equal to `limit` (main
     * 0x0022c48c/0x0022c490) and xglPacketMove reloads `limit` into it at
     * the start of every frame (main 0x0022c4e4/0x0022c4f0), so writers that
     * fill the packet backward from the top always restart from the same
     * address. main/tu107's nmlPacketSetAttributeData and
     * nmlPacketSetAttributeAlloc16N read it, decrement it by the size being
     * requested and store it back, then use the new value as the memcpy
     * destination or return it as the allocation (main 0x00238e28-0x00238e3c
     * and 0x00238fe0-0x00238ff4),
     * which is why the member is byte-addressed instead of scaled.
     */
    u8 *cursor;
};

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

/*
 * grRotTransPers's output vertex: the screen-space X/Y grPutXxx (siblings,
 * not decoded here) feed to the GS in 12.4 fixed point (the *16.0f below),
 * the projected depth as a plain integer, and the out-of-frustum bit
 * (0x8000) grRotTransPers itself copies from *clipFlags.
 */
typedef struct GrScreenVertex {
    int x;      /* +0x00 */
    int y;      /* +0x04 */
    int z;      /* +0x08 */
    int flags;  /* +0x0c */
} GrScreenVertex;

/* One GIF packed-mode quadword: an A+D register write, or as the vertex
 * writers fill it ST/Q, RGBAQ or XYZ2 (the vertex grRotTransPers writes). */
typedef struct GrGifSTQ {
    float s; /* +0x00 */
    float t; /* +0x04 */
    float q; /* +0x08 */
} GrGifSTQ;

typedef struct GrGifRGBA {
    int r; /* +0x00 */
    int g; /* +0x04 */
    int b; /* +0x08 */
    int a; /* +0x0c */
} GrGifRGBA;

/* The UV quadword grPutFT4 writes: UV coordinates in 12.4 fixed point,
 * converted from float S/T as unsigned (the c.le.s against 2^31 and the or of
 * 0x80000000). */
typedef struct GrGifUV {
    unsigned int u; /* +0x00 */
    unsigned int v; /* +0x04 */
} GrGifUV;

typedef union GrGifQuad {
    GrGifOpenTag tag;
    GrGifSTQ stq;
    GrGifUV uv;
    GrGifRGBA rgba;
    GrScreenVertex xyz;
} GrGifQuad;

/* The packet's quadword at index, from a fresh read of packet->data. */
#define GR_PACKET_QUAD(packet, index) ((GrGifQuad *)(packet)->data + (index))

static inline void grGifOpenRegister(GrPacket *packet, unsigned long long register_id)
{
    GrGifOpenTag *tags = packet->data;
    tags[packet->count].register_id.bits = register_id;
}

static inline void grGifAppendRegister(GrPacket *packet,
                                       unsigned long long register_id,
                                       unsigned long long control)
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

typedef struct GrVramCopyParams {
    unsigned short source_base;
    unsigned short source_width;
    unsigned short destination_base;
    unsigned short destination_width;
    unsigned short source_x;
    unsigned short source_y;
    unsigned short destination_x;
    unsigned short destination_y;
    unsigned short width;
    unsigned short height;
} GrVramCopyParams;

extern void grVramCopy(GrVramCopyParams *params);
extern void grGpInit(GrPacket *packet);
void grPacketSend(GrPacket *packet);

typedef struct GrRenderState {
    unsigned char unmodeled_00[0x20];
    unsigned short current_buffer;
    unsigned short drawing_buffer;
} GrRenderState;

extern GrRenderState sRender;
/* Framebuffer base addresses; grFBAdrGet indexes this table. */
extern int fb[4];

/* VIF FLUSH code (command 0x11 in bits 24..30, no immediate). */
#define VIF_CODE_FLUSH 0x11000000

extern void sceVif1PkAddDirectDataN(XglPacket *packet, const void *data,
                                    int count);
extern void sceVif1PkTerminate(XglPacket *packet);
void grPacketSend(GrPacket *packet);

/* One GS register write of grGsRegListSet's list; register_id -1 ends it. */
typedef struct GrGsRegListEntry {
    int register_id;          /* +0x00 */
    unsigned long long value; /* +0x08 */
} GrGsRegListEntry;

void grGpInit(GrPacket *packet)
{
    packet->count = 0;
    packet->data = (void *)SCRATCHPAD_BASE;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu048", grPutF2Open);

/* One record per primitive kind (F2, F4, FT4, FT4STQ, Spr). Each Open function
 * below appends a GIF packed-mode tag to the packet, stores the address of the
 * tag's control doubleword in `control` and clears `count`. The grPutXxx vertex
 * writers reference the same record (their data refs; their count update is not
 * yet read). The matching Close function ORs `count` into the tag's control
 * doubleword, the GIF NLOOP field, after the draw of those vertices. */
typedef struct GrPutTag {
    unsigned long long *control; /* +0x00 */
    int count;                   /* +0x04 */
} GrPutTag;

extern GrPutTag grPutF2Tag;

void grPutF2Close(void) {
    *grPutF2Tag.control |= grPutF2Tag.count;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu048", grPutF4Open);

extern GrPutTag grPutF4Tag;

void grPutF4Close(void) {
    *grPutF4Tag.control |= grPutF4Tag.count;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu048", grPutFT4Open);

extern GrPutTag grPutFT4Tag;

void grPutFT4Close(void) {
    *grPutFT4Tag.control |= grPutFT4Tag.count;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu048", grPutFT4STQOpen);

extern GrPutTag grPutFT4STQTag;

void grPutFT4STQClose(void) {
    *grPutFT4STQTag.control |= grPutFT4STQTag.count;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu048", grPutSprOpen);

extern GrPutTag grPutSprTag;

void grPutSprClose(void) {
    *grPutSprTag.control |= grPutSprTag.count;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu048", grPutF2);

INCLUDE_ASM("asm/nonmatchings/ov01/tu048", grPutF4);

INCLUDE_ASM("asm/nonmatchings/ov01/tu048", grPutFT4);

INCLUDE_ASM("asm/nonmatchings/ov01/tu048", grPutFT4STQ);

INCLUDE_ASM("asm/nonmatchings/ov01/tu048", grPutSpr);

void func_0012B4B8(XglPacket *packet);
void func_0012B9A0(XglPacket *packet, unsigned int code);

void grPacketSend(GrPacket *packet)
{
    XglPacket *current_packet;

    if (packet->count == 0) {
        return;
    }
    current_packet = xglPacketGetCurrent();
    sceVif1PkCnt(current_packet, 0);
    func_0012B9A0(current_packet, VIF_CODE_FLUSH);
    sceVif1PkOpenDirectHLCode(current_packet, 0);
    sceVif1PkAddDirectDataN(current_packet, packet->data, packet->count);
    sceVif1PkCloseDirectHLCode(current_packet);
    func_0012B4B8(current_packet);
}

void grGsRegSet(GrPacket *packet, unsigned long long register_id,
                unsigned long long value)
{
    grGifAppendRegister(packet, 14, 0x1000000000008001ULL);
    grGifAppendRegister(packet, register_id, value);
}

void grGsRegListSet(GrPacket *packet, GrGsRegListEntry *entries)
{
    GrGifOpenTag *head;
    GrGifOpenTag *tags;
    int *count;
    int tag_index = packet->count;
    int register_count = 0;

    grGifAppendRegister(packet, 14, 0x1000000000008000ULL);
    while (entries->register_id != -1) {
        grGifOpenRegister(packet, entries->register_id);
        count = &packet->count;
        tags = packet->data;
        tags[(*count)++].control.bits = entries->value;
        entries++;
        register_count++;
    }
    head = packet->data;
    head[tag_index].control.bits |= register_count;
}

int grFBAdrGet(int index)
{
    return fb[index];
}

int grBBIdxGet(void)
{
    int index = 0;

    if (fb[0] != 0xFFFFF) {
        while (fb[index] != 0xFFFFF) {
            if (fb[index] != sRender.drawing_buffer &&
                fb[index] != sRender.current_buffer) {
                return index;
            }
            index++;
        }
    }

    return -1;
}

int grDBIdxGet(int *drawing_index, int *current_index)
{
    int index = 0;

    if (fb[0] != 0xFFFFF) {
        for (; fb[index] != 0xFFFFF; index++) {
            if (sRender.drawing_buffer == fb[index]) {
                *drawing_index = index;
            }
            if (sRender.current_buffer == fb[index]) {
                *current_index = index;
            }
        }
    }

    return 1;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu048", grVramCopyBuffers);

/* Local-to-local GS transfer: BITBLTBUF (0x50), TRXPOS (0x51), TRXREG
 * (0x52), then TRXDIR (0x53) = 2 starts it. */
void grVramCopy(GrVramCopyParams *params)
{
    GrPacket packet;
    int *count;

    grGpInit(&packet);
    grGifOpenRegister(&packet, 14);
    count = &packet.count;
    GR_PACKET_QUAD(&packet, (*count)++)->tag.control.bits = 0x1000000000008004ULL;

    grGifOpenRegister(&packet, 0x50);
    count = &packet.count;
    GR_PACKET_QUAD(&packet, (*count)++)->tag.control.bits =
        params->source_base |
        (unsigned long long)params->source_width << 16 |
        (unsigned long long)params->destination_base << 32 |
        (unsigned long long)params->destination_width << 48;

    grGifOpenRegister(&packet, 0x51);
    count = &packet.count;
    GR_PACKET_QUAD(&packet, (*count)++)->tag.control.bits =
        params->source_x |
        (unsigned long long)params->source_y << 16 |
        (unsigned long long)params->destination_x << 32 |
        (unsigned long long)params->destination_y << 48;

    grGifOpenRegister(&packet, 0x52);
    count = &packet.count;
    GR_PACKET_QUAD(&packet, (*count)++)->tag.control.bits =
        params->width | (unsigned long long)params->height << 32;

    grGifOpenRegister(&packet, 0x53);
    count = &packet.count;
    GR_PACKET_QUAD(&packet, (*count)++)->tag.control.bits = 2;

    grPacketSend(&packet);
}
