#include "common.h"
#include "gs.h"
#include "shared.h"
#include "main/nml_model_set.h"
#include "main/tu045.h"
#include "main/xgl_2_031.h"

typedef unsigned int XglQuadword __attribute__((mode(TI)));

typedef union Matrix {
    float elements[16];
    Matrix4 rows;
    int words[4][4];
    XglQuadword quadwords[4];
} Matrix;

typedef union GsTag {
    u64 value;
    u32 words[2];
} GsTag;

#define NML_MAP_SHADOW_PARTS_MAX 8

/* One row of a pxy plane table: a plane or an angle quadruple. */
typedef struct PxyRow {
    float v[4];
} PxyRow;

/* func_001BBCCC fills it (at most two entries, five quadwords allocated). */
typedef struct PxyPlaneTable {
    PxyRow plane[2]; /* +0x00: plane normal and distance */
    PxyRow angle[2]; /* +0x20: rotX, rotY, then the next rotX, rotY */
} PxyPlaneTable;

/* The same five-quadword plane table as a packet holds it (quadword-aligned rows). */
typedef struct PxyClipPlanes {
    AlignedVector4 plane[2]; /* +0x00 */
    AlignedVector4 angle[2]; /* +0x20 */
} PxyClipPlanes;

/* A 0x60-byte part of a pxy model. */
typedef struct PxyPart {
    u8 unk00[8];
    signed char layer; /* +0x08 */
    u8 unk09[7];
    s16 maskBit; /* +0x10: bit of the D_0096A00C hide mask */
    u8 unk12[2];
    s16 index; /* +0x14: index into the per-part flag bytes */
    u8 unk16[2];
    u32 flags; /* +0x18 */
    u8 unk1C[4];
    Vector4 boundsMin; /* +0x20 */
    Vector4 boundsMax; /* +0x30 */
    AlignedVector4 sphere; /* +0x40: centre and radius */
    u8 unk50[0x60 - 0x50];
} PxyPart;

/* A 0x20-byte range of parts: [0] opaque, [1] translucent. */
typedef struct PxyPartRange {
    u8 unk00[0x14];
    int first; /* +0x14 */
    int count; /* +0x18 */
    u8 unk1C[4];
} PxyPartRange;

/* The "pxy" model file header. */
typedef struct PxyHeader {
    char magic[4]; /* "pxy" */
    float version; /* +0x04 */
    u32 flags; /* +0x08 */
    u8 unk0C[4];
    char name[0x20]; /* +0x10 */
    int layerCount; /* +0x30 */
    u8 unk34[8];
    int offset3C; /* +0x3c */
    int partOffset; /* +0x40 */
    u8 unk44[4];
    int partCount; /* +0x48 */
    u8 unk4C[8];
    int rangeOffset; /* +0x54 */
    int offset58; /* +0x58 */
    int unk5C; /* +0x5c */
    int geometryCount; /* +0x60 */
    int offset64; /* +0x64 */
    int offset68; /* +0x68 */
    int offset6C; /* +0x6c */
    int offset70; /* +0x70 */
    int offset74; /* +0x74 */
    int offset78; /* +0x78 */
    u8 unk7C[0x8C - 0x7C];
    float radius; /* +0x8c: bounding radius */
    u8 unk90[0xE8 - 0x90];
    int sortEntryOffset; /* +0xe8 */
} PxyHeader;

/* The light record D_0096A06C points to (pxyState starts with one). */
typedef struct PxyLight {
    Vector4 direction; /* +0x00 */
    u8 unk10[0xD0 - 0x10];
    Vector4 savedDirection; /* +0xd0 */
    Vector4 drawDirection; /* +0xe0 */
    Vector4 override; /* +0xf0; w != 0 selects it */
} PxyLight;

/* The 0x180-byte light block of a shadow projector, cleared for each model. */
typedef struct PxyShadowLight {
    Vector4 direction; /* +0x00 */
    u8 unk10[0x70 - 0x10];
    float color[3]; /* +0x70 */
    u8 unk7C[0x180 - 0x7C];
} PxyShadowLight;

/* The shadow projector D_0096A080 points to (0x36 quadwords). */
typedef struct PxyShadow {
    Matrix light; /* +0x000: light space to world */
    Matrix unk040; /* +0x040 */
    Matrix view; /* +0x080: world to light space */
    Matrix project; /* +0x0c0: world to shadow texture */
    PxyShadowLight lighting; /* +0x100 */
    int x; /* +0x280: texture rectangle */
    int y; /* +0x284 */
    int width; /* +0x288 */
    int height; /* +0x28c */
    s16 repeatU; /* +0x290: REGION_REPEAT instead of REGION_CLAMP */
    s16 repeatV; /* +0x292 */
    s16 psm; /* +0x294 */
    s16 frameBase; /* +0x296 */
    int clutOffset; /* +0x298 */
    u8 unk29C[4];
    float scaleU; /* +0x2a0 */
    float scaleV; /* +0x2a4 */
    u8 unk2A8[8];
    u64 tex0; /* +0x2b0 */
    u64 clamp; /* +0x2b8 */
    Vector4 uv; /* +0x2c0: offset u, scale u, offset v, scale v */
    Vector4 unk2D0; /* +0x2d0 */
    Vector4 direction; /* +0x2e0 */
    Vector4 center; /* +0x2f0 */
    Vector4 min; /* +0x300 */
    Vector4 max; /* +0x310 */
    Vector4 bounds; /* +0x320: centre and radius */
    int unk330; /* +0x330 */
    u8 unk334[4];
    u64 unk338; /* +0x338 */
} PxyShadow;

/* The 0x2a0-byte parameter block copied from nmlRenderStatus into the packet. */
typedef struct PxyPacket {
    u32 renderStatus; /* +0x00 */
    int camera; /* +0x04: camera that owns the model */
    int unk08; /* +0x08: draw mode; 1 and 4 cast shadows */
    u8 unk0C[4];
    PxyHeader *header; /* +0x10 */
    PxyPartRange *ranges; /* +0x14 */
    u8 unk18[8];
    PxyPart *parts; /* +0x20 */
    u8 unk24[0x34 - 0x24];
    u8 *partFlags; /* +0x34 */
    int unk38; /* +0x38 */
    u8 unk3C[0x5C - 0x3C];
    struct PxyMatrix *matrices; /* +0x5c: one per part matrix index */
    u8 unk60[0x68 - 0x60];
    int unk68; /* +0x68 */
    PxyLight *light; /* +0x6c */
    u8 unk70[0x80 - 0x70];
    PxyShadow *shadow; /* +0x80 */
    u32 unk84; /* +0x84 */
    u8 unk88[0x98 - 0x88];
    struct PxyMatrix *unk98; /* +0x98 */
    int unk9C; /* +0x9c */
    u8 unkA0[4];
    s16 planeCount; /* +0xa4 */
    u8 unkA6[2];
    PxyClipPlanes *planes; /* +0xa8: clip planes */
    u8 unkAC[0xC0 - 0xAC];
    float unkC0; /* +0xc0: lower clip height */
    struct PxyHideMask *hideMask; /* +0xc4 */
    float unkC8; /* +0xc8: upper clip height */
    u8 unkCC[0xD4 - 0xCC];
    u32 unkD4; /* +0xd4 */
    u8 unkD8[0xE0 - 0xD8];
    float world[4][4]; /* +0xe0: model to world */
    u8 unk120[0x170 - 0x120];
    int cameraMask[16]; /* +0x170: nonzero where a camera draws the model */
    Vector4 center; /* +0x1b0 */
    void (*preDraw)(int, int); /* +0x1c0 */
    u8 unk1C4[0x1D0 - 0x1C4];
    void (*postDraw)(int, int); /* +0x1d0 */
} PxyPacket;

#define PXY_SLOT_COUNT 4 /* ring of recently drawn packets */

typedef struct PxySlot {
    PxyPacket *packet;
    int batch;
    int translucentBatch;
    int unk0C;
} PxySlot;

typedef struct PxyState {
    PxyLight light;
    u8 unk100[0x6A0 - 0x100];
    struct PxyBatch *overlayBatches[32]; /* +0x6a0: batches of header flags 0x81, D_0096A2A0 of them */
    struct PxyBatch *shadowBatches[64]; /* +0x720: batches of the shadow casters */
    struct PxyBatch *unk820[0xA0]; /* +0x820 */
    u8 unkAA0[0x49C0 - 0xAA0];
    PxySlot slots[PXY_SLOT_COUNT]; /* +0x49c0 */
} PxyState;

typedef struct PxyBatch {
    s16 start; /* +0x00: first entry in D_0097A980 */
    s16 count;
    float depth;
    u32 flags;
    PxyPacket *packet; /* +0x0c */
    s16 unk10;
    s16 next; /* +0x12: index of the next batch, < 0 at the end */
    struct PxyBatch *translucent; /* +0x14 */
    u8 unk18[8];
} PxyBatch;

typedef struct PxyBatchTable {
    PxyBatch batches[256];
    int unk2000;
    int unk2004;
    int count; /* +0x2008 */
    int unk200C;
} PxyBatchTable;

typedef struct PxyDrawEntry {
    PxyPart *part;
    PxyPacket *packet;
} PxyDrawEntry;

/* Draw state kept in the EE scratchpad while the batches are drawn. */
typedef struct PxyDrawWork {
    PxyDrawEntry *entry; /* +0x00 */
    int i; /* +0x04 */
    u32 batchFlags; /* +0x08 */
    int count; /* +0x0c */
    PxyPacket *packet; /* +0x10 */
    PxyPacket *lastPacket; /* +0x14 */
    int mode; /* +0x18 */
    PxyPart *part; /* +0x1c */
    u32 partFlags; /* +0x20 */
    u32 packetFlags; /* +0x24 */
    int unk28; /* +0x28 */
    u8 cameraBit; /* +0x2c */
    u8 unk2D[3];
    u32 frameMask; /* +0x30 */
    u32 frameBase; /* +0x34 */
    PxyBatch *batch; /* +0x38 */
} PxyDrawWork;

#define pxyDrawWork ((PxyDrawWork *)0x70000000)

#define PXY_BATCH_MAX 0x100
#define PXY_OPAQUE_ENTRY_END 0x1000      /* end of the opaque draw-entry range */
#define PXY_TRANSLUCENT_ENTRY_END 0x1A00 /* end of the translucent draw-entry range */
#define PXY_SHADOW_QUADWORDS 0x36        /* size of a PxyShadow in quadwords */

/* Bits of the per-part flag bytes (PxyPacket.partFlags). */
#define PXY_PART_VISIBLE 0x80  /* set for every part when the model is entered */
#define PXY_PART_MERGED 0x40   /* translucent part already attached to the previous entry */
#define PXY_PART_LOD_MASK 0xF  /* level-of-detail bits tested for header flag 1 */

typedef struct PxyCamera {
    u8 unk00[0xD0];
    Vector4 position; /* +0xd0 */
    u8 unkE0[0x4F0 - 0xE0];
    Matrix4 unk4F0; /* +0x4f0: the clip matrix _ModelCalcClipInitMat1 starts from */
} PxyCamera;

typedef struct PxySortEntry {
    u8 unk00[0xC];
    s16 matrix; /* +0x0c */
    s16 key; /* +0x0e */
} PxySortEntry;

typedef struct PxySortItem {
    Vector4 position;
    s16 start; /* +0x10 */
    s16 count;
    s16 key;
    s16 matrix;
    float depth; /* +0x18 */
    u8 unk1C[4];
} PxySortItem;

typedef struct PxySortGroup {
    int count;
    s16 start; /* +0x04 */
    s16 entryCount; /* +0x06 */
    u8 unk08[8];
    PxySortItem items[32]; /* +0x10 */
    PxySortItem *sorted[32]; /* +0x410 */
} PxySortGroup;

typedef struct PxySortWork {
    PxySortGroup groups[8];
    int groupCount; /* +0x2480 */
    int entryCount;
    PxyDrawEntry *entries; /* +0x2488 */
    PxySortItem *item; /* +0x248c */
    PxySortGroup *group; /* +0x2490 */
    int i; /* +0x2494 */
    PxySortEntry *sortEntries; /* +0x2498 */
    PxySortEntry *sortEntry; /* +0x249c */
    int j; /* +0x24a0 */
    PxyDrawEntry *out; /* +0x24a4 */
    PxySortItem **cursor; /* +0x24a8 */
    PxySortItem *swapped; /* +0x24ac */
    void *matrixSet; /* +0x24b0 */
    float (*matrices)[4][4]; /* +0x24b4 */
    u8 unk24B8[8];
    float matrix[4][4]; /* +0x24c0 */
    Vector4 eye; /* +0x2500 */
    PxyDrawEntry buffer[0x35E]; /* +0x2510 */
} PxySortWork;

typedef struct PxyHideMask { u32 bits[1]; } PxyHideMask;
typedef struct PxyMatrix { float m[4][4]; } PxyMatrix;

/* Slot of a render layout store: one 32-bit word read as float or int. */
typedef union {
    float f;
    int i;
} LayoutSlot;

/*
 * Request bits of the layout store's render-status word (LayoutStore +0x250).
 * Each nmlModelSet* setter ORs its own bit; the render entries set the
 * microcode-family bit around nmlPacketAddTransMicrocode.
 */
enum NmlRenderStatus {
    NML_RENDER_PLACE = 0x1,           /* nmlModelSetPlace */
    NML_RENDER_FOG = 0x2,             /* nmlModelSetFogCol / SetFogDist */
    NML_RENDER_STENCIL = 0x4,
    NML_RENDER_ZWRITE = 0x8,
    NML_RENDER_LIGHT = 0x10,          /* nmlModelSetLight */
    NML_RENDER_TOUMEI = 0x20,         /* see-through (toumei) parts */
    NML_RENDER_HUMAN_MODEL = 0x40,
    NML_RENDER_PROREAL = 0x800,       /* nmlModelSetTexProreal / RenderProreal */
    NML_RENDER_POINT_LIGHT = 0x1000,  /* nmlModelSetPointLight */
    NML_RENDER_FOG_CANCEL = 0x2000,
    NML_RENDER_TEXMAP_TEXTURE = 0x4000, /* nmlModelSetTexMap, plain texture kind */
    NML_RENDER_TEXTURE_PASS = 0x8000, /* set while nmlModelRenderTexture runs */
    NML_RENDER_MAP_ENTRY = 0x10000,   /* nmlModelSetMapEntry / SetShadowMapEntry */
    NML_RENDER_CULLED = 0x20000,      /* nmlModelCalcSample: clipped out of every active window */
    NML_RENDER_TEXMAP_DROP = 0x40000, /* nmlModelSetTexMap, drop kind */
    NML_RENDER_DROP_PASS = 0x80000,   /* set while nmlModelRenderDrop runs */
    NML_RENDER_FACE_MODEL = 0x100000, /* nmlModelSetFaceModel */
    NML_RENDER_TEXMAP_CIRCLE = 0x200000,
    NML_RENDER_TEXMAP_DROP_CIRCLE = 0x400000,
    NML_RENDER_BACK_CLEAR = 0x800000, /* group becomes NML_GROUP_BACK_CLEAR */
    NML_RENDER_GLOBAL_FOG = 0x1000000, /* nmlModelSetGlobalFogCol / Dist */
    NML_RENDER_GLOBAL_POINT_LIGHT = 0x2000000
};

/*
 * A loaded model resource.  Its block count is +0x44 and the table at +0xb0
 * holds byte offsets from this record to individual NmlModelBlock records.
 */
typedef struct NmlModel {
    unsigned char unmodeled_000[0x3f];
    signed char unclassified_3f;
    int partCount; /* +0x40 */
    int blockCount;
    int sampleCount; /* +0x48 */
    unsigned char unmodeled_04c[4];
    int weightedMatrixOffset; /* +0x50: byte offset from the model to its weighted matrices */
    unsigned char unmodeled_054[4];
    int weightOffset;    /* +0x58: byte offset from the model to the sample weights */
    int transformOffset; /* +0x5c: byte offset from the model to the sample transforms */
    u32 geometryTableOffset; /* +0x60: byte offset to the geometry block offset table */
    unsigned char unmodeled_064[8];
    u32 textureSetOffset; /* +0x6c: byte offset to the model's texture set; 0 = none */
    Vector4 boxMinOffset; /* +0x70: bounding box corners relative to the placement */
    Vector4 boxMaxOffset; /* +0x80 */
    Vector4 entryTranslation; /* +0x90; w at +0x9c */
    int unclassified_a0;
    unsigned char unmodeled_0a4[0x0c];
    int blockOffsets[1];
} NmlModel;

/* The same 16 bytes as two doublewords, for the plain ld/sd copies. */
typedef struct NmlQword {
    unsigned long long lo;
    unsigned long long hi;
} NmlQword;

/* A vector on a doubleword boundary: the original copies it with ld/sd. */
typedef union NmlVector4Overlay {
    Vector4 v;
    NmlQword q;
} NmlVector4Overlay;

typedef struct NmlPartPixelAlpha {
    short alpha;
    short partId;
} NmlPartPixelAlpha;

/* One 0x40-byte weighted record: a transform matrix, or the part flags word
 * at +0x1c (matrix[1][3]).  The packet builders send it as four vectors. */
typedef struct NmlPartFlagData {
    unsigned char unmodeled_00[0x1c];
    unsigned int flags; /* +0x1c */
    unsigned char unmodeled_20[0x40 - 0x20];
} NmlPartFlagData;

typedef union NmlWeightedMatrix {
    Matrix4 matrix;
    Vector4 rows[4];
    NmlPartFlagData part;
} NmlWeightedMatrix;

