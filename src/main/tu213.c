#include "common.h"
#include "main/sdv_033.h"
#include "main/tu213.h"
#include "gs.h"

/* One queued GS register write: the data word, then the register address word. */
typedef struct GsQueueQword {
    u64 data;
    u64 reg;
} GsQueueQword;

extern long long *gsQueueBase;
extern int gsQueueCount;
extern void (*gsQueueBeginHandler)(GsQueueContext *context);
u64 gsMakeFrame(int buffer, int flag);

/* Qword i of the queue buffer. The base pointer is read through a GsQueueQword ** view of gsQueueBase: that typed
   read does not alias the u64 queue stores, which lets the scheduler place the base load as the original does. */
#define GS_QUEUE_BASE (*(GsQueueQword **)&gsQueueBase)
#define GS_QUEUE_AT(i) (GS_QUEUE_BASE[i])

/* gsQueueCount read and written through an unsigned view: like GS_QUEUE_BASE this gives the access its own alias
   set, which lets the scheduler move the gsQueueBase store ahead of it as the original does. */
#define GS_QUEUE_INDEX (*(unsigned int *)&gsQueueCount)

/* A batch starts in the scratchpad (SCRATCHPAD_BASE) with a GIF tag qword: the tag (NLOOP 0, patched in by
   gsQueueFlush, EOP set, NREG 1) in the low dword and its register list (GIF_AD) in the high dword. */
#define GS_QUEUE_SCRATCHPAD ((GsQueueQword *)SCRATCHPAD_BASE)
#define GS_QUEUE_GIFTAG 0x1000000000008000ULL

/* Qword count at which the kick-vertex forms flush the queue (901). */
#define GS_QUEUE_FLUSH_LIMIT 901

/* PRIM values of the triangle-strip batch: tristrip, Gouraud, texture mapping, alpha blend, UV coordinates (FST). */
#define GS_QUEUE_PRIM_TRISTRIP_TEXTURED 0x15C
#define GS_QUEUE_PRIM_TRISTRIP 0x14C

/* A queue qword viewed as two u64 words. An access through a union may alias anything, so these stores keep
   their order against the count and base accesses (the GIF tag qword, the TEX1 register word) as in the original. */
typedef union GsQueueWords {
    GsQueueQword qw;
    u64 word[2];
} GsQueueWords;

extern int gsQueueTextured;
void gsQueueSetFrame(int buffer);
void gsQueueSetTest(void);
void gsQueueSetTexA(void);
void gsQueueSetTex0(int value);
void gsQueueSetBlendColor(int kind, unsigned int color, int halveRgb);

/* The direct GS queue: each gsQueue emitter appends one qword (GS register address in the high word, data in the
   low word) at gsQueueBase[gsQueueCount]. gsQueueFlush sends the queued qwords through the VIF1 direct-HL path. */

/* Far depth (the largest 24-bit Z) for the screen quads below. */

/* gsQuadHalfScreenFill: fills a four-vertex screen quad (x, y, z per vertex) for kind 0 or 1. */
#define GS_QUAD_FAR_Z 16777215.0f

void gsQuadHalfScreenFill(int kind, float quad[][4])
{
    if (kind < 2U) {
        switch (kind) {
        case 0:
            quad[0][0] = 0.0f;
            quad[0][1] = 224.0f;
            quad[0][2] = GS_QUAD_FAR_Z;
            quad[1][0] = 128.0f;
            quad[1][1] = 0.0f;
            quad[1][2] = GS_QUAD_FAR_Z;
            quad[2][0] = 128.0f;
            quad[2][1] = 448.0f;
            quad[2][2] = GS_QUAD_FAR_Z;
            quad[3][0] = 256.0f;
            quad[3][1] = 224.0f;
            quad[3][2] = GS_QUAD_FAR_Z;
            break;
        case 1:
            quad[0][0] = 384.0f;
            quad[0][1] = 0.0f;
            quad[0][2] = GS_QUAD_FAR_Z;
            quad[1][0] = 512.0f;
            quad[1][1] = 224.0f;
            quad[1][2] = GS_QUAD_FAR_Z;
            quad[2][0] = 256.0f;
            quad[2][1] = 224.0f;
            quad[2][2] = GS_QUAD_FAR_Z;
            quad[3][0] = 384.0f;
            quad[3][1] = 448.0f;
            quad[3][2] = GS_QUAD_FAR_Z;
            break;
        }
    }
}

