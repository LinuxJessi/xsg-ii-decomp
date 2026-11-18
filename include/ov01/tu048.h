#ifndef INCLUDE_OV01_TU048_H
#define INCLUDE_OV01_TU048_H

/* The draw packet functions of src/ov01/tu048.c that other files call. */

/* grGpInit's argument: the VIF1 DIRECT packet grPacketSend transmits, a data buffer and its length. grGpInit
 * points data at the scratchpad (0x70000000). A local packet must be the whole record, because grGpInit writes
 * both words. */
typedef struct GrPacket {
    void *data;  /* +0x0 */
    int count;   /* +0x4 */
} GrPacket;

void grGpInit(GrPacket *packet);
void grPacketSend(GrPacket *packet);
void grPutSprOpen(GrPacket *packet);
void grPutSprClose(void);

#endif /* INCLUDE_OV01_TU048_H */