/* +0x1c0 of a layout: the fog colour parcel; the fourth word is the integer
 * fog intensity nmlModelEntry and nmlPacketAddFog store. */
typedef struct NmlFogColor {
    float color[3];
    int intensity;
} NmlFogColor;

typedef union NmlPartsReferenceScratch {
    u64 halves[2];
    Vector4 vector;
    float components[4];
} NmlPartsReferenceScratch;

/*
 * The "proreal" parameter block: the 0x1e0 bytes nmlModelSetProreal builds in
 * s_inProReal (main 0x0095b940) and nmlModelEntry copies into the packet with
 * nmlPacketSetAttributeData16N(s_inProReal, 0x1e), storing the packet address
 * it returns in the layout's proreal (main 0x00233028-0x00233030).
 */
typedef struct NmlProRealParam {
    unsigned char unmodeled_000[0x40];
    Matrix projection_transform;
    Matrix4 normal_transform;
    u32 block_material_data[16];
    unsigned char unmodeled_100[0x1c];
    /*
     * +0x11c: nmlModelSetProreal clears it (main 0x0022f9dc) and the two
     * projection entries set it to 1 immediately after calling that function,
     * nmlModelCalcEntryProjectCircle (main 0x002320ec) and
     * nmlModelCalcEntryProjectMap (main 0x00232680), so it marks a block built
     * for a projected entry.  add_exec_prog reads it to take the projected
     * slot of the "Pro" microcode family instead of the plain one.
     */
    u32 projection_entry;
    unsigned char unmodeled_120[0x10];
    GsTag texture_gs_parameters[2];
    unsigned char unmodeled_140[0x20];
    /* +0x160: the last normal row; the map projection's volume test reads
     * its x and z as the clipping plane normal. */
    union {
        XglQuadword final_normal_row;
        struct {
            float plane_normal_x;
            unsigned char unmodeled_164[4];
            float plane_normal_z;
        };
    };
    unsigned char unmodeled_170[0x20];
    NmlPartsReferenceScratch reference_point; /* +0x190 */
    Vector4 bounds_min; /* +0x1a0 */
    Vector4 bounds_max; /* +0x1b0 */
    void *texture; /* +0x1c0 */
    unsigned char unmodeled_1c4[4];
    u64 alpha; /* +0x1c8 */
} NmlProRealParam;

struct NmlStealthGeometry;

/*
 * The model layout the engine renders from: the single s_inLayout (main
 * 0x004a91e0, 0x320 bytes) that the nmlModelSet* API fills, plus the
 * per-entry copies nmlModelEntry stores in s_apModelLayout.  The packet
 * builders and the filter renderers read the same record.
 */
union LayoutStore {
    LayoutSlot slots[200];
    unsigned long long qwords[100];
    struct {
        /* +0x0; row 3 w (+0x3c) carries the block material alpha the drop
         * shadow path computes. */
        Matrix4 lightDirection;
        Matrix4 lightColor; /* +0x40; rows 0-2 are the normal light vectors */
        Matrix4 place; /* +0x80 */
        Matrix4 inversePlace; /* +0xc0 */
        Vector4 pointPosition[4]; /* +0x100 */
        Vector4 pointColor[4]; /* +0x140 */
        Matrix4 projectionMatrix; /* +0x180; saved by the circle and map projection paths */
        NmlFogColor fogColor; /* +0x1c0 */
        Vector4 fogPara; /* +0x1d0 */
        float filterAlpha; /* +0x1e0 */
        float filterAmplitude; /* +0x1e4 */
        unsigned char unmodeled_1e8[8];
        /* +0x1f0: the model centre (w = radius): copied as a projection input,
         * passed to AlphaGroupSortEntry, the circle shadow centre and the
         * stealth filter's fallback sphere. */
        NmlVector4Overlay center;
        Vector4 multiplyColor; /* +0x200 */
        Vector4 currentPosition;            /* +0x210; copied from the weighted position by nmlModelCalcSample */
        float transparency; /* +0x220; nmlModelSetTransparency (also +0x2c0) */
        float reflTransparency; /* +0x224; nmlModelSetReflTransparency */
        float defaultBlockAlpha; /* +0x228 */
        float alphaLimit; /* +0x22c */
        union {
            u32 alphaOptions;
            float alphaOptionsValue;
        }; /* +0x230 */
        unsigned int packetFilterFlags;
        int pixelAlpha;                     /* +0x238, full-width pixel-alpha word */
        void *currentMatrix;                /* +0x23c; the model matrix nmlModelSetMatrix stores */
        const void *texture;                /* +0x240; the texture set nmlModelSetTexture stores */
        int texfunc;                        /* +0x244 */
        u64 alpha;                          /* +0x248 */
        /*
         * +0x250: the render-status word.  nmlModelSetRenderStatus is the
         * setter that names it, and it ORs the value it is given (main
         * 0x0022fab0 sets 0x100, 0x0022fad0 sets 0x200); the rest of the
         * nmlModelSet* API ORs one request bit each into the same word -
         * nmlModelSetPlace 0x1 (main 0x0022ff24), nmlModelSetFogCol 0x2
         * (0x00230030), nmlModelSetStencil 0x4 (0x0022fd28), nmlModelSetZwrite
         * 0x8 (0x0022fd48), nmlModelSetLight 0x10 (0x0022fe50),
         * nmlModelSetToumei 0x20 (0x0022fdf0), nmlModelSetHumanModel 0x40
         * (0x0022e760), nmlModelSetTexProreal 0x800 (0x0022fa5c),
         * nmlModelSetPointLight 0x1000 (0x0022f55c), nmlModelSetFogCancel
         * 0x2000 (0x0023034c), nmlModelSetTexMap 0x4000 (0x0022f670) and
         * nmlModelSetFaceModel 0x100000 (0x0022e73c).  The three render
         * entries set the bit that picks their microcode family just before
         * calling nmlPacketAddTransMicrocode: nmlModelRenderProreal 0x800
         * (main 0x002346c4), nmlModelRenderTexture 0x8000 (0x00234938) and
         * nmlModelRenderDrop 0x80000 (0x00234d58).
         */
        u32 render_status;
        NmlModel *model; /* +0x254; also how a child entry finds its parent layout */
        NmlWeightedMatrix *weightedMatrices; /* +0x258 */
        unsigned char unmodeled_25c[4];
        /*
         * +0x260: this layout's own proreal block, the packet address
         * nmlModelEntry stores after copying s_inProReal (main 0x00233030).
         */
        NmlProRealParam *proreal;
        /*
         * +0x264: another layout's proreal block, borrowed for the length of
         * one render.  nmlModelRenderProreal stores the entry's own +0x260
         * here (main 0x002346dc) and clears it again afterwards (main
         * 0x0023473c); nmlModelEntry clears it as well (main 0x00233034).
         * add_exec_prog reads it in preference to proreal whenever it is set.
         */
        NmlProRealParam *prorealOverride;
        int texMapMode; /* +0x268; supplied by map and actor shadow setup */
        float textureTransparency;
        int texMapSize; /* +0x270 */
        float texMapWeight; /* +0x274 */
        int mapShadowPartCount; /* +0x278 */
        int window; /* +0x27c; nmlModelSetWindow */
        struct NmlStealthGeometry *stealthParts[10]; /* +0x280 */
        int shadowHeightEnabled; /* +0x2a8 */
        float shadowHeight; /* +0x2ac */
        /*
         * +0x2b0: the render-level word nmlModelSetRenderLevel ORs its whole
         * argument into (main 0x0022e97c).  nmlModelSetSpecularOff sets bit 1
         * (main 0x0022f588), and nmlModelCalcSpecularClip sets and clears that
         * same bit (main 0x00231ae0, 0x00231bf4) and sets bit 4 (0x00231bd4).
         */
        unsigned int renderLevel;
        int partPixelAlphaCount; /* +0x2b4 */
        unsigned char *clipMasks; /* +0x2b8 */
        float specialBlockAlpha; /* +0x2bc */
        float circleShadowScale; /* +0x2c0 */
        float circleScale; /* +0x2c4 */
        int stealthEntryCount; /* +0x2c8 */
        unsigned char unmodeled_2cc[4];
        NmlPartPixelAlpha partPixelAlpha[4]; /* +0x2d0 */
        unsigned short mapShadowParts[NML_MAP_SHADOW_PARTS_MAX]; /* +0x2e0 */
        float fogIntensity[4]; /* +0x2f0; one fog intensity multiplier per viewport */
        float viewportAlpha[4];
        int viewportFlags[4];
    } fields;
};

typedef union LayoutStore LayoutStore;

void nmlModelSetZwrite(int enabled);
/* canon: config/header-canon.json chose src/math/main/spark-cont01-00231d50/nmlModelCalcDropShadow.c over 0 other accepted spellings */
extern LayoutStore D_00969DB0;

/* nmlFilterStealthMake allocates 31 sixteen-byte records. Its draw consumer
 * witnesses fourteen XYZ records at +0x30 and fourteen UV records at +0x110.
 * Each record is one vector slot: the projection helper writes its integer
 * screen coordinates into the same storage it would hold a float vector in. */
union NmlStealthXYZ {
    Vector4 vector;
    struct {
        int x;
        int y;
        int z;
        int w;
    };
};

union NmlStealthUV {
    Vector4 vector;
    struct {
        int u;
        int v;
        int clipFlags;
        unsigned char unmodeled_0c[4];
    };
};

/*
 * The allocator's 31 sixteen-byte records, including actual endpoint
 * vectors.  The first endpoint's w is the squared camera distance of the
 * part centre; nmlModelEntry depth sorts a layout's stealth parts by it
 * within a sort class.
 */
struct NmlStealthGeometry {
    Vector4 endpoints[2];
    float sortClass;
    unsigned char unmodeled_24[0x0c];
    union NmlStealthXYZ xyz[14];
    union NmlStealthUV uv[14];
};

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", pxyResetSlots);

void pxyResetSlots(void);

void func_001B9728(void) {
    pxyResetSlots();
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001B9758);

void func_001B9758(void);

void func_001B97CC(void) {
    func_001B9758();
}

void xglMatrixUnit(void *);
void xglVectorClear(void *);
void *memset(void *, int, unsigned int);
extern PxyState pxyState;
extern int nmlRenderStatus;
extern int D_0096A004;
extern int D_0096A008;
extern PxyHideMask *D_0096A00C;
extern PxyHeader *D_0096A010;
extern PxyPartRange *D_0096A014;
extern void *D_0096A018;
extern void *D_0096A01C;
extern PxyPart *D_0096A020;
extern void *D_0096A024;
extern void *D_0096A028;
extern void *D_0096A02C;
extern void *D_0096A030;
extern u8 *D_0096A034;
extern int D_0096A038;
extern void *D_0096A03C;
extern PxyHeader *D_0096A040;
extern float D_0096A044;
extern float D_0096A048;
extern float D_0096A04C;
extern void *D_0096A050;
extern void *D_0096A054;
extern void *D_0096A05C;
extern f32 D_0096A060;
extern f32 D_0096A064;
extern void *D_0096A068;
extern PxyLight *D_0096A06C;
extern PxyShadow *D_0096A080;
extern int D_0096A084;
extern float D_0096A088;
extern int D_0096A090;
extern PxyMatrix *D_0096A098;
extern int D_0096A09C;
extern int D_0096A0A0;
extern s16 D_0096A0A4;
extern PxyPlaneTable *D_0096A0A8;
extern void *D_0096A0AC;
extern int D_0096A0B4;
extern f32 D_0096A0C0;
extern void *D_0096A0C4;
extern f32 D_0096A0C8;
extern int D_0096A0D0;
extern int D_0096A0D4;
extern float D_0096A0D8;
extern unsigned char nmlModelDrawMatrix[];
extern unsigned char D_0096A120[];
extern f32 D_0096A160;
extern f32 D_0096A164;
extern f32 D_0096A168;
extern f32 D_0096A16C;
extern int D_0096A180;
extern int D_0096A184;
extern int D_0096A188;
extern int D_0096A18C;
extern Vector4 D_0096A1B0;
extern int D_0096A1C0;
extern int D_0096A1D0;
extern float D_0096A2B8;
extern int D_0096A2D8;
extern f32 D_0096A318;
extern unsigned char D_00987980[];

void pxyResetGlobals(void) {
    xglMatrixUnit(&nmlModelDrawMatrix);
    xglMatrixUnit(&D_0096A120);
    nmlRenderStatus = 0;
    D_0096A0D4 = 0;
    D_0096A008 = 0;
    D_0096A004 = 0;
    D_0096A044 = 1.0f;
    D_0096A048 = 1.0f;
    D_0096A04C = 1.0f;
    D_0096A088 = 1.0f;
    D_0096A160 = 1.0f;
    D_0096A164 = 1.0f;
    D_0096A168 = 1.0f;
    D_0096A16C = 1.0f;
    D_0096A180 = 0xFF0;
    D_0096A184 = 0xFF0;
    D_0096A188 = 0xFF0;
    D_0096A18C = 0xFF0;
    D_0096A060 = 0;
    D_0096A064 = 0;
    D_0096A084 = 0;
    D_0096A09C = 0;
    D_0096A0A4 = 0;
    D_0096A0C8 = 0.5f;
    D_0096A0C0 = -1.0f;
    xglVectorClear(&D_0096A1B0);
    D_0096A0D8 = 1.0f;
    D_0096A00C = 0;
    D_0096A010 = 0;
    D_0096A014 = 0;
    D_0096A018 = 0;
    D_0096A01C = 0;
    D_0096A020 = 0;
    D_0096A024 = 0;
    D_0096A028 = 0;
    D_0096A02C = 0;
    D_0096A030 = 0;
    D_0096A034 = 0;
    D_0096A038 = 0;
    D_0096A03C = 0;
    D_0096A040 = 0;
    D_0096A050 = 0;
    D_0096A054 = 0;
    D_0096A05C = 0;
    D_0096A068 = 0;
    D_0096A06C = &pxyState.light;
    D_0096A080 = 0;
    D_0096A090 = 0;
    D_0096A098 = 0;
    D_0096A0A0 = 0;
    D_0096A0A8 = 0;
    D_0096A0AC = 0;
    D_0096A0C4 = 0;
    D_0096A0B4 = 0;
    D_0096A0D0 = 0;
    D_0096A1C0 = 0;
    D_0096A1D0 = 0;
    D_0096A2B8 = 0;
    D_0096A2D8 = 0;
    D_0096A318 = 1.0f;
    memset(&D_00987980, 0, 0x30);
}

void pxyResetGlobals(void);
extern int D_0096A2BC;
extern int D_0096A2C0;
extern int D_0096A2DC;
extern int D_0096A308;
extern int D_0096A30C;
extern int D_0096A324;

void pxyResetCounters(void)
{
    pxyResetGlobals();
    D_0096A2BC = 0;
    D_0096A2C0 = 0;
    D_0096A2DC = 1;
    D_0096A308 = 0;
    D_0096A30C = 0x1000;
    D_0096A324 = 0;
}

void pxyResetCounters(void);

void func_001B9B44(void) {
    pxyResetCounters();
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", pxyClearEntryTable);

void func_001AB3F8(void *);
void pxyClearEntryTable();
extern int D_00645F90[4];
extern int D_00645F94;
extern int D_00645F98;
extern int D_00645F9C;
extern float D_00969E80;
extern float D_00969E90;
extern float D_00969EA0;
extern float D_00969EB0;
extern float D_00969EC0;
extern float D_00969ED0;
extern float D_00969EE0;
extern f32 D_0096A310;
extern f32 D_0096A314;

void pxyInitTables(void)
{
    D_00645F90[0] = 1;
    D_00645F94 = 0;
    D_00645F98 = 0;
    D_00645F9C = 0;
    func_001AB3F8(&pxyState);
    memset(&D_00969E80, 0, 0x180);
    D_00969E80 = 1.0f;
    D_00969E90 = 1.0f;
    D_00969EA0 = 1.0f;
    D_00969EB0 = 1.0f;
    D_00969EC0 = 1.0f;
    D_00969ED0 = 1.0f;
    D_00969EE0 = 1.0f;
    D_0096A310 = 0;
    D_0096A314 = 0;
    pxyClearEntryTable();
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001B9D78);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001B9EC4);

typedef int s32;

void func_001BA214(void) {
    D_0096A0D4 |= 1;
}

void func_001BA248(int value)
{
  D_0096A0D0 = value;
}

void func_001BA278(void) {
    D_00969DB0.fields.render_status |= 0x400;
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BA2AC);

/* Address of the pxy state block: the 16-slot entry table at +0x630 and the slot records. */
void *pxyGetState(void) {
    return &pxyState;
}

void func_001BA320(void) {
    func_001AB3F8(&pxyState);
}

void func_001BA358(int value)
{
  D_0096A2DC = value;
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BA388);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", pxyFindEntryIndex);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", pxyGetEntry);

void func_001BA618(f32 first, f32 second) {
    D_0096A060 = first;
    D_0096A064 = second;
}

extern int pxyFlags;

void func_001BA658(void) {
    pxyFlags |= 2;
}

void func_001BA68C(void) {
    pxyFlags |= 8;
}

void func_001BA6C0(void) {
    pxyFlags |= 0x2000;
}

void func_001BA6F4(void) {
    pxyFlags |= 0x200;
}

void func_001BA728(void) {
    pxyFlags |= 0x4000;
}

void func_001BA75C(void) {
    pxyFlags |= 0x20000;
}