/* gsQueueBeginTriStrip: starts a batch in the scratchpad packet (GIF tag, FRAME, TEST, texture state, blend,
   PRIM type 4, TEXFLUSH) and stores itself as the begin handler. */
void gsQueueBeginTriStrip(GsQueueContext *context) {
    int textured = context->textured;
    int frame = context->frame;
    int tex0 = context->tex0;
    int color = context->color;
    int blendKind = context->blend_kind;
    GsQueueQword *queue = GS_QUEUE_SCRATCHPAD;

    gsQueueCount = 0;
    ((GsQueueWords *)queue)->word[0] = GS_QUEUE_GIFTAG;
    GS_QUEUE_BASE = queue;
    gsQueueTextured = textured;
    ((GsQueueWords *)queue)[GS_QUEUE_INDEX++].word[1] = GIF_AD;
    queue[gsQueueCount].data = 0;
    queue[gsQueueCount++].reg = GS_TEXFLUSH;
    gsQueueSetFrame(frame);
    gsQueueSetTest();
    if (textured) {
        gsQueueSetTexA();
        gsQueueSetTex0(tex0);
    }
    gsQueueSetBlendColor(blendKind, color, textured);
    if (textured) {
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_PRIM(4, 1, 1, 0, 1, 0, 1, 0, 0);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_PRIM;
    } else {
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_PRIM(4, 1, 0, 0, 1, 0, 1, 0, 0);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_PRIM;
    }
    GS_QUEUE_AT(gsQueueCount).data = 0;
    GS_QUEUE_AT(gsQueueCount++).reg = GS_TEXFLUSH;
    gsQueueBeginHandler = gsQueueBeginTriStrip;
}

/* gsQueueAddPrimTriStrip: PRIM for a triangle strip, textured when the argument is nonzero. */
void gsQueueAddPrimTriStrip(int textured) {
    if (textured) {
        GS_QUEUE_AT(gsQueueCount).data = GS_QUEUE_PRIM_TRISTRIP_TEXTURED;
        GS_QUEUE_AT(gsQueueCount++).reg = GS_PRIM;
    } else {
        GS_QUEUE_AT(gsQueueCount).data = GS_QUEUE_PRIM_TRISTRIP;
        GS_QUEUE_AT(gsQueueCount++).reg = GS_PRIM;
    }
}

/* gsQueueBeginTriFan: starts a triangle fan batch (PRIM type 5) in the scratchpad packet: GIF tag, TEXFLUSH, FRAME, TEST,
   texture state when textured, PRIM, blend state; stores itself as the begin handler. */
void gsQueueBeginTriFan(GsQueueContext *context) {
    int textured = context->textured;
    int frame = context->frame;
    int tex0 = context->tex0;
    int color = context->color;
    int blendKind = context->blend_kind;
    GsQueueQword *queue = GS_QUEUE_SCRATCHPAD;

    gsQueueCount = 0;
    ((GsQueueWords *)queue)->word[0] = GS_QUEUE_GIFTAG;
    GS_QUEUE_BASE = queue;
    gsQueueTextured = textured;
    ((GsQueueWords *)queue)[GS_QUEUE_INDEX++].word[1] = GIF_AD;
    queue[gsQueueCount].data = 0;
    queue[gsQueueCount++].reg = GS_TEXFLUSH;
    gsQueueSetFrame(frame);
    gsQueueSetTest();
    if (textured) {
        gsQueueSetTexA();
        gsQueueSetTex0(tex0);
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_PRIM(5, 1, 1, 0, 1, 0, 1, 0, 0);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_PRIM;
    } else {
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_PRIM(5, 1, 0, 0, 1, 0, 1, 0, 0);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_PRIM;
    }
    gsQueueSetBlendColor(blendKind, color, textured);
    gsQueueBeginHandler = gsQueueBeginTriFan;
}

/* gsQueueBeginLines: starts a line batch (PRIM type 1) in the scratchpad packet: GIF tag, TEXFLUSH, FRAME, TEST,
   texture state when textured, PRIM, blend state; stores itself as the begin handler. */
