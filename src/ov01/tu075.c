#include "common.h"
#include "shared.h"
#include "main/tu165.h"
#include "main/tu197.h"
#include "ov01/tu075.h"
/* EE scratchpad RAM. */
#define SCRATCHPAD_BASE 0x70000000
/* Default packet capacity used when no custom buffer is supplied. */
#define MGS_PACKET_DEFAULT_CAPACITY 0x400

/*
 * The GS direct-transfer packet MGsGPInit prepares and MGsGPTerm flushes.
 * By default it targets the scratchpad with a fixed capacity; a caller that
 * supplies its own buffer address also supplies that buffer's capacity
 * (MGsGPInit: movn on both the address and the size against the same
 * condition register).
 *
 *   data      the transfer base address: MGsGPInit's default/custom value,
 *             and the address MGsGPTerm passes to sceVif1PkAddDirectDataN.
 *   capacity  the buffer's byte capacity, set by MGsGPInit; no function in
 *             this TU reads it back.
 *   current   reset to data by both MGsGPInit and MGsGPTerm; the code that
 *             advances it while queuing packet contents is outside this TU.
 *   count     the queued transfer size, passed to sceVif1PkAddDirectDataN
 *             and reset to zero after each flush.
 *   auxCount  reset to zero alongside count by both functions; no further
 *             access is evidenced in this TU.
 */
typedef struct MGsPacket {
    void *data;      /* +0x00 */
    int capacity;    /* +0x04 */
    void *current;   /* +0x08 */
    int count;       /* +0x0C */
    int auxCount;    /* +0x10 */
} MGsPacket;

void MMathRandomSeed();
int sefResetWorkTables();

/* Scene state reset (proposed ov01SceneStateReset), called by ov01SceneInit:
 * zeroes ov01LastValue and ov01TickCount, then runs the two init calls. */
void ov01SceneStateReset(void) {
    extern s16 ov01LastValue;
    extern int ov01TickCount;
    extern int D_00AED084;

    sefStartWithDefaultMemory();
    sefStartClipTestDisabled();
    ov01LastValue = 0;
    ov01TickCount = 0;
    D_00AED084 = 0;
    MMathRandomSeed(0);
    MSeObjInit();
}

void ov01SceneSystemsShutdown(void) {
    MSeObjShutdown();
    sefResetWorkTables();
}

/* Records value as the last value and counts one tick (proposed ov01LastValueSet). */
void ov01LastValueSet(s16 value) {
    extern s16 ov01LastValue;
    extern int ov01TickCount;

    ov01LastValue = value;
    ov01TickCount += 1;
}

/* Last value recorded by ov01LastValueSet (proposed ov01LastValueGet). */
s16 ov01LastValueGet(void) {
    extern s16 ov01LastValue;

    return ov01LastValue;
}

/* Ticks since the last reset (proposed ov01TickCountGet). */
int ov01TickCountGet(void) {
    extern int ov01TickCount;

    return ov01TickCount;
}

void MGsGPInit(MGsPacket *packet, void *address, int size)
{
    void *base = (address != 0) ? address : (void *)SCRATCHPAD_BASE;

    packet->data = base;
    packet->capacity = (address != 0) ? size : MGS_PACKET_DEFAULT_CAPACITY;
    packet->current = base;
    packet->count = 0;
    packet->auxCount = 0;
}

void func_0012B9A0();
void sceVif1PkAddDirectDataN();
void sceVif1PkCloseDirectHLCode();
void sceVif1PkCnt();
void sceVif1PkOpenDirectHLCode();
void *xglPacketGetCurrent(void);

/* Flushes the packet that MGsGPInit prepares (proposed MGsGPTerm; X1 twin in src/ov01/m_gs.c). */
void MGsGPTerm(MGsPacket *packet) {
    void *vifPacket;

    if (packet->count == 0) {
        packet->current = packet->data;
        packet->auxCount = 0;
        return;
    }
    vifPacket = xglPacketGetCurrent();
    sceVif1PkCnt(vifPacket, 0);
    func_0012B9A0(vifPacket, 0x11000000);
    sceVif1PkOpenDirectHLCode(vifPacket, 0);
    sceVif1PkAddDirectDataN(vifPacket, packet->data, packet->count);
    sceVif1PkCloseDirectHLCode(vifPacket);
    packet->current = packet->data;
    packet->count = 0;
    packet->auxCount = 0;
}

void *memset(void *dest, int value, unsigned int size);

void func_00AAAEC8(void *buffer) {
    memset(buffer, 0, 0x40);
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu075", MSeFire);
