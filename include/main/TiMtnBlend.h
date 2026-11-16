#ifndef INCLUDE_MAIN_TIMTNBLEND_H
#define INCLUDE_MAIN_TIMTNBLEND_H

/* Functions of src/main/TiMtnBlend.c that other files call. */

struct TiMtnBlendContext;

void tiMtnBlendSwapLocal(struct TiMtnBlendContext *ctx, int *flags);
void tiMtnBlendLerpMatrix(float (*dst)[4], float (*a)[4], float (*b)[4], float t);
void tiMtnBlendUpdateGlobal(struct TiMtnBlendContext *ctx);
void tiMtnBlendUpdateLocal(struct TiMtnBlendContext *ctx);

#endif /* INCLUDE_MAIN_TIMTNBLEND_H */
