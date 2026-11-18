#ifndef INCLUDE_OV01_TU062_H
#define INCLUDE_OV01_TU062_H

/* Functions of src/ov01/tu062.c that other files call. */
void menuPanelCreate(void);
/* Opens a stat panel of kind. */
void menuStatPanelNew(int kind);

struct GrPacket;
struct Ov01Object;

/* Draws the face of the unit with key `key`; small picks the smaller of the two frames. */
void func_00AA7750(struct GrPacket *packet, int x, int y, short key, int small);
/* A sprite of width x height at (x, y) from the texel rectangle at (srcX, srcY) of the same size. */
int menuStatSprite(struct GrPacket *packet, int x, int y, int width, int height, int srcX, int srcY);
/* A sprite of width x height at (x, y) from the texel rectangle (srcX, srcY)-(srcRight, srcBottom), which the
 * gauges keep at full size while the width shrinks. */
void menuStatSprite2(struct GrPacket *packet, int x, int y, int width, int height, int srcX, int srcY, int srcRight,
                     int srcBottom);
void menuNumDraw(struct GrPacket *packet, int x, int y, int value, int digits, int style);
void menuNumDrawTarget(struct GrPacket *packet, int x, int y, int value, int digits, int style,
                       struct Ov01Object *target);
void func_00AA7A48(struct GrPacket *packet, int x, int y, int value, int digits, int style, struct Ov01Object *target);

#endif /* INCLUDE_OV01_TU062_H */