void func_001BA794(void) {
    D_00969DB0.fields.render_status |= NML_RENDER_FOG;
    D_0096A2D8 = 1;
}

/* Requests packet mode 3 (bit 0x10 of pxyFlags); pxyFlush passes that bit to xglPacketSetMode3Flag. */
void pxySetMode3Flag(int enabled)
{
    if (enabled) {
        pxyFlags |= 0x10;
    }
}

/* Requests packet mode 4 (bit 0x80000 of pxyFlags); pxyFlush passes that bit to xglPacketSetMode4Flag. */
void pxySetMode4Flag(void) {
    pxyFlags |= 0x80000;
}

void func_001BA850(void) {
    pxyFlags |= 0x40000;
}

void func_001BA888(void) {
    pxyFlags |= 0x40;
}

void func_001BA8BC(void) {
    pxyFlags |= 0x80;
}

void func_001BA8F0(void) {
    D_00969DB0.fields.render_status |= NML_RENDER_PROREAL;
}

void func_001BA924(void)
{
    nmlRenderStatus |= 0x20000;
    pxyFlags |= 0x8000;
}

void func_001BA970(int value)
{
  D_0096A090 = value;
}

extern float D_0096A08C;

void func_001BA9A0(float value)
{
    D_0096A08C = value;
    nmlRenderStatus |= 0x100;
}

void func_001BA9E4(void)
{
    nmlRenderStatus |= 0x40000;
    pxyFlags |= 0x100000;
}

void func_001BAA34(void) {
    D_00969DB0.fields.render_status |= NML_RENDER_MAP_ENTRY;
}

extern int D_0096A2C4;

void func_001BAA6C(void) {
    D_0096A2C4 = 1;
}

void func_001BAA98(s32 value) {
    if (D_0096A1C0 == 0) {
        D_0096A1C0 = value;
    }
}

void func_001BAAD8(s32 value) {
    if (D_0096A1D0 == 0) {
        D_0096A1D0 = value;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", pxyRegisterEntry);

extern void (*pxyDrawCallback)(void);

/* Installs the callback that func_001CBCE4 calls through pxyDrawCallback (TiWater passes its draw function). */
void pxySetDrawCallback(void (*callback)(void))
{
  pxyDrawCallback = callback;
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BAC2C);

void func_001BAC70(int flags)
{
    D_0096A084 |= flags;
    if (D_0096A084 & 0x40) {
        pxyFlags |= 0x100;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BACD4);

void func_001BADA8(f32 first, f32 second) {
    D_0096A0C8 = first;
    D_0096A0C0 = second;
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BADE8);

void func_001BAE28(int value)
{
  D_0096A0A0 = value;
}

void func_001BAE58(int value)
{
  D_0096A05C = value;
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BAE88);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BAEEC);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BB0D8);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BB144);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BB2FC);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BB368);

void func_001BB3CC(float value)
{
  D_0096A2B8 = value;
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BB3FC);

void func_001BB44C(void *value)
{
  D_0096A00C = value;
}

void func_001BB47C(void *value)
{
  D_0096A0C4 = value;
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BB4AC);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BB628);

void func_001BB678(void *value)
{
  D_0096A040 = value;
}

void xglMatrixCopy_001CC9C4(void *, void *);

void nmlModelSetDrawMatrix(void *matrix) {
    xglMatrixCopy_001CC9C4(&nmlModelDrawMatrix, matrix);
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BB6E8);

void nmlModelSetZwrite(int enabled)
{
    if (enabled) {
        D_00969DB0.fields.render_status |= NML_RENDER_ZWRITE;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", nmlModelSetTransparency);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", nmlModelSetReflTransparency);

void nmlModelSetStencil(int enabled) {
    if (enabled) {
        D_00969DB0.fields.render_status |= NML_RENDER_STENCIL;
    }
}

void func_001BB96C(PxyLight *light)
{
  D_0096A06C = light;
}

void *func_001D2B20(int);
void func_002A9E04(void *, void *, int);

void func_001BB99C(PxyLight *light)
{
    D_0096A06C = func_001D2B20(0x19);
    func_002A9E04(D_0096A06C, light, 0x180);
}

void func_001BB9F4(void) {
    D_00969DB0.fields.render_status |= NML_RENDER_HUMAN_MODEL;
}

extern long long nmlStencilAlphaData;
extern long long nmlStencilAlphaRegister;

/* Stencil passes: stores the GS ALPHA_1 (register 0x42) data word gsAlphaData in nmlStencilAlphaData and the register address in nmlStencilAlphaRegister. */
void nmlModelSetStencilAlpha(long long gsAlphaData)
{
    nmlStencilAlphaData = gsAlphaData;
    nmlStencilAlphaRegister = 0x42;
    nmlRenderStatus |= 0x80;
}

void func_001BBA78(void) {
    D_00969DB0.fields.render_status |= 0x80000;
}

void func_001BBAB0(void) {
    D_00969DB0.fields.render_status |= NML_RENDER_TEXMAP_CIRCLE;
}

void nmlModelSetModel(NmlModel *model)
{
  D_00969DB0.fields.model = model;
}

void func_001BBB18(void) {
    pxyResetGlobals();
}

extern int D_0096A370;

void *func_001BBB48(void) {
    return &D_0096A370;
}

extern int D_0096A31C;

int func_001BBB70(void) {
    return D_0096A31C;
}

void pxyResetState(void);
extern s32 D_0096A304;

/* Frame start for the pxy module: reset the state, clear D_0096A304 (pxyFlush sets it at its end). */
void pxyBeginFrame(void) {
    pxyResetState();
    D_0096A304 = 0;
    pxyClearFlushDone();
}

void func_001BBBD8(void) {
    pxyFlags |= 0x400;
}

void func_001BBC0C(void) {
    pxyFlags |= 0x800;
}

void func_001BBC40(void) {
    pxyFlags |= 0x1000;
}

int func_001BBC74(void) {
    return pxyFlags & 4;
}

int func_001BBCA0(void) {
    return pxyFlags & 1;
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BBCCC);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BBE3C);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BC038);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BC1E0);

/* Append one A+D pair (value, register) to a direct-queue packet. */
extern float D_0096A058;
extern float D_0096A0CC;
extern float D_0096A110;
extern float D_0096A114;
extern float D_0096A118;
extern u16 D_0064583A;
extern s16 cameraScreenWidth;
extern s16 cameraScreenHeight;
float sqrtf(float);
float atan2f(float, float);
void xglVectorCopy_001CC998(void *, void *);
void xglVectorNormal_001A9200(void *, void *);
float xglPointLength(void *, void *);
PxyCamera *xglStudioSelectGetActiveCamera(int);
void _ModelCalcClipInitMat1(void *, void *);
int _ModelCalcClipMat1(void *);
int func_001B73B0(PxyCamera *, void *);
void xglVectorScale(Vector4 *destination, const Vector4 *source, float scale);
void xglVectorScaleXYZ(void *, void *, float);
void xglVectorAddXYZ_001CCA30(void *, void *, void *);
void xglVectorSubXYZ_001CCA68(void *, void *, void *);
void xglVectorMulMat(Vector4 *destination, Matrix4 matrix, const Vector4 *vector);
void xglMatrixStackUnit(void);
void xglMatrixStackInverse(void);
void xglMatrixStackSave(void *);
void xglMatrixStackMul(void *);
void xglMatrixStackRotX(float);
void xglMatrixStackRotY(float);
void xglMatrixStackRotZ(float);
void func_001AA8F4(void *);
void func_001AA8B0(void *);
void func_001A9F88(void *, PxyShadow *, void *);
void func_001A9B50(void *, void *);

/* Aims the shadow projector at the model from the light and builds its matrices and texture registers. */
void pxySetupShadowProjector(PxyHeader *header)
{
    int texWidth = 0x400;
    int texHeight = 0x200;
    PxyShadow *shadow = D_0096A080;
    float rotX;
    float rotY;
    float horizontal;
    Matrix inverse;
    AlignedVector4 scale = { D_0096A318 * 128.0f, D_0096A318 * -128.0f, 8000.0f, 1.0f };
    AlignedVector4 size = { 256.0f, 256.0f, 1.0f, 1.0f };
    AlignedVector4 offset = { 2045.0f, 2045.0f, 0.0f, 1.0f };
    AlignedVector4 direction;
    float zoom = 1.0f;
    float extent = header->radius;

    if (pxyFlags & 0x10) {
        if (D_0096A110 != 0.0f || D_0096A114 != 0.0f || D_0096A118 != 0.0f) {
        }
        if (D_0096A0CC < 139000.0f) {
            D_0096A008 = 0;
            return;
        } else if (D_0096A0CC < 173000.0f) {
            D_0096A058 *= 1.0f - (173000.0f - D_0096A0CC) / 34000.0f;
        }
    }
    if (D_0096A080 == 0) {
        D_0096A080 = func_001D2B20(PXY_SHADOW_QUADWORDS);
    }
    shadow = D_0096A080;
    xglVectorCopy_001CC998(&shadow->center, &D_0096A1B0);
    extent = header->radius * 2.0f;
    if (header->radius >= 5.0f) {
        extent = header->radius * 3.0f;
    }
    if (extent < 0.968f) {
        extent = 0.968f;
    }
    zoom = 0.968f / extent;
    scale.x *= zoom;
    scale.y *= zoom;
    if (D_0096A06C->override.w == 0.0f) {
        direction.x = D_0096A06C->direction.x;
        direction.y = 2.0f;
        direction.z = D_0096A06C->direction.z;
        direction.w = 1.0f;
        if (pxyFlags & 0x10) {
            direction.y = D_0096A06C->direction.y;
        }
    } else {
        direction.x = D_0096A06C->override.x;
        direction.y = D_0096A06C->override.y;
        direction.z = D_0096A06C->override.z;
        direction.w = 1.0f;
    }
    xglVectorNormal_001A9200(&direction, &direction);
    xglVectorCopy_001CC998(&shadow->min, &shadow->center);
    xglVectorCopy_001CC998(&shadow->max, &shadow->center);
    if (pxyFlags & 0x10) {
        float tilt;

        shadow->min.x -= header->radius;
        shadow->min.y -= header->radius;
        shadow->min.z -= header->radius;
        shadow->max.x += header->radius;
        shadow->max.y += header->radius;
        shadow->max.z += header->radius;
        tilt = 1.0f;
        if (direction.y != 0.0f) {
            tilt = __builtin_fabsf((direction.x * direction.x + direction.z * direction.z) / direction.y);
        }
        if (tilt < 1.0f) {
            tilt = 1.0f;
        }
        if (direction.x > 0.0f) {
            shadow->min.x -= direction.x * tilt * header->radius * 1.3f;
            shadow->max.x += direction.x * tilt * header->radius * 0.3f;
        } else {
            shadow->max.x -= direction.x * tilt * header->radius * 1.3f;
            shadow->min.x += direction.x * tilt * header->radius * 0.3f;
        }
        if (direction.z > 0.0f) {
            shadow->min.z -= direction.z * tilt * header->radius * 1.3f;
            shadow->max.z += direction.z * tilt * header->radius * 0.3f;
        } else {
            shadow->max.z -= direction.z * tilt * header->radius * 1.3f;
            shadow->min.z += direction.z * tilt * header->radius * 0.3f;
        }
        if (!(direction.y > 0.0f)) {
            shadow->max.y += header->radius;
            shadow->min.y -= header->radius * 0.5f;
        } else {
            shadow->min.y -= header->radius * 1.5f;
            shadow->max.y -= header->radius * 0.5f;
        }
    } else {
        float spread;
        float side;
        float tilt;
        float lift;

        shadow->min.x -= header->radius;
        shadow->min.y -= header->radius;
        shadow->min.z -= header->radius;
        shadow->max.x += header->radius;
        shadow->max.y += header->radius;
        shadow->max.z += header->radius;
        spread = 1.5f;
        side = 0.3f;
        tilt = 1.0f;
        lift = 0.0f;
        if (header->radius >= 5.0f) {
            lift = header->radius * 1.9f;
        } else if (header->radius >= 2.0f) {
            lift = header->radius;
        }
        if (direction.y != 0.0f) {
            tilt = __builtin_fabsf((direction.x * direction.x + direction.z * direction.z) / direction.y);
        }
        if (tilt < 1.0f) {
            tilt = 1.0f;
        }
        if (direction.x != 0.0f || direction.z != 0.0f) {
            if (direction.x > 0.0f) {
                shadow->min.x -= __builtin_fabsf(direction.x * tilt * header->radius * spread + lift);
                shadow->max.x += __builtin_fabsf(direction.x * tilt * header->radius * side);
            } else {
                shadow->max.x += __builtin_fabsf(direction.x * tilt * header->radius * spread + lift);
                shadow->min.x -= __builtin_fabsf(direction.x * tilt * header->radius * side);
            }
            if (direction.z > 0.0f) {
                shadow->min.z -= __builtin_fabsf(direction.z * tilt * header->radius * spread + lift);
                shadow->max.z += __builtin_fabsf(direction.z * tilt * header->radius * side);
            } else {
                shadow->max.z += __builtin_fabsf(direction.z * tilt * header->radius * spread + lift);
                shadow->min.z -= __builtin_fabsf(direction.z * tilt * header->radius * side);
            }
        }
        if (direction.y > 0.0f) {
            shadow->min.y -= header->radius * 1.5f;
            shadow->max.y -= header->radius * 0.5f;
        } else {
            shadow->max.y += header->radius;
            shadow->min.y -= header->radius * 0.5f;
        }
    }
    shadow->bounds.x = (shadow->max.x + shadow->min.x) * 0.5f;
    shadow->bounds.y = (shadow->max.y + shadow->min.y) * 0.5f;
    shadow->bounds.z = (shadow->max.z + shadow->min.z) * 0.5f;
    shadow->bounds.w = xglPointLength(&shadow->max, &shadow->min) * 0.5f;
    {
        PxyCamera *camera = xglStudioSelectGetActiveCamera(0);

        if (camera != 0) {
            Matrix clip;

            xglMatrixUnit(&clip);
            _ModelCalcClipInitMat1(camera->unk4F0, &clip);
            if (_ModelCalcClipMat1(&shadow->bounds) == 0) {
                nmlRenderStatus &= ~2;
            } else if ((pxyFlags & 0x10) && func_001B73B0(camera, &shadow->bounds)) {
                nmlRenderStatus &= ~2;
            }
            if ((nmlRenderStatus & 0x20) && (nmlRenderStatus & 0x10)) {
                nmlRenderStatus &= ~2;
            }
        }
    }
    horizontal = sqrtf(direction.x * direction.x + direction.z * direction.z);
    rotY = -atan2f(direction.x, -direction.z);
    rotX = atan2f(direction.y, horizontal);
    {
        AlignedVector4 eye = direction;

        xglVectorScale(&eye, &eye, 0.0f);
        xglVectorAddXYZ_001CCA30(&eye, &eye, &shadow->center);
        xglMatrixStackUnit();
        func_001AA8F4(&eye);
        xglMatrixStackRotY(rotY);
        xglMatrixStackRotX(rotX);
        xglMatrixStackInverse();
        xglMatrixStackSave(&shadow->view);
        xglVectorNormal_001A9200(&direction, &direction);
        xglVectorScale(&direction, &direction, 3.0f);
        xglVectorAddXYZ_001CCA30(&direction, &direction, &shadow->center);
        xglMatrixStackUnit();
        func_001AA8F4(&direction);
        xglMatrixStackRotY(rotY);
        xglMatrixStackRotX(rotX);
        xglMatrixStackRotZ(1.5707964f);
        xglMatrixStackSave(&shadow->light);
        xglMatrixStackInverse();
        xglMatrixStackSave(&inverse);
        xglMatrixStackUnit();
        offset.x -= (cameraScreenWidth - size.x) * 0.5f;
        offset.y -= (cameraScreenHeight - size.y) * 0.5f;
        func_001AA8F4(&offset);
        func_001AA8B0(&scale);
        xglMatrixStackMul(&inverse);
        xglMatrixStackSave(&shadow->project);
        memset(&shadow->lighting, 0, sizeof(shadow->lighting));
        if (D_0096A008 == 4) {
            shadow->unk338 = 0x42;
        } else {
            shadow->unk338 = 0x89;
        }
        shadow->x = 0;
        shadow->y = 0;
        shadow->width = 0x100;
        shadow->height = 0x100;
        shadow->repeatU = 0;
        shadow->repeatV = 0;
        shadow->frameBase = D_0064583A;
        shadow->scaleU = 64.0f / (zoom * 128.0f);
        shadow->scaleV = 64.0f / (zoom * 128.0f);
        shadow->unk330 = 0;
        shadow->clutOffset = 0;
        shadow->psm = GS_PSMCT32;
        {
            AlignedVector4 depth = { 0.0f, 0.0f, 1000.0f, 1.0f };
            Matrix texture;
            int wrapU;
            int wrapV;
            int minU;
            int maxU;
            int minV;
            int maxV;

            scale.x = (float)shadow->width * 0.015625f * shadow->scaleU;
            scale.y = (float)shadow->height * 0.015625f * shadow->scaleV;
            scale.z = scale.w = 1.0f;
            func_001A9F88(&texture, shadow, &scale);
            xglVectorMulMat(&shadow->unk2D0, texture.rows, &depth);
            xglVectorSubXYZ_001CCA68(&shadow->direction, &shadow->unk2D0, texture.rows[3]);
            func_001A9B50(&shadow->unk040, &texture);
            xglVectorNormal_001A9200(&shadow->direction, &shadow->direction);
            switch (shadow->psm) {
            case GS_PSMT4:
                shadow->tex0 = GS_SET_TEX0(shadow->frameBase << 5, 16, GS_PSMT4, 10, 9, 1, 0,
                                           (shadow->frameBase << 5) + shadow->clutOffset, 0, 0, 0, 1);
                break;
            case GS_PSMT8:
                shadow->tex0 = GS_SET_TEX0(shadow->frameBase << 5, 16, GS_PSMT8, 10, 9, 1, 0,
                                           (shadow->frameBase << 5) + shadow->clutOffset, 0, 0, 0, 1);
                break;
            case GS_PSMCT32:
                shadow->tex0 = GS_SET_TEX0(shadow->frameBase << 5, 8, GS_PSMCT32, 9, 9, 1, 0, 0, 0, 0, 0, 1);
                texWidth = 0x200;
                texHeight = 0x200;
                break;
            }
            wrapU = shadow->repeatU ? GS_REGION_REPEAT : GS_REGION_CLAMP;
            wrapV = shadow->repeatV ? GS_REGION_REPEAT : GS_REGION_CLAMP;
            minU = shadow->repeatU ? shadow->width - 1 : shadow->x;
            maxU = shadow->repeatU ? shadow->x : shadow->x + shadow->width - 1;
            minV = shadow->repeatV ? shadow->height - 1 : shadow->y;
            maxV = shadow->repeatV ? shadow->y : shadow->y + shadow->height - 1;
            shadow->clamp = GS_SET_CLAMP(wrapU, wrapV, minU, maxU, minV, maxV);
            shadow->uv.y = (float)shadow->width / (float)texWidth;
            shadow->uv.x = (float)shadow->x / (float)texWidth + shadow->uv.y * 0.5f;
            shadow->uv.w = -(float)shadow->height / (float)texHeight;
            shadow->uv.z = (float)shadow->y / (float)texHeight - shadow->uv.w * 0.5f;
            xglVectorScaleXYZ(&shadow->direction, &shadow->direction, -1.0f);
            shadow->lighting.direction.x = shadow->direction.x;
            shadow->lighting.direction.y = shadow->direction.y;
            shadow->lighting.direction.z = shadow->direction.z;
            shadow->lighting.color[0] = shadow->lighting.color[1] = shadow->lighting.color[2] = 1.0f;
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BDC48);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BDD6C);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BE5AC);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BEA8C);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BEBCC);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BECEC);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BEE5C);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BF22C);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BF3F0);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BF870);