void gsQueueBeginLines(GsQueueContext *context) {
    int textured = context->textured;
    int frame = context->frame;
    int tex0 = context->tex0;
    int color = context->color;
    int blendKind = context->blend_kind;
    GsQueueQword *queue = GS_QUEUE_SCRATCHPAD;

    gsQueueCount = 0;
    ((GsQueueWords *)queue)->word[0] = GS_QUEUE_GIFTAG;
    GS_QUEUE_BASE = queue;
    gsQueueTextured = textured;
    ((GsQueueWords *)queue)[GS_QUEUE_INDEX++].word[1] = GIF_AD;
    queue[gsQueueCount].data = 0;
    queue[gsQueueCount++].reg = GS_TEXFLUSH;
    gsQueueSetFrame(frame);
    gsQueueSetTest();
    if (textured) {
        gsQueueSetTexA();
        gsQueueSetTex0(tex0);
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_PRIM(1, 1, 1, 0, 1, 0, 1, 0, 0);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_PRIM;
    } else {
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_PRIM(1, 1, 0, 0, 1, 0, 1, 0, 0);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_PRIM;
    }
    gsQueueSetBlendColor(blendKind, color, textured);
    gsQueueBeginHandler = gsQueueBeginLines;
}

/* gsQueueBeginSprites: starts a sprite batch (PRIM type 6) in the scratchpad packet: GIF tag, TEXFLUSH, FRAME, TEST,
   texture state when textured, PRIM, blend state; stores itself as the begin handler. */
void gsQueueBeginSprites(GsQueueContext *context) {
    int textured = context->textured;
    int frame = context->frame;
    int tex0 = context->tex0;
    int color = context->color;
    int blendKind = context->blend_kind;
    GsQueueQword *queue = GS_QUEUE_SCRATCHPAD;

    gsQueueCount = 0;
    ((GsQueueWords *)queue)->word[0] = GS_QUEUE_GIFTAG;
    GS_QUEUE_BASE = queue;
    gsQueueTextured = textured;
    ((GsQueueWords *)queue)[GS_QUEUE_INDEX++].word[1] = GIF_AD;
    queue[gsQueueCount].data = 0;
    queue[gsQueueCount++].reg = GS_TEXFLUSH;
    gsQueueSetFrame(frame);
    gsQueueSetTest();
    if (textured) {
        gsQueueSetTexA();
        gsQueueSetTex0(tex0);
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_PRIM(6, 1, 1, 0, 1, 0, 1, 0, 0);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_PRIM;
    } else {
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_PRIM(6, 1, 0, 0, 1, 0, 1, 0, 0);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_PRIM;
    }
    gsQueueSetBlendColor(blendKind, color, textured);
    gsQueueBeginHandler = gsQueueBeginSprites;
}

/* gsQueueAddTexturedQuad: four vertices, UV and XYZ qwords; the second row carries the kick.
   The rows are addressed as *(uv + n) / *(pos + n): the uv[n] / pos[n] / &uv[n][0] / n-index-variable forms compile
   to different operand order (2 to 4 differing rows in this function). */
void gsQueueAddTexturedQuad(XglPacket *packet, float pos[][4], float uv[][4], GsQueueContext *context)
{
    int row, col;
    for (row = 0; row < 2; row++) {
        for (col = 0; col < 2; col++) {
            float *u = *(uv + row * 2 + col);
            GS_QUEUE_AT(gsQueueCount).data = (u64)(u[0] * 16.0f) | ((u64)(u[1] * 16.0f) << 16);
            GS_QUEUE_AT(gsQueueCount++).reg = GS_UV;
            if (row == 1) {
                float *p = *(pos + row * 2 + col);
                GS_QUEUE_AT(gsQueueCount).data = (u64)(int)((p[0] + 1792.0f) * 16.0f) | ((u64)(int)((p[1] + 1824.0f) * 16.0f) << 16) | ((u64)p[2] << 32);
                GS_QUEUE_AT(gsQueueCount++).reg = GS_XYZF2;
            } else {
                float *p = *(pos + row * 2 + col);
                GS_QUEUE_AT(gsQueueCount).data = (u64)(int)((p[0] + 1792.0f) * 16.0f) | ((u64)(int)((p[1] + 1824.0f) * 16.0f) << 16) | ((u64)p[2] << 32);
                GS_QUEUE_AT(gsQueueCount++).reg = GS_XYZ3;
            }
        }
    }
    if (gsQueueCount >= GS_QUEUE_FLUSH_LIMIT) {
        gsQueueFlush(packet);
        gsQueueRestart(context);
    }
}

