#ifndef INCLUDE_OV01_TU063_H
#define INCLUDE_OV01_TU063_H

#include "ov01/tu048.h"

/* Functions of src/ov01/tu063.c that other files call. */

/* Texture flush for the menu (waits on dataStatBufGet, then one GS_TEXFLUSH write). */
void menuTexFlush(void);
/* Emits one 16-byte texture state record into the packet. */
void grPacketTexState(GrPacket *packet, void *record);
void menuTexStateByKey(GrPacket *packet, int key);
void menuSmallCellTexState(GrPacket *packet);
void menuNumTexState(GrPacket *packet);
void menuNumStyle1TexState(GrPacket *packet);
void menuPanelTexState(GrPacket *packet);
void menuStatusIconsTexState(GrPacket *packet);
/* Texture state of a function of the number at 0xAB6A88 (called from src/ov01/tu054.c and tu055.c). */
void func_00AA8530(GrPacket *packet);

#endif /* INCLUDE_OV01_TU063_H */
