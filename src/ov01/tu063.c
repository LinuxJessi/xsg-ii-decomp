/*
 * OV01 original TU 63: 0x00aa8200..0x00aa8590 (17 functions)
 */
#include "common.h"
#include "ov01/tu008.h"
#include "ov01/tu063.h"
#include "ov01/tu012.h"
#include "gs.h"

typedef int s32;


int menuTexCellIndex(int key)
{
    StatBlock *unit = batUnitRecordGet(key);
    unsigned short flags = unit->flags;

    if (key >= 1 && key <= 10) {
        return key - 1;
    }
    if (key >= 11 && key <= 15) {
        return key - 4;
    }
    if (key >= 16) {
        switch (flags & 3) {
        case 0:
            return 11;
        case 1:
            return 10;
        case 2:
            return 12;
        }
    }
    return 0;
}

s32 xglPacketTextureTrans(s32 value);

/*
 * Texture flush for the menu: waits on the value of dataStatBufGet, then sends
 * one GS write to register GS_TEXFLUSH with 1.
 */
void menuTexFlush(void) {
    GrPacket packet;
    s32 result;

    result = dataStatBufGet();
    xglPacketTextureTrans(result);
    grGpInit(&packet);
    grGsRegSet(&packet, GS_TEXFLUSH, 1);
    grPacketSend(&packet);
}

void GS_REG_SET(void *packet, int register_id, u64 control);

/* GIF tag of the record: NLOOP 5, EOP set, NREG 1 (the five A+D pairs that follow). */
#define GIF_TAG_FIVE_AD_EOP 0x1000000000008005ULL

/* One 16-byte texture state record: TEX0 bits 0..15 at +0, TEX0 bits 37..52 at +2, ALPHA_1 at +8. */
typedef struct TexStateRecord {
    unsigned short tex0Low;
    unsigned short tex0Clut;
    unsigned char unmodeled_04[0x8 - 0x4];
    u64 alpha;
} TexStateRecord;

/*
 * Emits one 16-byte texture state record into the packet: GS TEX0 from the
 * halfwords at +0 and +2, ALPHA from the word at +8, then TEST, CLAMP and TEX1.
 */
void grPacketTexState(GrPacket *packet, void *record)
{
    TexStateRecord *texState = record;

    GS_REG_SET(packet, GIF_AD, GIF_TAG_FIVE_AD_EOP);
    GS_REG_SET(packet, GS_TEX0_1, texState->tex0Low | ((u64)texState->tex0Clut << 37) | 0x2000000629340000ULL);
    GS_REG_SET(packet, GS_ALPHA_1, texState->alpha);
    GS_REG_SET(packet, GS_TEST_1, 0x3000dULL);
    GS_REG_SET(packet, GS_CLAMP_1, 0);
    GS_REG_SET(packet, GS_TEX1_1, 0);
}

int menuTexCellIndex(int key);
extern TexStateRecord menuTexStateTable[];

/* Emits the record of table menuTexStateTable chosen by key. */
void menuTexStateByKey(GrPacket *packet, s32 key) {
    s32 cellIndex = menuTexCellIndex(key);
    grPacketTexState(packet, menuTexStateTable + cellIndex);
}

extern TexStateRecord menuTexStateAltTable[];

/* The same record chosen by key from the sibling table menuTexStateAltTable (0xA0 further on). */
void menuTexStateAltByKey(void *packet, s32 key) {
    s32 cellIndex = menuTexCellIndex(key);
    grPacketTexState(packet, menuTexStateAltTable + cellIndex);
}

extern TexStateRecord menuSmallCellTexRecord[];

/* Texture state of the small cell sprites (the 24 x 28 grid), one fixed record. */
void menuSmallCellTexState(GrPacket *packet) {
    grPacketTexState(packet, menuSmallCellTexRecord);
}

extern TexStateRecord menuNumTexRecord[];

void menuNumTexState(GrPacket *packet) {
    grPacketTexState(packet, menuNumTexRecord);
}

extern TexStateRecord menuNumStyle1TexRecord[];

void menuNumStyle1TexState(GrPacket *packet) {
    grPacketTexState(packet, menuNumStyle1TexRecord);
}

extern TexStateRecord menuIconTexRecord[];

void menuIconTexState(void *packet) {
    grPacketTexState(packet, menuIconTexRecord);
}

extern TexStateRecord D_00AB6A38[];

void func_00AA8490(void *packet) {
    grPacketTexState(packet, D_00AB6A38);
}

extern TexStateRecord D_00AB6A48[];

void func_00AA84B0(void *packet) {
    grPacketTexState(packet, D_00AB6A48);
}

extern TexStateRecord D_00AB6A58[];

void func_00AA84D0(void *packet) {
    grPacketTexState(packet, D_00AB6A58);
}

extern TexStateRecord D_00AB6A68[];

void func_00AA84F0(void *packet) {
    grPacketTexState(packet, D_00AB6A68);
}

extern TexStateRecord menuPanelTexRecord[];

void menuPanelTexState(GrPacket *packet) {
    grPacketTexState(packet, menuPanelTexRecord);
}

extern TexStateRecord D_00AB6A88[];

void func_00AA8530(GrPacket *packet) {
    grPacketTexState(packet, D_00AB6A88);
}

extern TexStateRecord D_00AB6A98[];

void func_00AA8550(void *packet) {
    grPacketTexState(packet, D_00AB6A98);
}

extern TexStateRecord menuStatusIconsTexRecord[];

void menuStatusIconsTexState(GrPacket *packet) {
    grPacketTexState(packet, menuStatusIconsTexRecord);
}