/* gsQueueAddTriangle: queues a textured triangle, UV then XYZ for each of the three vertices; the third XYZ uses
   XYZF2 (drawing kick), the first two XYZF3 (no kick). Flushes and restarts the batch when the queue is full.
   The vertices are addressed as a byte offset plus the array base, and the positions are read through the
   DoublewordVector4 union view: plain &uv[i] / &pos[i] indexing compiles to a different operand order
   (4 differing rows), so the byte-offset form is kept. */
void gsQueueAddTriangle(XglPacket *packet, DoublewordVector4 *pos, Vector4 *uv, GsQueueContext *context)
{
    int i;
    for (i = 0; i < 3; i++) {
        int offset = i * sizeof(Vector4);
        Vector4 *texCoord = (Vector4 *)(offset + (int)uv);
        GS_QUEUE_AT(gsQueueCount).data = (u64)(texCoord->x * 16.0f) | ((u64)(texCoord->y * 16.0f) << 16);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_UV;
        {
            DoublewordVector4 *vertex = (DoublewordVector4 *)(offset + (int)pos);
            GS_QUEUE_AT(gsQueueCount).data = (u64)(int)((vertex->vector.x + 1792.0f) * 16.0f)
                | ((u64)(int)((vertex->vector.y + 1824.0f) * 16.0f) << 16) | ((u64)vertex->vector.z << 32);
        }
        if (i == 2) {
            GS_QUEUE_AT(gsQueueCount++).reg = GS_XYZF2;
        } else {
            GS_QUEUE_AT(gsQueueCount++).reg = GS_XYZ3;
        }
    }
    if (gsQueueCount >= GS_QUEUE_FLUSH_LIMIT) {
        gsQueueFlush(packet);
        gsQueueRestart(context);
    }
}

/* gsQueueBeginPoints: starts a point batch (PRIM type 0): GIF tag, FRAME, TEST (depth test, alpha test off),
   TEX0 and TEX1 when textured, PRIM, blend state; stores itself as the begin handler. */
void gsQueueBeginPoints(GsQueueContext *context) {
    int frame = context->frame;
    int tex0 = context->tex0;
    int textured = context->textured;
    int color = context->color;
    int blendKind = context->blend_kind;

    GsQueueQword *queue = GS_QUEUE_SCRATCHPAD;

    gsQueueCount = 0;
    ((GsQueueWords *)queue)->word[0] = GS_QUEUE_GIFTAG;
    GS_QUEUE_BASE = queue;
    gsQueueTextured = textured;
    ((GsQueueWords *)queue)[GS_QUEUE_INDEX++].word[1] = GIF_AD;
    gsQueueSetFrame(frame);
    ((GsQueueQword *)gsQueueBase)[gsQueueCount].data = GS_SET_TEST(0, 0, 0, 1, 0, 0, 1, 3);
    ((GsQueueQword *)gsQueueBase)[gsQueueCount++].reg = GS_TEST_1;
    if (textured) {
        gsQueueSetTex0(tex0);
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_TEX1(1, 0, 1, 1, 0, 0, 0);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_TEX1_1;
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_PRIM(0, 1, 1, 0, 1, 0, 1, 0, 0);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_PRIM;
    } else {
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_PRIM(0, 1, 0, 0, 1, 0, 1, 0, 0);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_PRIM;
    }
    gsQueueSetBlendColor(blendKind, color, textured);
    gsQueueBeginHandler = gsQueueBeginPoints;
}

/* gsQueueAddTexturedKickVertex: one kick vertex, UV first when textured, flush at 901 qwords (GS_QUEUE_FLUSH_LIMIT). */
void gsQueueAddTexturedKickVertex(XglPacket *packet, float *pos, float *uv, GsQueueContext *context)
{
    if (gsQueueTextured) {
        GS_QUEUE_AT(gsQueueCount).data = (u64)(uv[0] * 16.0f) | ((u64)(uv[1] * 16.0f) << 16);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_UV;
    }
    ((GsQueueQword *)gsQueueBase)[gsQueueCount].data = (u64)(int)((pos[0] + 1792.0f) * 16.0f) | ((u64)(int)((pos[1] + 1824.0f) * 16.0f) << 16) | ((u64)pos[2] << 32);
    ((GsQueueQword *)gsQueueBase)[gsQueueCount++].reg = GS_XYZF2;
    if (gsQueueCount >= GS_QUEUE_FLUSH_LIMIT) {
        gsQueueFlush(packet);
        gsQueueRestart(context);
    }
}