extern float D_0096A094;
extern AlignedVector4 D_0096A190;
extern AlignedVector4 D_0096A1A0;
extern int D_0096A2B0;
extern AlignedVector4 D_0096A380;
extern AlignedVector4 D_0096A390;
extern PxyBatchTable D_0096A6A0[2];
extern PxyBatchTable D_0096C6B0;
extern PxyDrawEntry D_0097A980[];
void func_001BF870(PxyHeader *);
void func_001BDD6C(PxyHeader *);
int strcmp(const char *, const char *);
void xglMatrixStackLoad(void *);
void xglMatrixStackMulVector(void *, void *);
void func_001BF22C(PxyHeader *);
void func_001BECEC(PxyHeader *);
void func_001BEE5C(void);
void func_001BF3F0(PxyHeader *);
PxyPacket *xglPacketAppendVectors(void *, int);
void func_001BE5AC(PxySlot *, PxyPacket *);

/* Validates a "pxy" model header and queues its parts as opaque and translucent draw batches (shadow, light and fade setup included). */
int pxySubmitModel(PxyHeader *header)
{
    PxyCamera *camera;
    PxySlot *newSlot;
    PxySlot *slot;
    PxyRow vector;
    void *ptr;
    int i;
    int j;
    float fadeFullDistance;
    float fadeZeroDistance;
    float fadeFactor;
    Vector4 direction;
    PxyDrawEntry *entry;
    PxyDrawEntry *translucentEntry;
    PxyBatchTable *table;
    PxyBatch *batch;
    PxyBatchTable *translucentTable;
    PxyBatch *translucentBatch;
    int a;
    int b;
    int c;
    int d;
    PxySortWork work;
    PxySortWork *w;

    camera = xglStudioSelectGetActiveCamera(D_0096A004);
    newSlot = 0;
    slot = 0;
    D_0096A31C = 0;
    if (camera == 0) {
        pxyResetGlobals();
        return 1;
    }
    if (D_0096A2BC >= PXY_BATCH_MAX || D_0096A6A0[0].count >= PXY_BATCH_MAX) {
        pxyResetGlobals();
        return 1;
    }
    if ((u32)header & 0xF) {
        pxyResetGlobals();
        return 1;
    }
    if (header == 0 || header->magic[0] != 'p' || header->magic[1] != 'x' || header->magic[2] != 'y'
        || (int)(header->version * 10.0f) <= 0) {
        pxyResetGlobals();
        return 1;
    }
    func_001BF870(header);
    func_001BDD6C(header);
    if ((header->flags & 0x40) && (pxyFlags & 2)) {
        pxyFlags |= 1;
    }
    if ((header->flags & 0x100) && (pxyFlags & 8)) {
        pxyFlags |= 4;
    }
    if (header->flags & 0x800) {
        pxyFlags |= 0x8000;
    }
    if ((pxyFlags & 0x20) && !(D_0096A084 & 0x100)) {
        D_0096A1A0 = D_0096A390;
        D_0096A190 = D_0096A380;
        nmlRenderStatus |= 1;
    }
    if (header->flags & 2) {
        if (D_0096A008 == 1 || D_0096A008 == 5 || D_0096A008 == 4) {
            D_0096A084 |= 0x80;
        }
    }
    if (D_0096A084 & 0x10) {
        D_0096A09C = -1;
    }
    if (pxyFlags & 0x80) {
        D_0096A190.x = 0;
        D_0096A190.y = 0;
        D_0096A190.z = 0;
    }
    if (pxyFlags & 0x400) {
        nmlRenderStatus |= 0x40000;
    }
    if (nmlRenderStatus & 0x400) {
        nmlRenderStatus |= 1;
        D_0096A190.x = 255.0f;
        D_0096A190.y = 255.0f;
        D_0096A190.z = 255.0f;
    }
    if (D_0096A06C->override.w == 0.0f) {
        vector.v[0] = D_0096A06C->direction.x;
        vector.v[2] = D_0096A06C->direction.z;
    } else {
        vector.v[0] = D_0096A06C->override.x;
        vector.v[2] = D_0096A06C->override.z;
    }
    if (vector.v[0] == 0.0f && vector.v[2] == 0.0f) {
        nmlRenderStatus |= 0x20;
    }
    if (pxyFlags & 0x80000) {
        if (strcmp(header->name, "MOM2_B04") == 0) {
            D_0096A324 = 1;
        } else if (D_0096A324 != 0) {
            ptr = &pxyState.light;
            ((PxyLight *)ptr)->drawDirection.x = ((PxyLight *)ptr)->savedDirection.x;
            ((PxyLight *)ptr)->drawDirection.y = ((PxyLight *)ptr)->savedDirection.y;
            ((PxyLight *)ptr)->drawDirection.z = ((PxyLight *)ptr)->savedDirection.z;
            ptr = D_0096A06C;
            ((PxyLight *)ptr)->drawDirection.x = ((PxyLight *)ptr)->savedDirection.x;
            ((PxyLight *)ptr)->drawDirection.y = ((PxyLight *)ptr)->savedDirection.y;
            ((PxyLight *)ptr)->drawDirection.z = ((PxyLight *)ptr)->savedDirection.z;
        }
    }
    D_0096A2BC++;
    ptr = 0;
    D_0096A010 = header;
    D_0096A020 = (PxyPart *)((char *)header + header->partOffset);
    D_0096A014 = (PxyPartRange *)((char *)header + header->rangeOffset);
    D_0096A018 = (char *)header + header->offset58;
    D_0096A01C = (char *)header + header->offset6C;
    D_0096A03C = (char *)header + header->offset3C;
    D_0096A024 = (char *)header + header->offset68;
    D_0096A028 = (char *)header + header->offset70;
    D_0096A030 = (char *)header + header->offset64;
    D_0096A050 = (char *)header + header->offset74;
    D_0096A054 = (char *)header + header->offset78;
    if ((nmlRenderStatus & 0x800) || (header->flags & 0x88)) {
        if (nmlRenderStatus & 1) {
            D_0096A068 = func_001D2B20(header->geometryCount);
        }
    }
    D_0096A034 = func_001D2B20(header->partCount / 16 + 1);
    xglVectorScaleXYZ(&D_0096A1B0, &D_0096A1B0, D_0096A088);
    memset(D_0096A034, PXY_PART_VISIBLE, header->partCount);
    if (D_0096A05C == 0) {
        D_0096A05C = D_0096A024;
    }
    func_001A9B50(&D_0096A120, &nmlModelDrawMatrix);
    if ((nmlRenderStatus & 0xE000) && D_0096A0A8 != 0) {
        vector = (PxyRow){ { 0.0f, 0.0f, 1.0f, 0.0f } };
        for (i = 0; i < D_0096A0A4; i++) {
            D_0096A0A8->plane[i].v[3] = 1.0f;
            xglMatrixStackLoad(&D_0096A120);
            xglMatrixStackMulVector(&D_0096A0A8->plane[i], &D_0096A0A8->plane[i]);
            xglMatrixStackRotY(D_0096A0A8->angle[i].v[1]);
            xglMatrixStackRotX(D_0096A0A8->angle[i].v[0]);
            xglMatrixStackMulVector(&vector, &vector);
            vector.v[3] = -(vector.v[0] * D_0096A0A8->plane[i].v[0] + vector.v[1] * D_0096A0A8->plane[i].v[1]
                            + vector.v[2] * D_0096A0A8->plane[i].v[2]);
            D_0096A0A8->plane[i] = vector;
            D_0096A0A8->angle[i].v[0] = D_0096A0A8->angle[i].v[2];
            D_0096A0A8->angle[i].v[1] = D_0096A0A8->angle[i].v[3];
            D_0096A0A8->angle[i].v[2] = D_0096A0A8->angle[i].v[0] + D_0096A0A8->angle[i].v[1];
        }
    }
    if (D_0096A040 != 0) {
        i = D_0096A2B0;
        for (j = PXY_SLOT_COUNT; j > 0; j--) {
            if (--i < 0) {
                i = PXY_SLOT_COUNT - 1;
            }
            if (pxyState.slots[i].packet != 0 && pxyState.slots[i].packet->header == D_0096A040) {
                slot = &pxyState.slots[i];
                ptr = slot->packet;
                D_0096A06C = ((PxyPacket *)ptr)->light;
                D_0096A080 = ((PxyPacket *)ptr)->shadow;
                D_0096A098 = ((PxyPacket *)ptr)->unk98;
                if (D_0096A038 == 0) {
                    D_0096A038 = ((PxyPacket *)ptr)->unk38;
                }
                break;
            }
        }
        func_001BF22C(header);
    } else {
        newSlot = &pxyState.slots[D_0096A2B0++];
        if (D_0096A2B0 >= PXY_SLOT_COUNT) {
            D_0096A2B0 = 0;
        }
        func_001BECEC(header);
        func_001BEE5C();
        func_001BF22C(header);
    }
    if ((pxyFlags & 0x80010) && (header->flags & 2)) {
        fadeFullDistance = 173000.0f;
        fadeZeroDistance = 139000.0f;
        if (pxyFlags & 0x80000) {
            fadeFullDistance = 1073000.0f;
            fadeZeroDistance = 839000.0f;
        }
        if (D_0096A0CC < fadeZeroDistance) {
            D_0096A044 = 0;
            D_0096A0D8 = 0;
        } else if (D_0096A0CC < fadeFullDistance) {
            D_0096A044 *= 1.0f - (fadeFullDistance - D_0096A0CC) / (fadeFullDistance - fadeZeroDistance);
            D_0096A0D8 *= 1.0f - (fadeFullDistance - D_0096A0CC) / (fadeFullDistance - fadeZeroDistance);
        }
        if (D_0096A084 & 0x800) {
            D_0096A044 = 0;
        }
        if (D_0096A044 == 0.0f) {
            D_0096A084 |= 0x800;
        }
    }
    if (D_0096A084 & 0x1000) {
        fadeZeroDistance = 223000.0f;
        fadeFullDistance = 209000.0f;
        fadeFactor = 1.0f;
        if (D_0096A0CC < fadeFullDistance) {
            fadeFactor = 0;
        } else if (D_0096A0CC < fadeZeroDistance) {
            fadeFactor *= 1.0f - (fadeZeroDistance - D_0096A0CC) / (fadeZeroDistance - fadeFullDistance);
        }
        if (fadeFactor <= 0.0f) {
            pxyResetGlobals();
            return 0;
        }
        if (fadeFactor < 1.0f) {
            nmlRenderStatus |= 4;
            D_0096A044 *= fadeFactor;
            D_0096A048 *= fadeFactor;
            D_0096A04C *= fadeFactor;
            D_0096A0D8 *= fadeFactor;
        }
    }
    if (!(nmlRenderStatus & 0x10) || (nmlRenderStatus & 2)) {
        func_001BF3F0(header);
        if (header->geometryCount < 0x100 && D_0096A00C != 0) {
            i = (int)D_0096A020;
            for (j = 0; j < header->partCount; i += sizeof(PxyPart), j++) {
                if (D_0096A00C->bits[(s16)(((PxyPart *)i)->maskBit >> 5)] & (1 << (s16)(((PxyPart *)i)->maskBit & 0x1F))) {
                    D_0096A034[j] = D_0096A034[j] & 0x7F;
                }
            }
        }
    }
    if (header->unk5C != 0 && D_0096A02C == 0) {
        D_0096A02C = D_0096A028;
    }
    if (D_0096A040 == 0) {
        switch (D_0096A008) {
        case 1:
        case 4:
        case 5:
            if (!(header->flags & 1)) {
                pxySetupShadowProjector(header);
            }
            break;
        case 3:
            D_0096A098 = func_001D2B20(4);
            xglMatrixUnit(D_0096A098);
            if (D_0096A06C->override.w == 0.0f) {
                direction.x = D_0096A06C->direction.x;
                direction.y = 2.0f;
                direction.z = D_0096A06C->direction.z;
                if (pxyFlags & 0x10) {
                    direction.y = D_0096A06C->direction.y;
                }
            } else {
                direction.x = D_0096A06C->override.x;
                direction.y = D_0096A06C->override.y;
                direction.z = D_0096A06C->override.z;
            }
            xglVectorNormal_001A9200(&direction, &direction);
            if (direction.y != 0.0f) {
                D_0096A098->m[1][0] = -direction.x / direction.y;
                D_0096A098->m[1][2] = -direction.z / direction.y;
            } else {
                D_0096A098->m[1][0] = 0;
                D_0096A098->m[1][2] = 0;
            }
            D_0096A098->m[1][1] = 0;
            D_0096A098->m[3][1] = D_0096A114 + 0.005f;
            D_0096A098->m[3][0] = -D_0096A114 * D_0096A098->m[1][0];
            D_0096A098->m[3][2] = -D_0096A114 * D_0096A098->m[1][2];
            D_0096A098->m[3][1] = D_0096A094 + 0.005f;
            D_0096A098->m[3][0] = -D_0096A094 * D_0096A098->m[1][0];
            D_0096A098->m[3][2] = -D_0096A094 * D_0096A098->m[1][2];
            break;
        case 2:
            break;
        }
    }
    j = (int)xglPacketAppendVectors(&nmlRenderStatus, 0x2A);
    i = (int)((PxyPacket *)j)->ranges;
    ptr = &((PxyPacket *)j)->ranges[1];
    entry = &D_0097A980[D_0096A308];
    translucentEntry = &D_0097A980[D_0096A30C];
    table = &D_0096A6A0[0];
    batch = &table->batches[table->count];
    translucentTable = &D_0096C6B0;
    translucentBatch = &translucentTable->batches[translucentTable->count];
    if (slot == 0) {
        batch->start = D_0096A308;
        batch->count = 0;
        batch->depth = xglPointLength(&((PxyPacket *)j)->center, &camera->position) + D_0096A2B8;
        batch->flags = ((PxyPacket *)j)->renderStatus;
        batch->unk10 = -1;
        batch->next = -1;
        batch->packet = (PxyPacket *)j;
        batch->translucent = translucentBatch;
        translucentBatch->start = D_0096A30C;
        translucentBatch->count = 0;
        translucentBatch->depth = 0;
        translucentBatch->flags = ((PxyPacket *)j)->renderStatus;
        translucentBatch->unk10 = -1;
        translucentBatch->next = -1;
        translucentBatch->packet = (PxyPacket *)j;
    } else {
        batch = &table->batches[slot->batch];
        translucentBatch = &translucentTable->batches[slot->translucentBatch];
        if (!(translucentBatch->flags & 4) && (((PxyPacket *)j)->renderStatus & 4)) {
            translucentBatch->flags |= 0x80000000;
        }
        batch->flags |= 0x400000;
        translucentBatch->flags |= 0x400000;
        translucentBatch->flags |= ((PxyPacket *)j)->renderStatus & 8;
        entry = &D_0097A980[batch->start + batch->count];
        translucentEntry = &D_0097A980[translucentBatch->start + translucentBatch->count];
    }
    if ((nmlRenderStatus & 0x8800) || (header->flags & 0x88)) {
        batch->flags |= 0x40000000;
        translucentBatch->flags |= 0x40000000;
    }
    if (header->flags & 0x40) {
        batch->flags |= 0x4000000;
        translucentBatch->flags |= 0x4000000;
    }
    if (header->flags & 0x100) {
        batch->flags |= 0x2000000;
        translucentBatch->flags |= 0x2000000;
    }
    if ((header->flags & 0x800) || (nmlRenderStatus & 0x20000)) {
        batch->flags |= 0x20000;
        translucentBatch->flags |= 0x20000;
    }
    if (D_0096A084 & 0x40) {
        batch->flags |= 0x100000;
        translucentBatch->flags |= 0x100000;
    }
    if (header->flags & 0x200) {
        batch->flags |= 0x800000;
        translucentBatch->flags |= 0x800000;
    }
    if (((PxyPacket *)j)->unk68 != 0) {
        batch->flags |= 0x80000000;
        translucentBatch->flags |= 0x80000000;
    }
    if ((header->flags & 0x20) || (nmlRenderStatus & 8)) {
        batch->flags |= 0x8000000;
        translucentBatch->flags |= 0x8000000;
    }
    if (!(((PxyPacket *)j)->renderStatus & 4)) {
        for (a = 0; a < header->layerCount; a++) {
            c = (int)&((PxyPacket *)j)->parts[((PxyPartRange *)i)->first];
            d = ((PxyPartRange *)i)->count;
            if (!(nmlRenderStatus & 0x10) || (nmlRenderStatus & 2)) {
                for (b = 0; b < d; b++, c += sizeof(PxyPart)) {
                    if (((PxyPart *)c)->layer == a) {
                        if (!(((PxyPacket *)j)->partFlags[((PxyPart *)c)->index] & PXY_PART_VISIBLE)) {
                            continue;
                        }
                        if ((header->flags & 1) && !(((PxyPacket *)j)->partFlags[((PxyPart *)c)->index] & PXY_PART_LOD_MASK)) {
                            continue;
                        }
                        if (batch->start + batch->count >= PXY_OPAQUE_ENTRY_END) {
                            break;
                        }
                        entry->part = (PxyPart *)c;
                        (entry++)->packet = (PxyPacket *)j;
                        batch->count++;
                    }
                }
            }
        }
    }
    if (((PxyPacket *)j)->renderStatus & 4) {
        for (d = 0; d < header->layerCount; d++) {
            b = (int)&((PxyPacket *)j)->parts[((PxyPartRange *)i)->first];
            a = ((PxyPartRange *)i)->count;
            if (!(nmlRenderStatus & 0x10) || (nmlRenderStatus & 2)) {
                for (c = 0; c < a; c++, b += sizeof(PxyPart)) {
                    if (((PxyPart *)b)->layer == d) {
                        if (!(((PxyPacket *)j)->partFlags[((PxyPart *)b)->index] & PXY_PART_VISIBLE)) {
                            continue;
                        }
                        if (((PxyPart *)b)->flags & 0x10) {
                            continue;
                        }
                        if ((header->flags & 1) && !(((PxyPacket *)j)->partFlags[((PxyPart *)b)->index] & PXY_PART_LOD_MASK)) {
                            continue;
                        }
                        if (translucentBatch->start + translucentBatch->count >= PXY_TRANSLUCENT_ENTRY_END) {
                            break;
                        }
                        translucentEntry->part = (PxyPart *)b;
                        (translucentEntry++)->packet = (PxyPacket *)j;
                        translucentBatch->count++;
                    }
                }
            }
        }
    }
    if (!(((PxyPacket *)j)->unkD4 & 1)) {
        if (!(nmlRenderStatus & 0x10) || (nmlRenderStatus & 2)) {
            for (d = header->layerCount - 1; d >= 0; d--) {
                b = (int)&((PxyPacket *)j)->parts[((PxyPartRange *)ptr)->first];
                if (header->flags & 0x200) {
                    for (c = 0; c < ((PxyPartRange *)ptr)->count; c++, b += sizeof(PxyPart)) {
                        if (((PxyPart *)b)->layer == d) {
                            if (((PxyPacket *)j)->partFlags[((PxyPart *)b)->index] & PXY_PART_MERGED) {
                                continue;
                            }
                            if (((((PxyPart *)b)->flags & 0x80) || !(((PxyPart *)b)->flags & 0x40))
                                && !(((PxyPart *)b)->flags & 0x200)) {
                                if (!(((PxyPart *)b)->flags & 0x240)) {
                                    if (!(((PxyPacket *)j)->partFlags[((PxyPart *)b)->index] & PXY_PART_VISIBLE)) {
                                        continue;
                                    }
                                    if ((header->flags & 1)
                                        && !(((PxyPacket *)j)->partFlags[((PxyPart *)b)->index] & PXY_PART_LOD_MASK)) {
                                        continue;
                                    }
                                }
                                if (translucentBatch->start + translucentBatch->count >= PXY_TRANSLUCENT_ENTRY_END) {
                                    break;
                                }
                                translucentEntry->part = (PxyPart *)b;
                                (translucentEntry++)->packet = (PxyPacket *)j;
                                translucentBatch->count++;
                                if (((PxyPart *)b)->flags & 0x80) {
                                    for (a = c + 1; a < ((PxyPartRange *)ptr)->count; a++) {
                                        if (((PxyPart *)b + 1)->flags & 0x80) {
                                            break;
                                        }
                                        if (((PxyPart *)b + 1)->flags & 0x240) {
                                            if (translucentBatch->start + translucentBatch->count >= PXY_TRANSLUCENT_ENTRY_END) {
                                                break;
                                            }
                                            c++;
                                            b += sizeof(PxyPart);
                                            translucentEntry->part = (PxyPart *)b;
                                            (translucentEntry++)->packet = (PxyPacket *)j;
                                            ((PxyPacket *)j)->partFlags[((PxyPart *)b)->index] =
                                                ((PxyPacket *)j)->partFlags[((PxyPart *)b)->index] | PXY_PART_MERGED;
                                            translucentBatch->count++;
                                        } else {
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (((PxyPacket *)j)->unk84 & 2) {
                        break;
                    }
                } else {
                    a = ((PxyPartRange *)ptr)->count;
                    if (!(header->flags & 0x1000) || header->sortEntryOffset == 0) {
                        for (c = 0; c < a; c++, b += sizeof(PxyPart)) {
                            if (((PxyPart *)b)->layer == d) {
                                if (!(((PxyPacket *)j)->partFlags[((PxyPart *)b)->index] & PXY_PART_VISIBLE)) {
                                    continue;
                                }
                                if ((header->flags & 1)
                                    && !(((PxyPacket *)j)->partFlags[((PxyPart *)b)->index] & PXY_PART_LOD_MASK)) {
                                    continue;
                                }
                                if (translucentBatch->start + translucentBatch->count >= PXY_TRANSLUCENT_ENTRY_END) {
                                    break;
                                }
                                translucentEntry->part = (PxyPart *)b;
                                (translucentEntry++)->packet = (PxyPacket *)j;
                                translucentBatch->count++;
                            }
                        }
                    } else {
                        w = &work;
                        w->sortEntries = (PxySortEntry *)((char *)header + header->sortEntryOffset);
                        w->entryCount = 0;
                        w->group = w->groups;
                        w->entries = translucentEntry;
                        w->groupCount = 0;
                        w->group->count = 0;
                        w->item = w->group->items;
                        w->item->count = 0;
                        w->matrixSet = D_0096A0AC;
                        w->matrices = D_0096A02C;
                        xglMatrixCopy_001CC9C4(w->matrix, &nmlModelDrawMatrix);
                        xglVectorCopy_001CC998(&w->eye, &camera->position);
                        for (c = 0; c < a; c++, b += sizeof(PxyPart)) {
                            if (((PxyPart *)b)->layer == d) {
                                if (!(((PxyPacket *)j)->partFlags[((PxyPart *)b)->index] & PXY_PART_VISIBLE)) {
                                    continue;
                                }
                                if ((header->flags & 1)
                                    && !(((PxyPacket *)j)->partFlags[((PxyPart *)b)->index] & PXY_PART_LOD_MASK)) {
                                    continue;
                                }
                                if (translucentBatch->start + translucentBatch->count >= PXY_TRANSLUCENT_ENTRY_END) {
                                    break;
                                }
                                translucentEntry->part = (PxyPart *)b;
                                (translucentEntry++)->packet = (PxyPacket *)j;
                                translucentBatch->count++;
                                w->entryCount++;
                            }
                        }
                        {
                            for (w->i = 0; w->i < w->entryCount; w->i++) {
                                w->sortEntry = &w->sortEntries[w->entries[w->i].part->maskBit];
                                if (w->sortEntry->key >= 0) {
                                    if (w->item->count == 0 || w->item->key != w->sortEntry->key) {
                                        if (w->item->count != 0) {
                                            w->item++;
                                            w->group->count++;
                                        }
                                        w->item->start = w->i;
                                        w->item->count = 1;
                                        w->item->key = w->sortEntry->key;
                                        w->item->matrix = w->sortEntry->matrix;
                                        xglVectorCopy_001CC998(w->item, w->sortEntry);
                                    } else {
                                        w->item->count++;
                                    }
                                    if (w->i == w->entryCount - 1) {
                                        w->group->count++;
                                        w->group++;
                                        w->groupCount++;
                                        w->group->count = 0;
                                        w->item = w->group->items;
                                        w->item->count = 0;
                                    }
                                } else if (w->item->count != 0) {
                                    w->group->count++;
                                    w->group++;
                                    w->groupCount++;
                                    w->group->count = 0;
                                    w->item = w->group->items;
                                    w->item->count = 0;
                                }
                            }
                        }
                        w->group = w->groups;
                        for (w->i = 0; w->i < w->groupCount; w->i++, w->group++) {
                            w->item = w->group->items;
                            w->group->start = w->group->items[0].start;
                            w->group->entryCount = 0;
                            for (w->j = 0; w->j < w->group->count; w->j++, w->item++) {
                                w->group->entryCount += w->item->count;
                                w->group->sorted[w->j] = w->item;
                                w->item->position.w = 1.0f;
                                if (w->item->matrix >= 0 && w->matrixSet != 0 && w->matrices != 0) {
                                    xglMatrixStackLoad(w->matrix);
                                    xglMatrixStackMul(w->matrices[w->item->matrix]);
                                    xglMatrixStackMulVector(w->item, w->item);
                                    w->item->depth = xglPointLength(w->item, &w->eye);
                                } else {
                                    xglMatrixStackLoad(w->matrix);
                                    xglMatrixStackMulVector(w->item, w->item);
                                    w->item->depth = xglPointLength(w->item, &w->eye);
                                }
                            }
                            {
                                do {
                                    w->swapped = 0;
                                    w->cursor = w->group->sorted;
                                    for (w->j = 0; w->j < w->group->count - 1; w->j++, w->cursor++) {
                                        if (w->cursor[0]->depth < (*(w->cursor + 1))->depth) {
                                            w->swapped = *w->cursor;
                                            *w->cursor = *(w->cursor + 1);
                                            *(w->cursor + 1) = w->swapped;
                                        }
                                    }
                                } while (w->swapped != 0);
                            }
                            w->out = w->buffer;
                            w->cursor = w->group->sorted;
                            for (w->j = 0; w->j < w->group->count; w->j++, w->cursor++) {
                                func_002A9E04(w->out, &w->entries[(*w->cursor)->start], (*w->cursor)->count * 8);
                                w->out += (*w->cursor)->count;
                            }
                            func_002A9E04(&w->entries[w->group->start], w->buffer, w->group->entryCount * 8);
                        }
                    }
                }
            }
        }
    }
    func_001BE5AC(newSlot, (PxyPacket *)j);
    D_0096A308 += batch->count;
    D_0096A30C += translucentBatch->count;
    pxyResetGlobals();
    return 0;
}

void func_001BEA8C(PxySlot *, PxyPacket *);

/* Reduced variant of pxySubmitModel: no header validation, shadow or fade setup; queues the parts as draw batches. */
int pxySubmitModelBasic(PxyHeader *header)
{
    PxyCamera *camera;
    PxySlot *newSlot;
    int j;
    int i;
    void *ptr;
    PxyDrawEntry *entry;
    PxyDrawEntry *translucentEntry;
    PxyBatchTable *table;
    PxyBatch *batch;
    PxyBatchTable *translucentTable;
    PxyBatch *translucentBatch;
    int a;
    int b;
    int c;
    int d;

    camera = xglStudioSelectGetActiveCamera(D_0096A004);
    newSlot = 0;
    D_0096A31C = 0;
    if (D_0096A2BC >= PXY_BATCH_MAX || D_0096A6A0[0].count >= PXY_BATCH_MAX) {
        pxyResetGlobals();
        return 1;
    }
    func_001BDD6C(header);
    if ((header->flags & 0x100) && (pxyFlags & 8)) {
        pxyFlags |= 4;
    }
    if (D_0096A084 & 0x10) {
        D_0096A09C = -1;
    }
    if (pxyFlags & 0x80) {
        D_0096A190.x = 0;
        D_0096A190.y = 0;
        D_0096A190.z = 0;
    }
    if (pxyFlags & 0x400) {
        nmlRenderStatus |= 0x40000;
    }
    D_0096A2BC++;
    D_0096A010 = header;
    D_0096A020 = (PxyPart *)((char *)header + header->partOffset);
    D_0096A014 = (PxyPartRange *)((char *)header + header->rangeOffset);
    D_0096A018 = (char *)header + header->offset58;
    D_0096A01C = (char *)header + header->offset6C;
    D_0096A03C = (char *)header + header->offset3C;
    D_0096A024 = (char *)header + header->offset68;
    D_0096A028 = (char *)header + header->offset70;
    D_0096A030 = (char *)header + header->offset64;
    D_0096A050 = (char *)header + header->offset74;
    D_0096A054 = (char *)header + header->offset78;
    D_0096A034 = func_001D2B20(header->partCount / 16 + 1);
    xglVectorScaleXYZ(&D_0096A1B0, &D_0096A1B0, D_0096A088);
    memset(D_0096A034, PXY_PART_VISIBLE, header->partCount);
    D_0096A05C = D_0096A024;
    func_001A9B50(&D_0096A120, &nmlModelDrawMatrix);
    newSlot = &pxyState.slots[D_0096A2B0++];
    if (D_0096A2B0 >= PXY_SLOT_COUNT) {
        D_0096A2B0 = 0;
    }
    func_001BECEC(header);
    func_001BEE5C();
    if (!(nmlRenderStatus & 0x10) || (nmlRenderStatus & 2)) {
        func_001BF3F0(header);
        if (header->geometryCount < 0x100 && D_0096A00C != 0) {
            i = (int)D_0096A020;
            for (j = 0; j < header->partCount; i += sizeof(PxyPart), j++) {
                if (D_0096A00C->bits[(s16)(((PxyPart *)i)->maskBit >> 5)] & (1 << (s16)(((PxyPart *)i)->maskBit & 0x1F))) {
                    D_0096A034[j] = D_0096A034[j] & 0x7F;
                }
            }
        }
    }
    i = (int)xglPacketAppendVectors(&nmlRenderStatus, 0x2A);
    j = (int)((PxyPacket *)i)->ranges;
    ptr = &((PxyPacket *)i)->ranges[1];
    entry = &D_0097A980[D_0096A308];
    translucentEntry = &D_0097A980[D_0096A30C];
    table = &D_0096A6A0[0];
    batch = &table->batches[table->count];
    translucentTable = &D_0096C6B0;
    translucentBatch = &translucentTable->batches[translucentTable->count];
    batch->start = D_0096A308;
    batch->count = 0;
    batch->depth = xglPointLength(&((PxyPacket *)i)->center, &camera->position) + D_0096A2B8;
    batch->flags = ((PxyPacket *)i)->renderStatus;
    batch->unk10 = -1;
    batch->next = -1;
    batch->packet = (PxyPacket *)i;
    batch->translucent = translucentBatch;
    translucentBatch->start = D_0096A30C;
    translucentBatch->count = 0;
    translucentBatch->depth = 0;
    translucentBatch->flags = ((PxyPacket *)i)->renderStatus;
    translucentBatch->unk10 = -1;
    translucentBatch->next = -1;
    translucentBatch->packet = (PxyPacket *)i;
    if (header->flags & 0x100) {
        batch->flags |= 0x2000000;
        translucentBatch->flags |= 0x2000000;
    }
    if (D_0096A084 & 0x40) {
        batch->flags |= 0x100000;
        translucentBatch->flags |= 0x100000;
    }
    if ((header->flags & 0x20) || (nmlRenderStatus & 8)) {
        batch->flags |= 0x8000000;
        translucentBatch->flags |= 0x8000000;
    }
    if (!(((PxyPacket *)i)->renderStatus & 4)) {
        for (a = 0; a < header->layerCount; a++) {
            c = (int)&((PxyPacket *)i)->parts[((PxyPartRange *)j)->first];
            d = ((PxyPartRange *)j)->count;
            if (!(nmlRenderStatus & 0x10) || (nmlRenderStatus & 2)) {
                for (b = 0; b < d; b++, c += sizeof(PxyPart)) {
                    if (((PxyPart *)c)->layer == a) {
                        if (!(((PxyPacket *)i)->partFlags[((PxyPart *)c)->index] & PXY_PART_VISIBLE)) {
                            continue;
                        }
                        if (batch->start + batch->count >= PXY_OPAQUE_ENTRY_END) {
                            break;
                        }
                        entry->part = (PxyPart *)c;
                        (entry++)->packet = (PxyPacket *)i;
                        batch->count++;
                    }
                }
            }
        }
    }
    if (((PxyPacket *)i)->renderStatus & 4) {
        for (d = 0; d < header->layerCount; d++) {
            b = (int)&((PxyPacket *)i)->parts[((PxyPartRange *)j)->first];
            a = ((PxyPartRange *)j)->count;
            if (!(nmlRenderStatus & 0x10) || (nmlRenderStatus & 2)) {
                for (c = 0; c < a; c++, b += sizeof(PxyPart)) {
                    if (((PxyPart *)b)->layer == d) {
                        if (!(((PxyPacket *)i)->partFlags[((PxyPart *)b)->index] & PXY_PART_VISIBLE)) {
                            continue;
                        }
                        if (((PxyPart *)b)->flags & 0x10) {
                            continue;
                        }
                        if (translucentBatch->start + translucentBatch->count >= PXY_TRANSLUCENT_ENTRY_END) {
                            break;
                        }
                        translucentEntry->part = (PxyPart *)b;
                        (translucentEntry++)->packet = (PxyPacket *)i;
                        translucentBatch->count++;
                    }
                }
            }
        }
    }
    if (!(((PxyPacket *)i)->unkD4 & 1)) {
        if (!(nmlRenderStatus & 0x10) || (nmlRenderStatus & 2)) {
            for (d = header->layerCount - 1; d >= 0; d--) {
                b = (int)&((PxyPacket *)i)->parts[((PxyPartRange *)ptr)->first];
                a = ((PxyPartRange *)ptr)->count;
                for (c = 0; c < a; c++, b += sizeof(PxyPart)) {
                    if (((PxyPart *)b)->layer == d) {
                        if (!(((PxyPacket *)i)->partFlags[((PxyPart *)b)->index] & PXY_PART_VISIBLE)) {
                            continue;
                        }
                        if (translucentBatch->start + translucentBatch->count >= PXY_TRANSLUCENT_ENTRY_END) {
                            break;
                        }
                        translucentEntry->part = (PxyPart *)b;
                        (translucentEntry++)->packet = (PxyPacket *)i;
                        translucentBatch->count++;
                    }
                }
            }
        }
    }
    func_001BEA8C(newSlot, (PxyPacket *)i);
    D_0096A308 += batch->count;
    D_0096A30C += translucentBatch->count;
    pxyResetGlobals();
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C3554);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C3628);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C374C);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C379C);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C37F0);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C3844);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C39A8);

