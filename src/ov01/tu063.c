/*
 * OV01 original TU 63: 0x00aa8200..0x00aa8590 (17 functions)
 */
#include "common.h"
#include "ov01/tu008.h"
#include "ov01/tu063.h"

INCLUDE_ASM("asm/nonmatchings/ov01/tu063", func_00AA8200);

typedef int s32;

s32 func_001B2120(s32 value);
void grGpInit(void *packet);
void grGsRegSet(void *packet, unsigned long long reg, unsigned long long value);
void grPacketSend(void *packet);

/*
 * Texture flush for the menu: waits on the value of dataStatBufGet, then sends
 * one GS write to register 0x3F (TEXFLUSH in the GS register map) with 1.
 */
void menuTexFlush(void) {
    s32 packet[4];
    s32 result;

    result = dataStatBufGet();
    func_001B2120(result);
    grGpInit(packet);
    grGsRegSet(packet, 0x3F, 1);
    grPacketSend(packet);
}

/*
 * Emits one 16-byte texture state record into the packet: GS TEX0 from the
 * halfwords at +0 and +2, ALPHA from the word at +8, then TEST, CLAMP and TEX1.
 */
INCLUDE_ASM("asm/nonmatchings/ov01/tu063", grPacketTexState);

/* Maps key to a record index: 1..10 gives key-1, 11..15 gives key-4, 16 and up gives 11, 10, 12 or 0 from the low two bits of the halfword at object+0x1C. */
s32 func_00AA8200(s32 key);
s32 grPacketTexState(void *packet, void *record);
extern unsigned char menuTexStateTable[];

/* Emits the record of table menuTexStateTable chosen by key. */
void menuTexStateByKey(void *packet, s32 key) {
    s32 index = func_00AA8200(key);
    grPacketTexState(packet, menuTexStateTable + index * 0x10);
}

extern unsigned char D_00AB6958[];

void func_00AA83D0(void *packet, s32 key) {
    s32 index = func_00AA8200(key);
    grPacketTexState(packet, D_00AB6958 + index * 0x10);
}

extern unsigned char D_00AB69F8[];

void func_00AA8410(void *packet) {
    grPacketTexState(packet, D_00AB69F8);
}

extern unsigned char D_00AB6A08[];

void func_00AA8430(void *packet) {
    grPacketTexState(packet, D_00AB6A08);
}

extern unsigned char D_00AB6A18[];

void func_00AA8450(void *packet) {
    grPacketTexState(packet, D_00AB6A18);
}

extern unsigned char menuIconTexRecord[];

void menuIconTexState(void *packet) {
    grPacketTexState(packet, menuIconTexRecord);
}

extern unsigned char D_00AB6A38[];

void func_00AA8490(void *packet) {
    grPacketTexState(packet, D_00AB6A38);
}

extern unsigned char D_00AB6A48[];

void func_00AA84B0(void *packet) {
    grPacketTexState(packet, D_00AB6A48);
}

extern unsigned char D_00AB6A58[];

void func_00AA84D0(void *packet) {
    grPacketTexState(packet, D_00AB6A58);
}

extern unsigned char D_00AB6A68[];

void func_00AA84F0(void *packet) {
    grPacketTexState(packet, D_00AB6A68);
}

extern unsigned char menuPanelTexRecord[];

void menuPanelTexState(void *packet) {
    grPacketTexState(packet, menuPanelTexRecord);
}

extern unsigned char D_00AB6A88[];

void func_00AA8530(void *packet) {
    grPacketTexState(packet, D_00AB6A88);
}

extern unsigned char D_00AB6A98[];

void func_00AA8550(void *packet) {
    grPacketTexState(packet, D_00AB6A98);
}

extern unsigned char menuStatusIconsTexRecord[];

void menuStatusIconsTexState(void *packet) {
    grPacketTexState(packet, menuStatusIconsTexRecord);
}