/* gsQueueAddTexturedVertex: one vertex without kick, UV first when textured. */
void gsQueueAddTexturedVertex(XglPacket *unused, float *pos, float *uv, GsQueueContext *context)
{
    if (gsQueueTextured) {
        GS_QUEUE_AT(gsQueueCount).data = (u64)(uv[0] * 16.0f) | ((u64)(uv[1] * 16.0f) << 16);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_UV;
    }
    ((GsQueueQword *)gsQueueBase)[gsQueueCount].data = (u64)(int)((pos[0] + 1792.0f) * 16.0f) | ((u64)(int)((pos[1] + 1824.0f) * 16.0f) << 16) | ((u64)pos[2] << 32);
    ((GsQueueQword *)gsQueueBase)[gsQueueCount++].reg = GS_XYZ3;
}

/* gsQueueAddKickVertex: one untextured kick vertex; flushes and restarts the batch when the queue is full. */
void gsQueueAddKickVertex(XglPacket *packet, float *pos, GsQueueContext *context)
{
    int x = (pos[0] + 1792.0f) * 16.0f;
    int y = (pos[1] + 1824.0f) * 16.0f;

    GS_QUEUE_AT(gsQueueCount).data = (u64)x | ((u64)y << 16) | ((u64)pos[2] << 32);
    GS_QUEUE_AT(gsQueueCount++).reg = GS_XYZF2;
    if (gsQueueCount >= GS_QUEUE_FLUSH_LIMIT) {
        gsQueueFlush(packet);
        gsQueueRestart(context);
    }
}

/* gsQueueAddVertex: one untextured vertex without kick. */
void gsQueueAddVertex(XglPacket *unused, float *pos, GsQueueContext *batch)
{
    int x = (pos[0] + 1792.0f) * 16.0f;
    int y = (pos[1] + 1824.0f) * 16.0f;
    GS_QUEUE_AT(gsQueueCount).data = (u64)x | ((u64)y << 16) | ((u64)pos[2] << 32);
    GS_QUEUE_AT(gsQueueCount++).reg = GS_XYZ3;
}


void gsQueueAddRgbaq(unsigned int color, int halveRgb);

/* gsQueueAddColor: appends an RGBAQ color; at 901 qwords it flushes and restarts the batch. */
void gsQueueAddColor(XglPacket *packet, int color, GsQueueContext *context) {
    gsQueueAddRgbaq(color, context->textured);
    if (gsQueueCount >= GS_QUEUE_FLUSH_LIMIT) {
        gsQueueFlush(packet);
        gsQueueRestart(context);
    }
}

/* gsQueueAddContextColor: as gsQueueAddColor, the color is taken from the context. */
void gsQueueAddContextColor(XglPacket *packet, GsQueueContext *context) {
    gsQueueAddRgbaq(context->color, context->textured);
    if (gsQueueCount >= GS_QUEUE_FLUSH_LIMIT) {
        gsQueueFlush(packet);
        gsQueueRestart(context);
    }
}

/* gsQueueRestart: calls the batch-begin handler stored in gsQueueBeginHandler. */
void gsQueueRestart(GsQueueContext *context) {
    gsQueueBeginHandler(context);
}

/* SCE packet builders (no shared header: sdv_033.c declares them with an unsigned count, tu001/tu127/game_defocus with an int one; these are the forms tu213 calls with). */
extern void sceVif1PkCnt(XglPacket *packet, int count);
extern void sceVif1PkOpenDirectHLCode(XglPacket *packet, int mode);
extern void sceVif1PkAddDirectDataN(XglPacket *packet, const void *data, int count);
extern void sceVif1PkCloseDirectHLCode(XglPacket *packet);

/* gsQueueFlush: the first qword counts the queued qwords, then the packet is sent in direct-HL mode. */
void gsQueueFlush(XglPacket *packet) {
    *gsQueueBase += gsQueueCount - 1;
    sceVif1PkCnt(packet, 0);
    sceVif1PkOpenDirectHLCode(packet, 0);
    sceVif1PkAddDirectDataN(packet, gsQueueBase, gsQueueCount);
    sceVif1PkCloseDirectHLCode(packet);
}

