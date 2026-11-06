#include "common.h"
#include "shared.h"
#include "main/nml_model_set.h"
#include "main/tu045.h"
#include "main/xgl_2_031.h"

typedef float Matrix4[4][4];

typedef unsigned int XglQuadword __attribute__((mode(TI)));

typedef union Matrix {
    float elements[16];
    Matrix4 rows;
    XglQuadword quadwords[4];
} Matrix;

typedef union GsTag {
    u64 value;
    u32 words[2];
} GsTag;

#define NML_MAP_SHADOW_PARTS_MAX 8

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
extern unsigned char pxyState[];
extern int nmlRenderStatus;
extern int D_0096A004;
extern int D_0096A008;
extern void *D_0096A00C;
extern int D_0096A010;
extern int D_0096A014;
extern int D_0096A018;
extern int D_0096A01C;
extern int D_0096A020;
extern int D_0096A024;
extern int D_0096A028;
extern int D_0096A02C;
extern int D_0096A030;
extern int D_0096A034;
extern int D_0096A038;
extern int D_0096A03C;
extern void *D_0096A040;
extern f32 D_0096A044;
extern f32 D_0096A048;
extern f32 D_0096A04C;
extern int D_0096A050;
extern int D_0096A054;
extern int D_0096A05C;
extern f32 D_0096A060;
extern f32 D_0096A064;
extern int D_0096A068;
extern void *D_0096A06C;
extern int D_0096A080;
extern int D_0096A084;
extern f32 D_0096A088;
extern int D_0096A090;
extern int D_0096A098;
extern int D_0096A09C;
extern int D_0096A0A0;
extern s16 D_0096A0A4;
extern int D_0096A0A8;
extern int D_0096A0AC;
extern int D_0096A0B4;
extern f32 D_0096A0C0;
extern void *D_0096A0C4;
extern f32 D_0096A0C8;
extern int D_0096A0D0;
extern int D_0096A0D4;
extern f32 D_0096A0D8;
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
extern unsigned char D_0096A1B0[];
extern int D_0096A1C0;
extern int D_0096A1D0;
extern f32 D_0096A2B8;
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
    D_0096A06C = &pxyState;
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
extern int D_00645F90;
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
    D_00645F90 = 1;
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

extern s32 pxyFlags;

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

void xglMatrixCopy_001CC9C4(void *, s32);

void nmlModelSetDrawMatrix(s32 value) {
    xglMatrixCopy_001CC9C4(&nmlModelDrawMatrix, value);
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

void func_001BB96C(int value)
{
  D_0096A06C = value;
}

int func_001D2B20(int);
void func_002A9E04(int, int, int);

void func_001BB99C(int value)
{
    D_0096A06C = func_001D2B20(0x19);
    func_002A9E04(D_0096A06C, value, 0x180);
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

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BC3E4);

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

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001BFD48);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C290C);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C3554);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C3628);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C374C);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C379C);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C37F0);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C3844);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C39A8);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C3A80);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C5274);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C555C);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C5C64);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C65F8);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C6F48);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C7500);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C7ADC);

INCLUDE_ASM("asm/main/nonmatchings/nml_model_set", func_001C8D64);

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
s32 D_0069E0C0 = 0;
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