typedef struct XglPacket XglPacket;

extern int D_0096A2A0;
extern int D_0096A2E0;
extern int D_0096A2E4;
extern u16 D_00645828;
extern s16 sRenderPsm;
extern XglQuadword *xglDirectQueueBuffer;
extern int xglDirectQueueCount;
void func_001CD108(int);
void xglDirectQueueBegin(void);
void xglDirectQueueFlush(void);
void func_001D00D0(void);
int func_001C39A8(PxyPacket *, PxyDrawWork *);
void pxyUploadPacketState(int, PxyPacket *);
void func_001CF520(PxyPacket *);
void func_001CF644(void);
void func_001CDC40(void);
void func_001D0798(void);
XglPacket *xglPacketGetCurrent(void);
void sceVif1PkCnt(XglPacket *packet, int count);
void func_0012B6E0(XglPacket *packet, int, int, int, int);
void func_0012B770(XglPacket *packet);
void func_0012BCE8(XglPacket *packet, void *, int);
void func_0012B9F8(XglPacket *packet, XglQuadword);
void xglMatrixMul(void *destination, void *left, void *right);

/* Draws the batches of shadow caster `caster` into its shadow texture at frameBase. */
void pxyDrawShadowCaster(int caster, u32 frameBase)
{
    int i;
    PxyPacket *packet;
    PxyShadow *shadow;
    int savedPass;
    u64 *p;

    packet = pxyState.shadowBatches[caster]->packet;
    shadow = packet->shadow;
    savedPass = D_0096A2E4;
    pxyDrawWork->frameBase = frameBase;
    pxyDrawWork->unk28 = 0;
    func_001CD1AC();
    func_001CD160();
    {
        u64 *start;

        xglDirectQueueBegin();
        p = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
        start = p;
        GS_PUT(p, GS_ZBUF_1, GS_SET_ZBUF(D_00645828, 1, 1));
        GS_PUT(p, GS_FBA_1, 0);
        GS_PUT(p, GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2));
        GS_PUT(p, GS_CLAMP_1, shadow->clamp);
        GS_PUT(p, GS_TEX0_1, shadow->tex0);
        GS_PUT(p, GS_ALPHA_1, shadow->unk338);
        xglDirectQueueCount += ((u32)p - (u32)start) >> 4;
    }
    func_001D00D0();
    for (i = 0; i < D_0096A2A0; i++) {
        int pass;
        AlignedVector4 v;
        void *q;
        Matrix m;
        Vector4 box[2];
        Vector4 position;
        AlignedVector4 sphere;
        Matrix local;

        pxyDrawWork->batch = pxyState.overlayBatches[i];
        if ((pxyFlags & 0x10) && !(pxyFlags & 0x40000) && (packet->unk08 == 1 || packet->unk08 == 4)
            && packet->unkC0 < packet->unkC8) {
            v = (AlignedVector4){ 0.0f, -1.0f, 0.0f, 0.0f };
            v.w = -(v.x * packet->center.x + v.y * packet->unkC0 + v.z * packet->center.z);
            if (pxyDrawWork->batch->packet->planes == 0) {
                pxyDrawWork->batch->packet->planes = func_001D2B20(5);
            }
            pxyDrawWork->batch->packet->planes->plane[0] = v;
            pxyDrawWork->batch->packet->planes->plane[1] = v;
            v = (AlignedVector4){ 0.0f, 1.0f, 0.0f, 0.0f };
            v.w = -(v.x * packet->center.x + v.y * packet->unkC8 + v.z * packet->center.z);
            if (pxyDrawWork->batch->packet->planes == 0) {
                pxyDrawWork->batch->packet->planes = func_001D2B20(5);
            }
            pxyDrawWork->batch->packet->planes->plane[1] = v;
            pxyDrawWork->batch->packet->planeCount = 2;
            pxyDrawWork->batch->packet->renderStatus |= 0x4000;
        }
        xglDirectQueueBegin();
        p = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
        q = p;
        GS_PUT(p, GS_FRAME_1, GS_SET_FRAME(frameBase, cameraScreenWidth / 64, sRenderPsm,
                                           pxyDrawWork->batch->packet->unk9C | 0xFF000000));
        GS_PUT(p, GS_FRAME_2, GS_SET_FRAME(frameBase, cameraScreenWidth / 64, sRenderPsm,
                                           pxyDrawWork->batch->packet->unk9C | 0xFF000000));
        xglDirectQueueCount += ((u32)p - (u32)q) >> 4;
        xglDirectQueueFlush();
        for (pass = 0; pass < 2; pass++) {
            pxyDrawWork->lastPacket = pxyDrawWork->batch->packet;
            D_0096A2E4 = pass;
            func_001CD108(D_0096A2E4);
            if (pass != 0) {
                pxyDrawWork->batch = pxyDrawWork->batch->translucent;
            }
            if (!((D_00645F90[D_0096A2E0] == 1 || D_0096A2E0 == pxyDrawWork->batch->packet->camera)
                  && pxyDrawWork->batch->packet->cameraMask[D_0096A2E0] != 0)) {
                continue;
            }
            pxyDrawWork->batch->packet->renderStatus |= 0x20000000;
            pxyUploadPacketState(pxyDrawWork->batch->packet->renderStatus & 4, pxyDrawWork->batch->packet);
            func_001CF520(pxyDrawWork->batch->packet);
            q = xglPacketGetCurrent();
            xglMatrixStackLoad(&shadow->unk040);
            xglMatrixStackMul(pxyDrawWork->batch->packet->world);
            xglMatrixStackSave(&m);
            sceVif1PkCnt(q, 0);
            func_0012B6E0(q, 0x39E, 0x6C, 1, 1);
            func_0012BCE8(q, &m, 4);
            func_0012B770(q);
            func_0012B6E0(q, 0x3C3, 0x6C, 1, 1);
            func_0012B9F8(q, *(XglQuadword *)&shadow->uv);
            func_0012B770(q);
            xglMatrixUnit(&m);
            shadow->center.w = 1.0f;
            shadow->lighting.direction.w = 0;
            xglVectorCopy_001CC998(m.rows[3], &shadow->lighting.direction);
            m.rows[0][3] = m.rows[3][0];
            m.rows[1][3] = m.rows[3][1];
            m.rows[2][3] = m.rows[3][2];
            m.rows[0][2] = shadow->center.x;
            m.rows[1][2] = shadow->center.y;
            m.rows[2][2] = shadow->center.z;
            m.rows[0][0] = 128.0f / (packet->center.w * 2.0f);
            if (packet->unk08 == 4) {
                m.words[0][1] = 1;
            }
            if (pxyFlags & 0x20000) {
                m.words[1][0] = 1;
            }
            func_0012B6E0(q, 0x392, 0x6C, 1, 1);
            func_0012BCE8(q, &m, 4);
            func_0012B770(q);
            pxyDrawWork->entry = &D_0097A980[pxyDrawWork->batch->start];
            for (pxyDrawWork->i = 0; pxyDrawWork->i < pxyDrawWork->batch->count;
                 pxyDrawWork->i++, pxyDrawWork->entry++) {
                pxyDrawWork->packet = pxyDrawWork->entry->packet;
                pxyDrawWork->part = pxyDrawWork->entry->part;
                if (pxyDrawWork->packet->header->flags & 0x80) {
                    break;
                }
                if (func_001C39A8(packet, pxyDrawWork)) {
                    continue;
                }
                if (pxyDrawWork->part->flags & 0x1000) {
                    continue;
                }
                if (pxyFlags & 0x10) {
                    if (D_0096A2DC == 2) {
                        if (pass != 0 && !(pxyDrawWork->part->flags & 0x20000)) {
                            continue;
                        }
                        if (pass == 0 && !(pxyDrawWork->part->flags & 0x20000)
                            && (pxyDrawWork->part->flags & 0x10)) {
                            continue;
                        }
                    } else if (D_0096A2DC == 1 && (pxyDrawWork->part->flags & 0x10)) {
                        continue;
                    }
                } else if (D_0096A2DC == 1 && (pxyDrawWork->part->flags & 0x10)) {
                    continue;
                }
                if (pxyFlags & 0x10) {
                    xglVectorAddXYZ_001CCA30(&box[0], &pxyDrawWork->part->boundsMax, pxyDrawWork->batch->packet->world[3]);
                    xglVectorAddXYZ_001CCA30(&box[1], &pxyDrawWork->part->boundsMin, pxyDrawWork->batch->packet->world[3]);
                    if (shadow->min.x > box[0].x || shadow->min.y > box[0].y || shadow->min.z > box[0].z
                        || shadow->max.x < box[1].x || shadow->max.y < box[1].y || shadow->max.z < box[1].z) {
                        continue;
                    }
                } else {
                    v = pxyDrawWork->part->sphere;
                    v.w = 1.0f;
                    xglVectorMulMat(&box[0], pxyDrawWork->batch->packet->world, &v);
                    if (shadow->min.x > box[0].x + pxyDrawWork->part->sphere.w
                        || shadow->min.y > box[0].y + pxyDrawWork->part->sphere.w
                        || shadow->min.z > box[0].z + pxyDrawWork->part->sphere.w
                        || shadow->max.x < box[0].x - pxyDrawWork->part->sphere.w
                        || shadow->max.y < box[0].y - pxyDrawWork->part->sphere.w
                        || shadow->max.z < box[0].z - pxyDrawWork->part->sphere.w) {
                        continue;
                    }
                }
                pxyDrawWork->packet->renderStatus |= 0x20000000;
                func_001CF644();
                if ((pxyDrawWork->batch->flags & 0x40000000) || pxyDrawWork->lastPacket != pxyDrawWork->packet) {
                    func_001CDC40();
                }
                func_001D0798();
                pxyDrawWork->packet->renderStatus &= ~0x20000000;
                pxyDrawWork->lastPacket = pxyDrawWork->packet;
            }
            if (pxyDrawWork->i < pxyDrawWork->batch->count) {
                for (; pxyDrawWork->i < pxyDrawWork->batch->count; pxyDrawWork->i++, pxyDrawWork->entry++) {
                    q = &pxyDrawWork->entry->packet->matrices[pxyDrawWork->entry->part->maskBit];
                    pxyDrawWork->packet = pxyDrawWork->entry->packet;
                    pxyDrawWork->part = pxyDrawWork->entry->part;
                    if (pxyDrawWork->part->flags & 0x1000) {
                        continue;
                    }
                    if (pxyFlags & 0x10) {
                        if (D_0096A2DC == 2) {
                            if (pass != 0 && !(pxyDrawWork->part->flags & 0x20000)) {
                                continue;
                            }
                            if (pass == 0 && !(pxyDrawWork->part->flags & 0x20000)
                                && (pxyDrawWork->part->flags & 0x10)) {
                                continue;
                            }
                        } else if (D_0096A2DC == 1 && (pxyDrawWork->part->flags & 0x10)) {
                            continue;
                        }
                    } else if (D_0096A2DC == 1 && (pxyDrawWork->part->flags & 0x10)) {
                        continue;
                    }
                    if (pxyFlags & 0x10) {
                        v = pxyDrawWork->part->sphere;
                        v.w = 1.0f;
                        xglMatrixStackLoad(pxyDrawWork->entry->packet->world);
                        xglMatrixStackMul(q);
                        xglMatrixStackMulVector(&v, &v);
                        v.w = pxyDrawWork->part->sphere.w;
                        if (shadow->min.x > v.x + pxyDrawWork->part->sphere.w
                            || shadow->min.y > v.y + pxyDrawWork->part->sphere.w
                            || shadow->min.z > v.z + pxyDrawWork->part->sphere.w
                            || shadow->max.x < v.x - pxyDrawWork->part->sphere.w
                            || shadow->max.y < v.y - pxyDrawWork->part->sphere.w
                            || shadow->max.z < v.z - pxyDrawWork->part->sphere.w) {
                            continue;
                        }
                    } else {
                        sphere = pxyDrawWork->part->sphere;
                        sphere.w = 1.0f;
                        xglVectorMulMat(&position, q, &sphere);
                        if (shadow->min.x > position.x + pxyDrawWork->part->sphere.w
                            || shadow->min.y > position.y + pxyDrawWork->part->sphere.w
                            || shadow->min.z > position.z + pxyDrawWork->part->sphere.w
                            || shadow->max.x < position.x - pxyDrawWork->part->sphere.w
                            || shadow->max.y < position.y - pxyDrawWork->part->sphere.w
                            || shadow->max.z < position.z - pxyDrawWork->part->sphere.w) {
                            continue;
                        }
                    }
                    p = (u64 *)xglPacketGetCurrent();
                    xglMatrixMul(&local, &shadow->unk040, pxyDrawWork->packet->world);
                    xglMatrixMul(&local, &local, &pxyDrawWork->packet->matrices[pxyDrawWork->part->maskBit]);
                    sceVif1PkCnt((XglPacket *)p, 0);
                    func_0012B6E0((XglPacket *)p, 0x39E, 0x6C, 1, 1);
                    func_0012BCE8((XglPacket *)p, &local, 4);
                    func_0012B770((XglPacket *)p);
                    func_0012B6E0((XglPacket *)p, 0x3C3, 0x6C, 1, 1);
                    func_0012B9F8((XglPacket *)p, *(XglQuadword *)&shadow->uv);
                    func_0012B770((XglPacket *)p);
                    xglMatrixMul(&local, pxyDrawWork->packet->world, &pxyDrawWork->packet->matrices[pxyDrawWork->part->maskBit]);
                    func_001A9B50(&local, &local);
                    shadow->center.w = 1.0f;
                    xglVectorMulMat(&sphere, local.rows, &shadow->center);
                    shadow->lighting.direction.w = 0;
                    xglVectorMulMat((Vector4 *)local.rows[3], local.rows, &shadow->lighting.direction);
                    local.rows[0][3] = local.rows[3][0];
                    local.rows[1][3] = local.rows[3][1];
                    local.rows[2][3] = local.rows[3][2];
                    local.rows[0][2] = sphere.x;
                    local.rows[1][2] = sphere.y;
                    local.rows[2][2] = sphere.z;
                    local.rows[0][0] = 128.0f / (packet->center.w * 2.0f);
                    if (packet->unk08 == 4) {
                        local.words[0][1] = 1;
                    }
                    if (pxyFlags & 0x20000) {
                        local.words[1][0] = 1;
                    }
                    func_0012B6E0((XglPacket *)p, 0x392, 0x6C, 1, 1);
                    func_0012BCE8((XglPacket *)p, &local, 4);
                    func_0012B770((XglPacket *)p);
                    pxyDrawWork->packet->renderStatus |= 0x20000000;
                    func_001CF644();
                    if ((pxyDrawWork->batch->flags & 0x40000000) || pxyDrawWork->lastPacket != pxyDrawWork->packet) {
                        func_001CDC40();
                    }
                    func_001D0798();
                    pxyDrawWork->packet->renderStatus &= ~0x20000000;
                    pxyDrawWork->lastPacket = pxyDrawWork->packet;
                }
            }
            pxyDrawWork->batch->packet->renderStatus &= ~0x20000000;
            if (D_0096A2DC == 1) {
                break;
            }
        }
        if ((pxyFlags & 0x10) && (packet->unk08 == 1 || packet->unk08 == 4)) {
            pxyDrawWork->batch->packet->renderStatus &= ~0x4000;
        }
    }
    func_001CD134();
    func_001CD1AC();
    func_001CD160();
    D_0096A2E4 = savedPass;
    func_001CD108(D_0096A2E4);
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C5274);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C555C);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C5C64);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C65F8);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C6F48);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C7500);