/* gsQueueSetFrame: appends the FRAME_1 register (GS_FRAME_1). */
void gsQueueSetFrame(int buffer) {
    u64 frame = gsMakeFrame(buffer, 0);

    GS_QUEUE_AT(gsQueueCount).data = frame;
    GS_QUEUE_AT(gsQueueCount++).reg = GS_FRAME_1;
}

int gsQueueTex0Value(int value);

/* gsQueueSetTex0: appends the TEX0_1 register (0x06). */
void gsQueueSetTex0(int value) {
    int tex0 = gsQueueTex0Value(value);

    GS_QUEUE_AT(gsQueueCount).data = tex0;
    GS_QUEUE_AT(gsQueueCount++).reg = GS_TEX0_1;
}

/* gsQueueSetBlendColor: appends ALPHA_1 for the blend kind, then gsQueueAddRgbaq. */
void gsQueueSetBlendColor(int kind, unsigned int color, int halveRgb) {
    switch (kind) {
    case 1:
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_ALPHA(0, 2, 0, 1, 0);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_ALPHA_1;
        break;
    case 2:
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_ALPHA(2, 0, 0, 1, 0);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_ALPHA_1;
        break;
    case 3:
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_ALPHA(0, 1, 0, 2, 0);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_ALPHA_1;
        break;
    case 0:
    default:
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_ALPHA(0, 1, 0, 1, 0);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_ALPHA_1;
        break;
    }
    gsQueueAddRgbaq(color, halveRgb);
}

/* gsQueueAddRgbaq: appends an RGBAQ qword; the flag halves R, G and B. */
void gsQueueAddRgbaq(unsigned int color, int halveRgb) {
    if (halveRgb) {
        GS_QUEUE_AT(gsQueueCount).data = (color & 0xFF000000) | (((color & 0xFF0000) >> 1) & 0xFF0000) | ((color >> 1) & 0x7F00) | ((color & 0xFF) >> 1);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_RGBAQ;
    } else {
        GS_QUEUE_AT(gsQueueCount).data = color;
        GS_QUEUE_AT(gsQueueCount++).reg = GS_RGBAQ;
    }
}

extern int gsQueueAlphaTest;
extern int gsQueueTexAlphaExpand;

/* gsQueueSetAlphaFlags: stores the texture-alpha expand flag and the alpha-test flag. */
void gsQueueSetAlphaFlags(int texAlphaExpand, int alphaTest, int unusedArg) {
    gsQueueTexAlphaExpand = texAlphaExpand;
    gsQueueAlphaTest = alphaTest;
}

/* gsQueueSetTest: appends TEST_1 (0x47), with alpha test when gsQueueAlphaTest is 1. */
void gsQueueSetTest(void) {
    /* The original compares against a zero local before the != 1 test; the byte-exact branch order needs it. */
    int off = 0;

    if (gsQueueAlphaTest == off || gsQueueAlphaTest != 1) {
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 3);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_TEST_1;
    } else {
        GS_QUEUE_AT(gsQueueCount).data = GS_SET_TEST(1, 6, 0, 0, 0, 0, 1, 3);
        GS_QUEUE_AT(gsQueueCount++).reg = GS_TEST_1;
    }
}

/* gsQueueSetClamp: appends CLAMP_1 (0x08). */
void gsQueueSetClamp(XglPacket *unused, int mode, int value) {
    if (mode) {
        gsQueueBase[gsQueueCount * 2] = ((u64)(value ^ 0x3FF) << 24) | 0x1C0F;
    } else {
        gsQueueBase[gsQueueCount * 2] = ((u64)(value ^ 0x3FF) << 4) | 0x20000000FULL;
    }
    gsQueueBase[gsQueueCount++ * 2 + 1] = GS_CLAMP_1;
}

/* int here, u64 in tu229/tu231: each caller's bytes need its own view of the result (a u64 return changes this TU). */
int gsMakeTex0(int value, int flag);

/* gsQueueTex0Value: the TEX0_1 data for a texture word, with the flag from gsQueueAlphaTest. */
int gsQueueTex0Value(int value)
{
  /* Zero local: 'flag' doubles as the compare operand and the 0 argument, as in the original. */
  int flag = 0;
  if ((gsQueueAlphaTest == flag) || (gsQueueAlphaTest != 1))
  {
    return gsMakeTex0(value, flag);
  }
  return gsMakeTex0(value, 1);
}

