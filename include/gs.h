#ifndef INCLUDE_GS_H
#define INCLUDE_GS_H

#include "shared.h"

/* GS register numbers and register values (the GS manual's field layout),
 * for A+D packets built by game code. */

#define GS_PRIM 0x00
#define GS_RGBAQ 0x01
#define GS_UV 0x03
#define GS_XYZF2 0x04
#define GS_TEX0_1 0x06
#define GS_CLAMP_1 0x08
#define GS_XYZ3 0x0C
#define GS_TEX1_1 0x14
#define GS_TEXA 0x3B
#define GS_TEXFLUSH 0x3F
#define GS_SCISSOR_1 0x40
#define GS_ALPHA_1 0x42
#define GS_COLCLAMP 0x46
#define GS_TEST_1 0x47
#define GS_PABE 0x49
#define GS_FBA_1 0x4A
#define GS_FRAME_1 0x4C
#define GS_FRAME_2 0x4D
#define GS_ZBUF_1 0x4E

/* GIFtag register descriptor of an A+D (address plus data) register. */
#define GIF_AD 0x0E

/* Appends one A+D register write (value, then register address) at the u64 cursor p. */
#define GS_PUT(p, reg, value) ((p)[0] = (value), (p)[1] = (reg), (p) += 2)

#define GS_SET_PRIM(prim, iip, tme, fge, abe, aa1, fst, ctxt, fix) \
    ((u64)(prim) | ((u64)(iip) << 3) | ((u64)(tme) << 4) | ((u64)(fge) << 5) | ((u64)(abe) << 6) \
     | ((u64)(aa1) << 7) | ((u64)(fst) << 8) | ((u64)(ctxt) << 9) | ((u64)(fix) << 10))
#define GS_SET_RGBAQ(r, g, b, a, q) \
    ((u64)(r) | ((u64)(g) << 8) | ((u64)(b) << 16) | ((u64)(a) << 24) | ((u64)(q) << 32))
#define GS_SET_UV(u, v) ((u64)(u) | ((u64)(v) << 16))
#define GS_SET_XYZ(x, y, z) ((u64)(x) | ((u64)(y) << 16) | ((u64)(z) << 32))
#define GS_SET_TEX0(tbp, tbw, psm, tw, th, tcc, tfx, cbp, cpsm, csm, csa, cld) \
    ((u64)(tbp) | ((u64)(tbw) << 14) | ((u64)(psm) << 20) | ((u64)(tw) << 26) | ((u64)(th) << 30) \
     | ((u64)(tcc) << 34) | ((u64)(tfx) << 35) | ((u64)(cbp) << 37) | ((u64)(cpsm) << 51) \
     | ((u64)(csm) << 55) | ((u64)(csa) << 56) | ((u64)(cld) << 61))
#define GS_SET_TEX1(lcm, mxl, mmag, mmin, mtba, l, k) \
    ((u64)(lcm) | ((u64)(mxl) << 2) | ((u64)(mmag) << 5) | ((u64)(mmin) << 6) | ((u64)(mtba) << 9) \
     | ((u64)(l) << 19) | ((u64)(k) << 32))
#define GS_SET_CLAMP(wms, wmt, minu, maxu, minv, maxv) \
    ((u64)(wms) | ((u64)(wmt) << 2) | ((u64)(minu) << 4) | ((u64)(maxu) << 14) | ((u64)(minv) << 24) \
     | ((u64)(maxv) << 34))
#define GS_SET_SCISSOR(x0, x1, y0, y1) ((u64)(x0) | ((u64)(x1) << 16) | ((u64)(y0) << 32) | ((u64)(y1) << 48))
#define GS_SET_ALPHA(a, b, c, d, fix) \
    ((u64)(a) | ((u64)(b) << 2) | ((u64)(c) << 4) | ((u64)(d) << 6) | ((u64)(fix) << 32))
#define GS_SET_TEST(ate, atst, aref, afail, date, datm, zte, ztst) \
    ((u64)(ate) | ((u64)(atst) << 1) | ((u64)(aref) << 4) | ((u64)(afail) << 12) | ((u64)(date) << 14) \
     | ((u64)(datm) << 15) | ((u64)(zte) << 16) | ((u64)(ztst) << 17))
#define GS_SET_FRAME(fbp, fbw, psm, fbmsk) \
    ((u64)(fbp) | ((u64)(fbw) << 16) | ((u64)(psm) << 24) | ((u64)(fbmsk) << 32))
#define GS_SET_ZBUF(zbp, psm, zmsk) ((u64)(zbp) | ((u64)(psm) << 24) | ((u64)(zmsk) << 32))

/* Pixel storage modes. */
#define GS_PSMCT32 0x00
#define GS_PSMT8 0x13
#define GS_PSMT4 0x14

/* CLAMP wrap modes. */
#define GS_REGION_CLAMP 2
#define GS_REGION_REPEAT 3

#endif /* INCLUDE_GS_H */