extern int D_0096A2D0;
extern int pxyFlags;
void func_001CFC74(void);
void func_001CE7C4(int);
void func_001CE4C4(void);
void func_001D5004(int, u32);

/* Draws the entries of `batch` in draw mode `mode` into the frame at frameBase; `frameMask`
 * is or-ed into the FRAME write mask of the batch. A part is drawn only in the modes its flags
 * select: flag 4 parts in mode 1 and only there, flag 0x2000 parts in mode 2, and parts that
 * are hidden-capable (packet 0x20000 or part 0x10000) in mode 4; mode 3 stops at the first
 * entry unless the batch has flag 0x100000. Parts with flag 0x4000 write their own FRAME masks. */
void pxyDrawBatch(int mode, PxyBatch *batch, u32 frameBase, int frameMask)
{
    u64 *p;
    u64 *q;

    pxyDrawWork->cameraBit = 1 << D_0096A2E0;
    pxyDrawWork->lastPacket = 0;
    pxyDrawWork->batch = batch;
    pxyDrawWork->mode = mode;
    pxyDrawWork->frameBase = frameBase;
    pxyDrawWork->frameMask = frameMask;
    pxyDrawWork->unk28 = 0;
    func_001CD1AC();
    func_001CD160();
    xglDirectQueueBegin();
    p = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
    q = p;
    GS_PUT(p, GS_FRAME_1, GS_SET_FRAME(pxyDrawWork->frameBase, cameraScreenWidth / 64, sRenderPsm,
                                       pxyDrawWork->frameMask | pxyDrawWork->batch->packet->unk9C));
    GS_PUT(p, GS_FRAME_2, GS_SET_FRAME(pxyDrawWork->frameBase, cameraScreenWidth / 64, sRenderPsm,
                                       pxyDrawWork->frameMask | pxyDrawWork->batch->packet->unk9C | 0xFF000000));
    xglDirectQueueCount += ((u32)p - (u32)q) >> 4;
    xglDirectQueueFlush();
    pxyUploadPacketState(pxyDrawWork->batch->packet->renderStatus & 4, pxyDrawWork->batch->packet);
    func_001CF520(pxyDrawWork->batch->packet);
    if (pxyDrawWork->batch->flags & 0x6920000) {
        pxyDrawWork->entry = &D_0097A980[pxyDrawWork->batch->start];
        pxyDrawWork->count = pxyDrawWork->batch->count;
        pxyDrawWork->batchFlags = pxyDrawWork->batch->flags;
        pxyDrawWork->mode = mode;
        for (pxyDrawWork->i = 0; pxyDrawWork->i < pxyDrawWork->count; pxyDrawWork->i++, pxyDrawWork->entry++) {
            pxyDrawWork->unk28 = 0;
            pxyDrawWork->packet = pxyDrawWork->entry->packet;
            pxyDrawWork->part = pxyDrawWork->entry->part;
            pxyDrawWork->partFlags = pxyDrawWork->part->flags;
            pxyDrawWork->packetFlags = pxyDrawWork->packet->renderStatus;
            if (!(pxyDrawWork->packet->partFlags[pxyDrawWork->part->index] & pxyDrawWork->cameraBit)) {
                continue;
            }
            if ((pxyDrawWork->mode == 1) ^ ((pxyDrawWork->partFlags & 4) != 0)) {
                continue;
            }
            if ((pxyDrawWork->mode == 2) ^ ((pxyDrawWork->partFlags & 0x2000) != 0)) {
                continue;
            }
            if (pxyDrawWork->mode == 3 && !(pxyDrawWork->batchFlags & 0x100000)) {
                break;
            }
            if (pxyDrawWork->packet->hideMask != 0) {
                if (pxyDrawWork->mode != 4
                    && !(pxyDrawWork->packet->hideMask->bits[(s16)(pxyDrawWork->part->maskBit >> 5)]
                         & (1 << (s16)(pxyDrawWork->part->maskBit & 0x1F)))
                    && ((pxyDrawWork->packetFlags & 0x20000) || (pxyDrawWork->partFlags & 0x10000))) {
                    continue;
                }
                if (pxyDrawWork->mode == 4
                    && ((pxyDrawWork->packet->hideMask->bits[(s16)(pxyDrawWork->part->maskBit >> 5)]
                         & (1 << (s16)(pxyDrawWork->part->maskBit & 0x1F)))
                        || !((pxyDrawWork->packetFlags & 0x20000) || (pxyDrawWork->partFlags & 0x10000)))) {
                    continue;
                }
            } else if ((pxyDrawWork->mode == 4)
                       ^ (((pxyDrawWork->packetFlags & 0x20000) || (pxyDrawWork->partFlags & 0x10000)) != 0)) {
                continue;
            }
            if (D_0096A2E4 != 0 && (pxyDrawWork->partFlags & 0x80)) {
                func_001D4568(pxyDrawWork->batch->packet->unk9C, pxyDrawWork->frameBase);
            }
            if (pxyDrawWork->partFlags & 0x4000) {
                if (!(pxyDrawWork->partFlags & 0x1000)) {
                    xglDirectQueueBegin();
                    q = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
                    p = q;
                    GS_PUT(q, GS_FRAME_1, GS_SET_FRAME(pxyDrawWork->frameBase, cameraScreenWidth / 64, sRenderPsm, 0xFFFFFFFF));
                    GS_PUT(q, GS_FRAME_2, GS_SET_FRAME(pxyDrawWork->frameBase, cameraScreenWidth / 64, sRenderPsm, 0xFFFFFFFF));
                    xglDirectQueueCount += ((u32)q - (u32)p) >> 4;
                    xglDirectQueueFlush();
                    pxyDrawWork->unk28 = 1;
                } else {
                    xglDirectQueueBegin();
                    q = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
                    p = q;
                    GS_PUT(q, GS_FRAME_1, GS_SET_FRAME(pxyDrawWork->frameBase, cameraScreenWidth / 64, sRenderPsm, 0xFF000000));
                    GS_PUT(q, GS_FRAME_2, GS_SET_FRAME(pxyDrawWork->frameBase, cameraScreenWidth / 64, sRenderPsm, 0xFFFFFFFF));
                    xglDirectQueueCount += ((u32)q - (u32)p) >> 4;
                    xglDirectQueueFlush();
                }
            } else if ((pxyDrawWork->partFlags & 0x200) && (pxyDrawWork->partFlags & 0x1000)) {
                continue;
            }
            if (pxyDrawWork->partFlags & 0x40) {
                xglDirectQueueBegin();
                q = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
                p = q;
                GS_PUT(q, GS_FBA_1, 1);
                xglDirectQueueCount += ((u32)q - (u32)p) >> 4;
                xglDirectQueueFlush();
            } else {
                xglDirectQueueBegin();
                q = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
                p = q;
                GS_PUT(q, GS_FBA_1, D_0096A2D0);
                xglDirectQueueCount += ((u32)q - (u32)p) >> 4;
                xglDirectQueueFlush();
            }
            if (!(pxyFlags & 0x40)) {
                func_001CFC74();
            }
            func_001CF644();
            if ((pxyDrawWork->batchFlags & 0x40000000) || pxyDrawWork->lastPacket != pxyDrawWork->packet) {
                func_001CDC40();
            }
            if (pxyDrawWork->batchFlags & 0x80000000) {
                func_001CE7C4(pxyDrawWork->packetFlags & 4);
            }
            func_001CE4C4();
            func_001D0798();
            if (pxyDrawWork->partFlags & 0x4000) {
                xglDirectQueueBegin();
                q = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
                p = q;
                GS_PUT(q, GS_FRAME_1, GS_SET_FRAME(pxyDrawWork->frameBase, cameraScreenWidth / 64, sRenderPsm,
                                                   (u32)pxyDrawWork->batch->packet->unk9C));
                GS_PUT(q, GS_FRAME_2, GS_SET_FRAME(pxyDrawWork->frameBase, cameraScreenWidth / 64, sRenderPsm,
                                                   pxyDrawWork->batch->packet->unk9C | 0xFF000000));
                xglDirectQueueCount += ((u32)q - (u32)p) >> 4;
                xglDirectQueueFlush();
            }
            if (D_0096A2E4 != 0 && (pxyDrawWork->partFlags & 0x100)) {
                func_001D5004(pxyDrawWork->batch->packet->unk9C, pxyDrawWork->frameBase);
            }
            pxyDrawWork->lastPacket = pxyDrawWork->packet;
        }
    } else {
        pxyDrawWork->entry = &D_0097A980[pxyDrawWork->batch->start];
        pxyDrawWork->count = pxyDrawWork->batch->count;
        pxyDrawWork->batchFlags = pxyDrawWork->batch->flags;
        if (pxyDrawWork->batchFlags & 0xC0400000) {
            for (pxyDrawWork->i = 0; pxyDrawWork->i < pxyDrawWork->count; pxyDrawWork->i++, pxyDrawWork->entry++) {
                pxyDrawWork->packet = pxyDrawWork->entry->packet;
                pxyDrawWork->part = pxyDrawWork->entry->part;
                if (!(pxyDrawWork->packet->partFlags[pxyDrawWork->part->index] & pxyDrawWork->cameraBit)) {
                    continue;
                }
                if (!(pxyFlags & 0x40)) {
                    func_001CFC74();
                }
                func_001CF644();
                if ((pxyDrawWork->batchFlags & 0x40000000) || pxyDrawWork->lastPacket != pxyDrawWork->packet) {
                    func_001CDC40();
                }
                if (pxyDrawWork->batchFlags & 0x80000000) {
                    func_001CE7C4(pxyDrawWork->packet->renderStatus & 4);
                }
                func_001CE4C4();
                func_001D0798();
                pxyDrawWork->lastPacket = pxyDrawWork->packet;
            }
        } else {
            for (pxyDrawWork->i = 0; pxyDrawWork->i < pxyDrawWork->count; pxyDrawWork->i++, pxyDrawWork->entry++) {
                pxyDrawWork->packet = pxyDrawWork->entry->packet;
                pxyDrawWork->part = pxyDrawWork->entry->part;
                if (!(pxyDrawWork->packet->partFlags[pxyDrawWork->part->index] & pxyDrawWork->cameraBit)) {
                    continue;
                }
                if (!(pxyFlags & 0x40)) {
                    func_001CFC74();
                }
                func_001CF644();
                func_001CE4C4();
                func_001D0798();
            }
        }
    }
    xglDirectQueueBegin();
    q = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
    p = q;
    GS_PUT(q, GS_FRAME_1, GS_SET_FRAME(pxyDrawWork->frameBase, cameraScreenWidth / 64, sRenderPsm, 0));
    GS_PUT(q, GS_FRAME_2, GS_SET_FRAME(pxyDrawWork->frameBase, cameraScreenWidth / 64, sRenderPsm, 0xFF000000));
    xglDirectQueueCount += ((u32)q - (u32)p) >> 4;
    xglDirectQueueFlush();
    func_001CD160();
}