/* gsQueueSetTexA: appends TEXA (0x3B); bit 15 is set when gsQueueTexAlphaExpand is set. */
void gsQueueSetTexA(void) {
    /* Same zero-local compare as gsQueueSetTest; keeps the original branch order. */
    int off = 0;

    if (gsQueueTexAlphaExpand == off || gsQueueTexAlphaExpand != 1) {
        GS_QUEUE_AT(gsQueueCount).data = 0;
        GS_QUEUE_AT(gsQueueCount++).reg = GS_TEXA;
    } else {
        GS_QUEUE_AT(gsQueueCount).data = 0x8000;
        GS_QUEUE_AT(gsQueueCount++).reg = GS_TEXA;
    }
}

/* gsQueueFlushTexture: appends TEXFLUSH (0x3F). */
void gsQueueFlushTexture(void) {
    GS_QUEUE_AT(gsQueueCount).data = 0;
    GS_QUEUE_AT(gsQueueCount++).reg = GS_TEXFLUSH;
}

/* gsQueueSetTexFilter: appends TEX1_1 (0x14), MMAG and MMIN from the argument. */
void gsQueueSetTexFilter(int filter) {
    GS_QUEUE_BASE[gsQueueCount].data = GS_SET_TEX1(1, 0, filter, filter, 0, 0, 0);
    ((GsQueueWords *)GS_QUEUE_BASE)[gsQueueCount++].word[1] = GS_TEX1_1;
}

/* Packs an alpha byte over opaque white: 0xAAFFFFFF. */
#define EFFECT_ALPHA_WHITE(alpha) (((alpha) << 24) | 0xFFFFFF)

int recordGridCellCount(void);
void func_002748C8(int bank, int code, int arg, int *params);

/* effectRequestToCommand: translates an effect request (kind, with a strength value) into the six-word parameter block and
   command code that func_002748C8 takes. Kinds not listed send command 0 with an unset block. */
