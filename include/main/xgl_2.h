#ifndef INCLUDE_MAIN_XGL_2_H
#define INCLUDE_MAIN_XGL_2_H

#include "shared.h"

/* A vector kept as a plain float[4] (like sceVu0FVECTOR); its w is element 3. Vector4 here changes the
 * address arithmetic of the w stores, so the casts to Vector4 * at the calls stay. */
typedef float XglFVector[4];

/* Light set built by xglLightPackBuildLightSet, cleared with xglZeroQuadwords18 (0x18 quadwords). */
typedef struct XglLightSet {
    XglFVector directions[6]; /* 0x000 */
    Vector4 viewDirection;  /* 0x060 */
    XglFVector colors[6];   /* 0x070 */
    Vector4 ambientColor;       /* 0x0D0 */
    Vector4 ambientColorLower;   /* 0x0E0 */
    Vector4 forward;        /* 0x0F0 */
    Vector4 reserved[2];    /* 0x100 */ 
    XglFVector points[3];   /* 0x120 */
    XglFVector pointDirections[3]; /* 0x150 */
} XglLightSet;

/* The copies of the light set tu156 keeps are 16-byte aligned (their copies use ld/sd without an
 * alignment test). */
typedef XglLightSet AlignedXglLightSet __attribute__((aligned(16)));

/* Functions of src/main/xgl_2.c (the whole xgl_2 file) that other files call. */

unsigned short xglSRand(void);
void xglLightPackCacheInvalidate(void);
void xglMatrixStackUnit(void);

#endif /* INCLUDE_MAIN_XGL_2_H */