s32 D_0069E0C0 = 0;

/* Half the screen, in pixels, as the GS offsets use it. */
#define SCREEN_HALF_W ((s16)(cameraScreenWidth / 2))
#define SCREEN_HALF_H ((s16)(cameraScreenHeight / 2))

extern PxyBatch *D_0096A2B4;

void func_001C3628(int, int);
f32 func_001D34C4(void);
f32 func_001D35FC(void);
void pxyDrawBatch(int, PxyBatch *, u32, int);
void func_001D4568(int, u32);
void func_001C6F48(PxyBatch *, u32);
void func_001C65F8(PxyBatch *, u32);
void func_001C7500(PxyBatch *, u32);
void func_001C555C(u32);
void func_001C5274(u32);
void func_001C5C64(u32);

/* Draws the overlay and then the sorted batches of the current camera in draw mode `mode`, then shadow casters and the translucent pass. */
void pxyDrawBatches(int mode, u32 frameBase)
{
    PxyBatchTable *table;
    int i;
    PxyBatch *batch;

    table = 0;
    batch = 0;
    func_001CD1AC();
    func_001CD160();
    D_0096A2E4 = 0;
    func_001CD108(D_0096A2E4);
    for (i = 0; i < D_0096A2A0; i++) {
        batch = pxyState.overlayBatches[i];
        if (!((D_00645F90[D_0096A2E0] == 1 || D_0096A2E0 == batch->packet->camera)
              && batch->packet->cameraMask[D_0096A2E0] != 0)
            || (mode == 4 && !(batch->packet->renderStatus & 0x20000) && !(batch->flags & 0x20000))
            || (mode == 1 && !(batch->flags & 0x4000000)) || (mode == 2 && !(batch->flags & 0x2000000))) {
            continue;
        }
        if (func_001D34C4() > 85.0f || func_001D35FC() > 85.0f) {
            return;
        }
        if (mode == 0 && batch->packet->preDraw != 0) {
            batch->packet->preDraw(D_0096A2E0, D_0096A2E4);
        }
        pxyDrawBatch(mode, batch, frameBase, 0);
        if (mode == 0 && batch->packet->postDraw != 0) {
            batch->packet->postDraw(D_0096A2E0, D_0096A2E4);
        }
    }
    if (mode == 0) {
        func_001C3628(4, D_0096A2E0);
    }
    table = &D_0096A6A0[D_0096A2E4];
    batch = D_0096A2B4;
    while (D_0096A2B4 != 0) {
        if (!((D_00645F90[D_0096A2E0] == 1 || D_0096A2E0 == batch->packet->camera)
              && batch->packet->cameraMask[D_0096A2E0] != 0 && !(batch->packet->renderStatus & 0x10))
            || (mode == 4 && !(batch->packet->renderStatus & 0x20000) && !(batch->flags & 0x20000))
            || (mode == 1 && !(batch->flags & 0x4000000)) || (mode == 2 && !(batch->flags & 0x2000000))) {
            if (batch->next < 0) {
                break;
            }
            batch = &table->batches[batch->next];
            continue;
        }
        if (func_001D34C4() > 85.0f || func_001D35FC() > 85.0f) {
            return;
        }
        if (mode == 0 && batch->packet->preDraw != 0) {
            batch->packet->preDraw(D_0096A2E0, D_0096A2E4);
        }
        if (batch->packet->renderStatus & 0x80000) {
            u64 *p;
            u64 *start;

            func_001D4568(batch->packet->unk9C, frameBase);
            xglDirectQueueBegin();
            p = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
            start = p;
            GS_PUT(p, GS_FBA_1, 1);
            xglDirectQueueCount += ((u32)p - (u32)start) >> 4;
            xglDirectQueueFlush();
        }
        pxyDrawBatch(mode, batch, frameBase, 0);
        if (batch->packet->renderStatus & 0x80000) {
            int y;
            int color[4];

            color[0] = 0x80;
            color[1] = 0x80;
            color[2] = 0x80;
            color[3] = 0x80;
            {
                u64 *p;
                u64 *start;

                xglDirectQueueBegin();
                p = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
                start = p;
                GS_PUT(p, GS_FBA_1, 0);
                xglDirectQueueCount += ((u32)p - (u32)start) >> 4;
                xglDirectQueueFlush();
            }
            for (y = 0; y < cameraScreenHeight; y += SCREEN_HALF_H) {
                int pass;

                {
                    u64 *p;
                    u64 *start;

                    xglDirectQueueBegin();
                    p = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
                    start = p;
                    GS_PUT(p, GS_FRAME_1, GS_SET_FRAME(D_0064583A, cameraScreenWidth / 64, sRenderPsm, 0));
                    GS_PUT(p, GS_TEXFLUSH, 0);
                    GS_PUT(p, GS_ZBUF_1, GS_SET_ZBUF(D_00645828, 1, 1));
                    GS_PUT(p, GS_CLAMP_1, 0);
                    GS_PUT(p, GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0, 0x3F800000));
                    GS_PUT(p, GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1));
                    GS_PUT(p, GS_TEX0_1, GS_SET_TEX0(frameBase << 5, 8, 0, 9, 9, 1, 0, 0, 0, 0, 0, 1));
                    GS_PUT(p, GS_TEX1_1, 0);
                    GS_PUT(p, GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 0, 0, 1, 0, 0));
                    GS_PUT(p, GS_UV, GS_SET_UV(0, (y << 4)));
                    GS_PUT(p, GS_XYZF2, GS_SET_XYZ(((0x800 - SCREEN_HALF_W) << 4), ((0x800 - SCREEN_HALF_H) << 4), 0));
                    GS_PUT(p, GS_UV, GS_SET_UV((cameraScreenWidth << 4), ((SCREEN_HALF_H + y) << 4)));
                    GS_PUT(p, GS_XYZF2, GS_SET_XYZ(((cameraScreenWidth - SCREEN_HALF_W + 0x7FF) << 4), (0x7FF << 4), 0));
                    GS_PUT(p, GS_RGBAQ, GS_SET_RGBAQ(color[0], color[1], color[2], color[3], 0x3F800000));
                    GS_PUT(p, GS_TEST_1, GS_SET_TEST(1, 5, 0x80, 0, 0, 0, 1, 1));
                    GS_PUT(p, GS_TEX1_1, GS_SET_TEX1(0, 0, 1, 1, 0, 0, 0));
                    GS_PUT(p, GS_FBA_1, 1);
                    xglDirectQueueCount += ((u32)p - (u32)start) >> 4;
                    func_001D00D0();
                }
                for (pass = 0; pass < 1; pass++) {
                    int alphas[2] = { 40, 48 };
                    int shifts[2] = { 2, 1 };
                    int alpha;
                    int shift;

                    alpha = alphas[pass];
                    shift = shifts[pass];
                    {
                        u64 *p;
                        u64 *start;

                        xglDirectQueueBegin();
                        p = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
                        start = p;
                        GS_PUT(p, GS_ALPHA_1, GS_SET_ALPHA(0, 1, 2, 1, alpha));
                        GS_PUT(p, GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 1, 0, 1, 0, 0));
                        GS_PUT(p, GS_UV, GS_SET_UV(9, (y << 4) + 9));
                        GS_PUT(p, GS_XYZF2, GS_SET_XYZ(((shift - SCREEN_HALF_W + 0x800) << 4), ((0x800 - SCREEN_HALF_H) << 4), 0));
                        GS_PUT(p, GS_UV, GS_SET_UV(((cameraScreenWidth - shift) << 4) + 9, ((SCREEN_HALF_H + y) << 4) + 9));
                        GS_PUT(p, GS_XYZF2, GS_SET_XYZ(((cameraScreenWidth - SCREEN_HALF_W + 0x800) << 4), 0x8000, 0));
                        xglDirectQueueCount += ((u32)p - (u32)start) >> 4;
                        func_001D00D0();
                    }
                    {
                        u64 *p;
                        u64 *start;

                        xglDirectQueueBegin();
                        p = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
                        start = p;
                        GS_PUT(p, GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 1, 0, 1, 0, 0));
                        GS_PUT(p, GS_UV, GS_SET_UV((shift << 4) + 9, (y << 4) + 9));
                        GS_PUT(p, GS_XYZF2, GS_SET_XYZ(((0x800 - SCREEN_HALF_W) << 4), ((0x800 - SCREEN_HALF_H) << 4), 0));
                        GS_PUT(p, GS_UV, GS_SET_UV((cameraScreenWidth << 4) + 9, ((SCREEN_HALF_H + y) << 4) + 9));
                        GS_PUT(p, GS_XYZF2, GS_SET_XYZ(((cameraScreenWidth - SCREEN_HALF_W - shift + 0x800) << 4), 0x8000, 0));
                        xglDirectQueueCount += ((u32)p - (u32)start) >> 4;
                        func_001D00D0();
                    }
                    {
                        u64 *p;
                        u64 *start;

                        xglDirectQueueBegin();
                        p = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
                        start = p;
                        GS_PUT(p, GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 1, 0, 1, 0, 0));
                        GS_PUT(p, GS_UV, GS_SET_UV(8, ((shift + y) << 4) + 9));
                        GS_PUT(p, GS_XYZF2, GS_SET_XYZ(((0x800 - SCREEN_HALF_W) << 4), ((0x800 - SCREEN_HALF_H) << 4), 0));
                        GS_PUT(p, GS_UV, GS_SET_UV((cameraScreenWidth << 4) + 8, ((SCREEN_HALF_H + y) << 4) + 9));
                        GS_PUT(p, GS_XYZF2, GS_SET_XYZ(((cameraScreenWidth - SCREEN_HALF_W + 0x800) << 4), ((0x800 - shift) << 4), 0));
                        xglDirectQueueCount += ((u32)p - (u32)start) >> 4;
                        func_001D00D0();
                    }
                    {
                        u64 *p;
                        u64 *start;

                        xglDirectQueueBegin();
                        p = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
                        start = p;
                        GS_PUT(p, GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 1, 0, 1, 0, 0));
                        GS_PUT(p, GS_UV, GS_SET_UV(8, (y << 4) + 9));
                        GS_PUT(p, GS_XYZF2, GS_SET_XYZ(((0x800 - SCREEN_HALF_W) << 4), ((shift - SCREEN_HALF_H + 0x800) << 4), 0));
                        GS_PUT(p, GS_UV, GS_SET_UV((cameraScreenWidth << 4) + 8, ((SCREEN_HALF_H + y - shift) << 4) + 9));
                        GS_PUT(p, GS_XYZF2, GS_SET_XYZ(((cameraScreenWidth - SCREEN_HALF_W + 0x800) << 4), 0x8000, 0));
                        xglDirectQueueCount += ((u32)p - (u32)start) >> 4;
                        func_001D00D0();
                    }
                }
                {
                    u64 *p;
                    u64 *start;

                    xglDirectQueueBegin();
                    p = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
                    start = p;
                    GS_PUT(p, GS_FBA_1, 0);
                    GS_PUT(p, GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000));
                    GS_PUT(p, GS_TEST_1, GS_SET_TEST(1, 5, 0x80, 0, 0, 0, 1, 1));
                    GS_PUT(p, GS_TEX1_1, 0);
                    GS_PUT(p, GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 0, 0, 1, 0, 0));
                    GS_PUT(p, GS_UV, GS_SET_UV(0, (y << 4)));
                    GS_PUT(p, GS_XYZF2, GS_SET_XYZ(((0x800 - SCREEN_HALF_W) << 4), ((0x800 - SCREEN_HALF_H) << 4), 0));
                    GS_PUT(p, GS_UV, GS_SET_UV((cameraScreenWidth << 4), ((SCREEN_HALF_H + y) << 4)));
                    GS_PUT(p, GS_XYZF2, GS_SET_XYZ(((cameraScreenWidth - SCREEN_HALF_W + 0x7FF) << 4), (0x7FF << 4), 0));
                    xglDirectQueueCount += ((u32)p - (u32)start) >> 4;
                    func_001D00D0();
                }
                {
                    u64 *p;
                    u64 *start;

                    xglDirectQueueBegin();
                    p = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
                    start = p;
                    GS_PUT(p, GS_FRAME_1, GS_SET_FRAME(frameBase, cameraScreenWidth / 64, sRenderPsm, 0));
                    GS_PUT(p, GS_TEXFLUSH, 0);
                    GS_PUT(p, GS_RGBAQ, GS_SET_RGBAQ(0x80, 0x80, 0x80, 0x80, 0x3F800000));
                    GS_PUT(p, GS_TEX0_1, GS_SET_TEX0(D_0064583A << 5, 8, 0, 9, 9, 1, 0, 0, 0, 0, 0, 1));
                    GS_PUT(p, GS_TEST_1, GS_SET_TEST(1, 5, 0x80, 0, 1, 0, 1, 1));
                    GS_PUT(p, GS_TEX1_1, GS_SET_TEX1(0, 0, 1, 1, 0, 0, 0));
                    GS_PUT(p, GS_PRIM, GS_SET_PRIM(6, 0, 1, 0, 0, 0, 1, 0, 0));
                    GS_PUT(p, GS_UV, GS_SET_UV(0, 0));
                    GS_PUT(p, GS_XYZF2, GS_SET_XYZ(((0x800 - SCREEN_HALF_W) << 4), ((y - SCREEN_HALF_H + 0x800) << 4), 0));
                    GS_PUT(p, GS_UV, GS_SET_UV((cameraScreenWidth << 4), (SCREEN_HALF_H << 4)));
                    GS_PUT(p, GS_XYZF2, GS_SET_XYZ(((cameraScreenWidth - SCREEN_HALF_W + 0x7FF) << 4), ((y + 0x7FF) << 4), 0));
                    GS_PUT(p, GS_TEST_1, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 2));
                    xglDirectQueueCount += ((u32)p - (u32)start) >> 4;
                    func_001D00D0();
                }
            }
            func_001CD134();
        }
        if (mode == 0 && batch->packet->postDraw != 0) {
            batch->packet->postDraw(D_0096A2E0, D_0096A2E4);
        }
        if (batch->next < 0) {
            break;
        }
        batch = &table->batches[batch->next];
    }
    {
        u64 *p;
        u64 *start;

        xglDirectQueueBegin();
        p = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
        start = p;
        GS_PUT(p, GS_FRAME_1, GS_SET_FRAME(frameBase, cameraScreenWidth / 64, sRenderPsm, 0));
        GS_PUT(p, GS_FRAME_2, GS_SET_FRAME(frameBase, cameraScreenWidth / 64, sRenderPsm, 0xFF000000));
        xglDirectQueueCount += ((u32)p - (u32)start) >> 4;
        xglDirectQueueFlush();
    }
    if (mode == 0) {
        func_001C3628(0, D_0096A2E0);
    }
    D_0096A2E4 = 1;
    func_001CD108(D_0096A2E4);
    table = &D_0096A6A0[0];
    batch = D_0096A2B4;
    while (D_0096A2B4 != 0) {
        if (!((D_00645F90[D_0096A2E0] == 1 || D_0096A2E0 == batch->packet->camera)
              && batch->packet->cameraMask[D_0096A2E0] != 0 && (batch->packet->renderStatus & 0x40000)
              && !(batch->packet->renderStatus & 0x10) && !(batch->packet->unkD4 & 1))
            || (mode == 4 && !(batch->packet->renderStatus & 0x20000) && !(batch->flags & 0x20000))
            || (mode == 1 && !(batch->flags & 0x4000000)) || (mode == 2 && !(batch->flags & 0x2000000))) {
            if (batch->next < 0) {
                break;
            }
            batch = &table->batches[batch->next];
            continue;
        }
        if (func_001D34C4() > 85.0f || func_001D35FC() > 85.0f) {
            return;
        }
        if (mode == 0 || mode == 2 || mode == 4) {
            func_001C6F48(batch->translucent, frameBase);
        }
        if (mode == 0 || mode == 2 || mode == 4) {
            func_001C65F8(batch->translucent, frameBase);
        }
        pxyDrawBatch(mode, batch->translucent, frameBase, 0);
        if (mode == 0 || mode == 2 || mode == 4) {
            D_0096A2E4 = 0;
            func_001CD108(D_0096A2E4);
            func_001C7500(batch, frameBase);
            D_0096A2E4 = 1;
            func_001CD108(D_0096A2E4);
        }
        if (batch->next < 0) {
            break;
        }
        batch = &table->batches[batch->next];
    }
    if ((pxyFlags & 0x100000) && mode == 0) {
        if (func_001D34C4() > 85.0f || func_001D35FC() > 85.0f) {
            return;
        }
        func_001C555C(frameBase);
        func_001C5274(frameBase);
        func_001C5C64(frameBase);
    }
    {
        for (i = 0; i < D_0096A2A0; i++) {
            batch = pxyState.overlayBatches[i];
            if (!((D_00645F90[D_0096A2E0] == 1 || D_0096A2E0 == batch->packet->camera)
                  && batch->packet->cameraMask[D_0096A2E0] != 0)
                || (mode == 4 && !(batch->packet->renderStatus & 0x20000) && !(batch->flags & 0x20000))
                || (mode == 1 && !(batch->flags & 0x4000000)) || (mode == 2 && !(batch->flags & 0x2000000))) {
                continue;
            }
            if (func_001D34C4() > 85.0f || func_001D35FC() > 85.0f) {
                return;
            }
            if (mode == 0 || mode == 2 || mode == 4) {
                func_001C6F48(batch->translucent, frameBase);
            }
            pxyDrawBatch(mode, batch->translucent, frameBase, 0);
        }
    }
    if (!(pxyFlags & 0x100000) && mode == 0) {
        if (func_001D34C4() > 85.0f || func_001D35FC() > 85.0f) {
            return;
        }
        func_001C555C(frameBase);
        func_001C5274(frameBase);
        func_001C5C64(frameBase);
    }
    table = &D_0096A6A0[0];
    batch = D_0096A2B4;
    while (D_0096A2B4 != 0) {
        if (!((D_00645F90[D_0096A2E0] == 1 || D_0096A2E0 == batch->packet->camera)
              && batch->packet->cameraMask[D_0096A2E0] != 0 && !(batch->packet->renderStatus & 0x40000)
              && !(batch->packet->renderStatus & 0x10) && !(batch->packet->unkD4 & 1))
            || (mode == 4 && !(batch->packet->renderStatus & 0x20000) && !(batch->flags & 0x20000))
            || (mode == 1 && !(batch->flags & 0x4000000)) || (mode == 2 && !(batch->flags & 0x2000000))) {
            if (batch->next < 0) {
                break;
            }
            batch = &table->batches[batch->next];
            continue;
        }
        if (func_001D34C4() > 85.0f || func_001D35FC() > 85.0f) {
            return;
        }
        if (mode == 0 || mode == 2 || mode == 4) {
            func_001C6F48(batch->translucent, frameBase);
        }
        if (mode == 0 || mode == 2 || mode == 4) {
            func_001C65F8(batch->translucent, frameBase);
        }
        pxyDrawBatch(mode, batch->translucent, frameBase, 0);
        if (mode == 0 || mode == 2 || mode == 4) {
            D_0096A2E4 = 0;
            func_001CD108(D_0096A2E4);
            func_001C7500(batch, frameBase);
            D_0096A2E4 = 1;
            func_001CD108(D_0096A2E4);
        }
        if (batch->next < 0) {
            break;
        }
        batch = &table->batches[batch->next];
    }
    {
        u64 *p;
        u64 *start;

        xglDirectQueueBegin();
        p = (u64 *)&xglDirectQueueBuffer[xglDirectQueueCount];
        start = p;
        GS_PUT(p, GS_FRAME_1, GS_SET_FRAME(frameBase, cameraScreenWidth / 64, sRenderPsm, 0));
        GS_PUT(p, GS_FRAME_2, GS_SET_FRAME(frameBase, cameraScreenWidth / 64, sRenderPsm, 0xFF000000));
        xglDirectQueueCount += ((u32)p - (u32)start) >> 4;
        xglDirectQueueFlush();
    }
    if (mode == 0) {
        if (func_001D34C4() > 85.0f || func_001D35FC() > 85.0f) {
            return;
        }
        func_001C3628(1, D_0096A2E0);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001CB700);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001CBCE4);