void effectRequestToCommand(int bank, unsigned int kind, int arg, unsigned int value)
{
    int params[12];
    int command = 0;
    int scaled;
    float cells;
    float ratio;

    switch (kind) {
    case 0:
        break;
    case 1:
        params[1] = 0;
        params[0] = EFFECT_ALPHA_WHITE(value);
        command = 10;
        break;
    case 2:
        params[1] = 0;
        params[0] = value << 24;
        command = 10;
        break;
    case 35:
        params[1] = 1;
        params[0] = EFFECT_ALPHA_WHITE(value);
        command = 10;
        break;
    case 7:
        value = value < 0x4D ? value : 0x4C;
        params[0] = 0x80FFFFFF;
        params[1] = value + 0x32;
        params[2] = value + 10;
        command = 8;
        break;
    case 3:
        params[0] = 0x40FFFFFF;
        if (value == 0) {
            value = 1;
        }
        params[1] = value;
        command = 11;
        break;
    case 4:
        params[0] = 0x40FFFFFF;
        if (value == 0) {
            value = 1;
        }
        params[1] = value;
        command = 18;
        break;
    case 5:
        params[0] = EFFECT_ALPHA_WHITE(value);
        command = 12;
        break;
    case 8:
    case 9:
    case 12:
    case 13:
        scaled = (int)((float)value / 127.0f * 247.0f);
        command = 13;
        switch (kind) {
        case 8:
        case 9:
            params[0] = 0xFFFFFF;
            break;
        case 12:
        case 13:
            params[0] = 0;
            break;
        }
        switch (kind) {
        case 8:
        case 12:
            params[1] = scaled;
            params[2] = scaled - 0x32;
            break;
        case 9:
        case 13:
            params[2] = scaled;
            params[1] = scaled - 0x32;
            break;
        }
        if (params[1] < 0) {
            params[1] = 0;
        }
        if (params[1] > 0x7F) {
            params[1] = 0x7F;
        }
        if (params[2] < 0) {
            params[2] = 0;
        }
        if (params[2] > 0x7F) {
            params[2] = 0x7F;
        }
        params[3] = 0;
        break;
    case 10:
    case 11:
    case 14:
    case 15:
        scaled = (int)((float)value / 127.0f * 197.0f);
        command = 13;
        switch (kind) {
        case 10:
        case 11:
            params[0] = 0xFFFFFF;
            break;
        case 14:
        case 15:
            params[0] = 0;
            break;
        }
        switch (kind) {
        case 10:
        case 14:
            params[1] = scaled;
            params[2] = scaled - 0x2D;
            break;
        case 11:
        case 15:
            params[2] = scaled;
            params[1] = scaled - 0x2D;
            break;
        }
        if (params[1] < 0) {
            params[1] = 0;
        }
        if (params[1] > 0x7F) {
            params[1] = 0x7F;
        }
        if (params[2] < 0) {
            params[2] = 0;
        }
        if (params[2] > 0x7F) {
            params[2] = 0x7F;
        }
        params[3] = 1;
        break;
    case 16:
    case 17:
        ratio = (float)value / 127.0f;
        params[0] = 0x7FFFFFFF;
        params[3] = 0xFF4020;
        params[4] = 0;
        params[2] = (int)(360.0f - ratio * 360.0f);
        command = 14;
        if (kind == 16) {
            params[1] = 1;
        } else if (kind == 17) {
            params[1] = 2;
        }
        break;
    case 18:
    case 19:
        ratio = (float)value / 127.0f;
        params[0] = 0x7FFFFFFF;
        params[3] = 0xFF0000;
        params[4] = 0;
        params[2] = (int)(360.0f - ratio * 360.0f);
        command = 15;
        if (kind == 18) {
            params[1] = 1;
        } else if (kind == 19) {
            params[1] = 0;
        }
        break;
    case 6:
        params[1] = value;
        params[0] = 0x7FFFFFFF;
        command = 9;
        if ((int)value <= 0) {
            params[1] = 1;
        }
        break;
    case 20:
        command = 3;
        params[0] = EFFECT_ALPHA_WHITE(value);
        params[1] = 0;
        params[2] = 0x11;
        params[3] = 0x11;
        break;
    case 36:
        command = 1;
        cells = (float)recordGridCellCount();
        ratio = (float)value / 127.0f;
        params[0] = 0x7FFFFFFF;
        params[1] = (int)(cells * ratio);
        break;
    case 21:
        params[0] = EFFECT_ALPHA_WHITE(value);
        params[2] = 10;
        command = 5;
        params[3] = -13;
        params[5] = 7;
        params[1] = 0;
        params[4] = 0;
        break;
    case 22:
        params[0] = EFFECT_ALPHA_WHITE(value);
        params[2] = 0x12;
        command = 5;
        params[3] = -20;
        params[5] = 7;
        params[1] = 0;
        params[4] = 0;
        break;
    case 23:
        params[0] = EFFECT_ALPHA_WHITE(value);
        params[1] = 1;
        command = 5;
        params[2] = 10;
        params[3] = -13;
        params[4] = (value << 24) | 0xFAB4B4;
        params[5] = 7;
        break;
    case 24:
        params[0] = EFFECT_ALPHA_WHITE(value);
        command = 4;
        params[2] = 12;
        params[3] = 7;
        params[1] = 0;
        params[4] = 4;
        params[5] = 0;
        break;
    case 25:
        params[0] = EFFECT_ALPHA_WHITE(value);
        params[1] = 0xAF;
        command = 16;
        params[2] = 22;
        break;
    case 26:
        params[1] = 1;
        params[0] = EFFECT_ALPHA_WHITE(value);
        command = 11;
        break;
    case 27:
        params[1] = 1;
        params[0] = EFFECT_ALPHA_WHITE(value);
        command = 18;
        break;
    case 28:
        params[1] = value;
        command = 20;
        params[0] = 0;
        break;
    case 29:
        params[0] = value;
        command = 22;
        break;
    case 30:
        params[0] = value >> 4;
        command = 19;
        break;
    case 31:
        params[1] = value;
        params[0] = 2;
        command = 20;
        break;
    case 32:
        params[1] = value;
        params[0] = 1;
        command = 20;
        break;
    case 33:
        params[0] = value;
        command = 21;
        break;
    case 34:
        params[0] = value;
        command = 23;
        break;
    }
    func_002748C8(bank, command, arg, params);
}

int func_002757F0(void);

void func_0027C7B8(void) {
    func_002757F0();
}