void pxyClearEntries(void);
void pxyBegin(void *);
void pxyEnd(void *);
void pxyDrawEntries(void);
void func_001C3554(int);
void func_001CBCE4(void);
void func_001CCF84(void *);
void func_001CCFD4(int);
void func_001CF468(void);
void func_001D0234(void);
f32 func_001D34C4(void);
f32 func_001D35FC(void);
void func_001D4498(int);
extern unsigned char D_00645FA0[];
extern unsigned char D_0068F0B8[];
extern int D_0096A2CC;
extern int D_0096A2E0;
extern int D_0096A2F0;
extern int D_0096A2F8;
extern f32 D_0096A2FC;
extern f32 D_0096A300;

void pxyFlush(void) {
    pxyBegin(&D_0068F0B8);
    pxyResetSubmitCount();
    func_001CF468();
    func_001CCF84(&D_00645FA0);
    func_001D4498(pxyFlags & 0x80);
    func_001CCFD4(pxyFlags & 0x40);
    xglPacketSetMode3Flag(pxyFlags & 0x10);
    xglPacketSetMode4Flag(pxyFlags & 0x80000);
    pxySetRotateFreeze(D_0096A2C4);
    func_001D0234();
    func_001C3554(3);
    if ((D_0096A2BC != 0) || (D_0096A2F0 != 0) || (D_0096A2CC != 0)) {
        for (D_0096A2E0 = 0; D_0096A2E0 < 4; D_0096A2E0 += 1) {
            func_001CBCE4();
        }
    }
    func_001C3554(2);
    xglMotionCacheInvalidate();
    pxyEnd(&D_0068F0B8);
    if (D_0069E0C0 != 0) {
        pxyDrawEntries();
    }
    pxyClearEntries();
    D_0096A2F8 = pxyGetSubmitCount();
    D_0096A2FC = func_001D34C4();
    D_0096A300 = func_001D35FC();
    D_0096A304 = 1;
    D_0096A310 = ((f32) D_0096A308 / 4096.0f) * 100.0f;
    D_0096A314 = ((f32) (D_0096A30C - 0x1000) / 2560.0f) * 100.0f;
    pxySetFlushDone();
}

void func_001B9728(void);
void func_001B97CC(void);

/* Reset the pxy state: records, counters and globals, entry table and slots. */
void pxyResetState(void)
{
    func_001B97CC();
    pxyResetCounters();
    pxyClearEntryTable();
    func_001B9728();
}


void func_001CC4EC(void) {
    xglPacketAddFlushEIrq();
}

void func_001CC51C(void) {
    xglPacketAddFlushEIrq();
}

int func_001CC54C(void) {
    return D_0096A2F8;
}

f32 func_001CC574(void) {
    return D_0096A2FC;
}

f32 func_001CC59C(void) {
    return D_0096A300;
}

f32 func_001CC5C4(void) {
    return D_0096A310;
}

f32 func_001CC5EC(void) {
    return D_0096A314;
}

extern int D_0096A2F4;

void func_001CC614(void) {
    D_0096A2F4 = 1;
}

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001CC640);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001CC8E4);

void func_001B9B44(void);
void pxyInitTables(void);
void func_001B9EC4(void);
extern void (*D_00645874)(void);
extern void (*D_00645878)(void);
extern void func_001CC640(void);
extern void func_001CC8E4(void);

/* Init the pxy module: the reset helpers, then the two callbacks it installs in D_00645878 and D_00645874. */
void pxyInit(void)
{
    D_0069E0C0 = 0;
    func_001B9758();
    func_001B9B44();
    pxyInitTables();
    pxyResetSlots();
    func_001B9EC4();
    D_0096A2F4 = 0;
    D_00645878 = &func_001CC640;
    D_00645874 = &func_001CC8E4;
}
