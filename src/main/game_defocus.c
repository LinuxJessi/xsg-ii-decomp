#include "common.h"
#include "shared.h"

typedef struct XglPacket XglPacket;

/* EE scratchpad RAM. */
#define SCRATCHPAD_BASE 0x70000000

typedef struct XglTaskPrefix XglTaskPrefix;
typedef struct XglTaskScheduler XglTaskScheduler;

/*
 * Only the link/callback prefix is evidenced by this allocation.  The
 * complete task records are allocated with a 0x80-byte stride, and callers
 * write task-specific payload beyond this 0x10-byte prefix.  This declaration
 * must not be read as the complete task layout or allocation size.
 */
struct XglTaskPrefix {
    XglTaskScheduler *scheduler;
    XglTaskPrefix *next;
    XglTaskPrefix *previous;
    int (*callback)(XglTaskPrefix *task);
};

struct XglTaskScheduler {
    XglTaskPrefix *free_tasks;
    XglTaskPrefix *active_head;
    XglTaskPrefix *active_tail;
    XglTaskPrefix *next_to_visit;
};

extern void sceVif1PkCnt(XglPacket *packet, int count);

unsigned short xglSRand(void);

float xglSin(float angle);
float xglCos(float angle);

extern XglTaskPrefix *xglTaskEntryNext(XglTaskScheduler *scheduler,
                                int (*callback)(XglTaskPrefix *task),
                                XglTaskPrefix *entry);

float I2F(int value);

int F2I(float value);

float xglFRand(void);

#define NULL ((void *)0)
#define DEFOCUS_SCRATCH ((DefocusSolidPacket *)SCRATCHPAD_BASE)
#define DEFOCUS_SOLID_GIF_TAG 0x51000004
#define DEFOCUS_PACKET_VIF_COMMAND 0x8001
#define DEFOCUS_SOLID_GS_MODE 0x30234000
#define DEFOCUS_SOLID_GS_REGISTERS 0x551

/* GS XY offsets (12.4 fixed point) of the 512x448 screen: the left / top edge
 * biased half a pixel, and the right / bottom edge one width / height later. */
#define DEFOCUS_SCREEN_X_BIAS 28664

#define DEFOCUS_SCREEN_Y_BIAS 29175
#define DEFOCUS_SCREEN_WIDTH_FIXED 8192
#define DEFOCUS_SCREEN_HEIGHT_FIXED 7168
#define DEFOCUS_SCREEN_X_END (DEFOCUS_SCREEN_X_BIAS + DEFOCUS_SCREEN_WIDTH_FIXED)
#define DEFOCUS_SCREEN_Y_END (DEFOCUS_SCREEN_Y_BIAS + DEFOCUS_SCREEN_HEIGHT_FIXED)
#define DEFOCUS_LAYER_COUNT 16
#define DEFOCUS_COLOR_NEUTRAL 0x80808080
#define DEFOCUS_LAYER_PARAMETERS 16
#define DEFOCUS_HEAD_WORDS 16
#define DEFOCUS_SOLID_WORDS 20
#define DEFOCUS_TYPE05_PACKET ((DefocusTexturedPacket *)SCRATCHPAD_BASE)
#define DEFOCUS_WAVE_HEADER ((DefocusWaveHeader *)SCRATCHPAD_BASE)
#define DEFOCUS_WAVE_TABLE ((DefocusWaveTable *)SCRATCHPAD_BASE)
#define DEFOCUS_WAVE_SPRITE ((DefocusWaveSprite *)(SCRATCHPAD_BASE + 0x80))
#define DEFOCUS_RADIAL_VERTICES ((DefocusRadialVertex *)SCRATCHPAD_BASE)
#define DEFOCUS_STAR_HEADER ((DefocusStarHeader *)(SCRATCHPAD_BASE + 0x318))
#define DEFOCUS_STAR_STRIP ((DefocusStarVertex *)(SCRATCHPAD_BASE + 0x338))
#define DEFOCUS_STAR_WORD(index) (((int *)(SCRATCHPAD_BASE + 0x318))[index])
#define DEFOCUS_RING_TABLE ((DefocusRingTable *)SCRATCHPAD_BASE)

typedef struct {
    u8 unmodeled_00[0x14];
    u16 tex0_state_14;
    u8 unmodeled_16[0x0a];
    u16 tex0_state_20;
    u16 tex0_state_22;
    u8 unmodeled_24[0x10];
    void (*callback)(XglPacket *packet);
} DefocusRenderState;

/* Partial view of one GameDefocusParam entry: 16 entries, stride 0x44.
 * The first three bytes are used by the recovered state handlers and the
 * following sixteen words hold the settings copied by GameDefocusSet. */
typedef struct {
    u8 type;
    u8 state;
    u8 index;
    u8 unmodeled_03;
    int parameters[DEFOCUS_LAYER_PARAMETERS];
} DefocusLayer;

/* GS registers inside the display-environment block at DispEnv. */
typedef struct {
    unsigned int en1 : 1;
    unsigned int en2 : 1;
    unsigned int crtmd : 3;
    unsigned int mmod : 1;
    unsigned int amod : 1;
    unsigned int slbg : 1;
    unsigned int alp : 8;
    unsigned int unmodeled_16 : 16;
    unsigned int unmodeled_32 : 32;
} DefocusPmode;

typedef struct {
    u64 fbp : 9;
    u64 unmodeled_09 : 55;
} DefocusDispfb;

typedef struct {
    unsigned int dx : 12;
    unsigned int dy : 11;
    unsigned int magh : 4;
    unsigned int magv : 2;
    unsigned int unmodeled_29 : 3;
    unsigned int dw : 12;
    unsigned int dh : 11;
    unsigned int unmodeled_55 : 9;
} DefocusDisplay;

typedef struct {
    unsigned int sx : 12;
    unsigned int sy : 11;
    unsigned int smph : 4;
    unsigned int smpv : 2;
    unsigned int unmodeled_29 : 3;
    unsigned int ww : 12;
    unsigned int wh : 11;
    unsigned int unmodeled_55 : 9;
} DefocusExtdata;

typedef struct {
    union {
        u64 flags;
        DefocusPmode pmode;
    };
    u8 unmodeled_08[8];
    u8 unmodeled_10[8];
    DefocusDisplay display1;
    u8 unmodeled_20[8];
    DefocusDispfb dispfb2;
    DefocusDisplay display2;
    u64 extbuf;
    DefocusExtdata extdata;
    u64 extwrite;
} DefocusDispEnv;

typedef struct {
    u32 header[3];
    u32 gif_tag;
    u32 vif_command;
    u32 gs_registers;
    u32 register_count;
    u32 header_padding;
    u32 red;
    u32 green;
    u32 blue;
    u32 alpha;
    u32 left;
    u32 top;
    u32 color;
    u32 flags;
    u32 right;
    u32 bottom;
    u32 second_color;
    u32 second_flags;
} DefocusSolidPacket;

/* Partial model of the fixed packet template; unknown words stay explicit. */
typedef struct DefocusSolidPacketHeader {
    u8 unmodeled_00[0x0c];
    u32 packet_tag_0c;
    u32 vu_memory_address_10;
    u32 packet_header_14;
    u32 packet_header_18;
    u32 unmodeled_1c;
    u32 packet_header_20;
    u32 unmodeled_24;
    u32 packet_header_28;
    u32 unmodeled_2c;
    u32 packet_header_30;
    u32 unmodeled_34;
    u32 packet_header_38;
    u32 unmodeled_3c;
} DefocusSolidPacketHeader;

typedef struct {
    u8 unmodeled_00[0x50];
    u64 texture;
    u8 unmodeled_58[8];
} DefocusTexturedHeader;

typedef struct {
    u32 header[3];
    u32 gif_tag;
    u32 vif_command;
    u32 gs_mode;
    u32 gs_registers;
    u32 header_padding;
    u32 red;
    u32 green;
    u32 blue;
    u32 alpha;
    u32 first_u;
    u32 first_v;
    u32 unmodeled_38[2];
    int left;
    int top;
    u32 color;
    u32 flags;
    u32 second_u;
    u32 second_v;
    u32 unmodeled_58[2];
    int right;
    int bottom;
    u32 second_color;
    u32 second_flags;
} DefocusTexturedPacket;

typedef struct {
    u64 unmodeled_00[12];
    u64 framebuffer;
    u64 frame_register;
    u64 texture;
    u64 unmodeled_78[15];
} DefocusCopyPacket;

typedef struct {
    u64 unmodeled_00[6];
    u64 source_frame;
    u64 frame_register;
    u64 color[2];
    u64 top_left;
    int first_color;
    u32 first_flags;
    u64 bottom_right;
    int second_color;
    u32 second_flags;
    u64 unmodeled_70[2];
    u64 destination_frame;
    u64 destination_register;
} DefocusTintPacket;

typedef struct {
    u32 unmodeled_00[20];
    u64 texture;
    u64 unmodeled_58;
    u64 texture_dimensions;
    u64 unmodeled_68;
} DefocusType05Header;

typedef struct {
    u8 unmodeled_00[0x20];
    u64 texture;
    u8 unmodeled_28[0x48];
} DefocusCopyTemplate;

typedef struct {
    u32 header[3];
    u32 gif_tag;
    u32 vif_command;
    u32 gs_mode;
    u32 gs_registers;
    u32 header_padding;
    u32 red;
    u32 green;
    u32 blue;
    u32 alpha;
    u32 sprite_tag_low;
    u32 sprite_tag_high;
    u32 sprite_registers;
    u32 sprite_padding;
} DefocusWaveHeader;

typedef struct {
    int x[17];
    int y[17];
} DefocusWaveTable;

typedef struct {
    u32 first_u;
    u32 first_v;
    u32 unmodeled_08[2];
    int left;
    int top;
    u32 color;
    u32 flags;
    u32 second_u;
    u32 second_v;
    u32 unmodeled_28[2];
    int right;
    int bottom;
    u32 second_color;
    u32 second_flags;
} DefocusWaveSprite;

typedef struct {
    int u;
    int v;
    u32 unmodeled_08[2];
    int x;
    int y;
    u32 color;
    u32 flags;
} DefocusRadialVertex;

typedef struct {
    u32 header[3];
    u32 gif_tag;
    u32 vif_command;
    u32 gs_mode;
    u32 gs_registers;
    u32 header_padding;
} DefocusStarHeader;

typedef struct {
    int x;
    int y;
} DefocusRingPoint;

typedef struct {
    DefocusRingPoint inner[33];
    DefocusRingPoint middle[33];
    DefocusRingPoint outer[33];
} DefocusRingTable;

typedef struct {
    int red;
    int green;
    int blue;
    int alpha;
    int x;
    int y;
    int z;
    int flags;
} DefocusStarVertex;

extern DefocusRenderState sRender;
extern DefocusLayer GameDefocusParam[DEFOCUS_LAYER_COUNT];
extern DefocusDispEnv DispEnv;
extern int ScanLineInterpolate;
extern void (*func_14[11])(DefocusLayer *layer, XglPacket *packet);

void DefocusMain(XglPacket *packet);

static void DefocusMainType09Final(DefocusLayer *layer);

extern void sceVif1PkAddDataN(XglPacket *packet, const void *data, int count);
extern int printf(const char *format, ...);

void DefocusMainType01(DefocusLayer *layer, XglPacket *packet);
void DefocusMainType06(DefocusLayer *layer, XglPacket *packet);
void DefocusMainType02(DefocusLayer *layer, XglPacket *packet);
void DefocusMainType07(DefocusLayer *layer, XglPacket *packet);
void DefocusMainType11(DefocusLayer *layer, XglPacket *packet);
void DefocusMainType09(DefocusLayer *layer, XglPacket *packet);
void DefocusMainType03(DefocusLayer *layer, XglPacket *packet);
void DefocusMainType05(DefocusLayer *layer, XglPacket *packet);
void DefocusMainType08(DefocusLayer *layer, XglPacket *packet);
void DefocusMainType10(DefocusLayer *layer, XglPacket *packet);

extern DefocusSolidPacketHeader Head_6_002BC5C0;
extern DefocusCopyPacket TestPrim_1;
extern DefocusTintPacket TestPrim_2;
extern u64 TestPrim_3[6];
extern DefocusTexturedHeader Head_0;
extern DefocusTexturedHeader Head_4_002BC4F0;
extern DefocusType05Header Head_5;
extern DefocusCopyTemplate Copy_10;
extern DefocusTexturedHeader Head_11;
extern u64 Head_13[10];
extern u64 Tail_15[8];
extern DefocusTexturedHeader Head_12;
extern DefocusTexturedHeader Head_7;
extern DefocusSolidPacketHeader Head_8;
extern u8 rate_9[37];

void GameDefocusCheck(void);
void GameDefocusSet(int index, int type, const int *settings);

static u64 GetTex0(int type, int tex_cc)
{
    switch (type) {
    case 0:
        return ((u64)0x24020000 | (u64)(sRender.tex0_state_20 << 5) |
                ((u64)0x9000 << 18) | ((u64)(u32)tex_cc << 34));
    case 1:
        return ((u64)0x24020000 | (u64)(sRender.tex0_state_22 << 5) |
                ((u64)0x9000 << 18) | ((u64)(u32)tex_cc << 34));
    case 2:
        return ((u64)0x24020000 | (u64)(sRender.tex0_state_14 << 5) |
                ((u64)0x9000 << 18) | ((u64)(u32)tex_cc << 34));
    default:
        return 0;
    }
}

void DefocusMainType01(DefocusLayer *layer, XglPacket *packet)
{
    DefocusTexturedPacket *draw = (DefocusTexturedPacket *)SCRATCHPAD_BASE;
    u8 *color;
    int right;
    int bottom;
    u32 vertex_color;
    u32 repeat = 0;

    Head_0.texture = GetTex0(layer->parameters[0], 0);
    draw->gif_tag = 0x51000006;
    draw->vif_command = DEFOCUS_PACKET_VIF_COMMAND;
    draw->gs_mode = 0x50ab4000;
    draw->gs_registers = 0x53531;
    draw->header[0] = 0;
    draw->header[1] = 0;
    draw->header[2] = 0;
    draw->header_padding = 0;
    color = (u8 *)&layer->parameters[3];
    draw->red = color[0];
    draw->green = color[1];
    draw->blue = color[2];
    draw->alpha = color[3];
    draw->second_u = DEFOCUS_SCREEN_WIDTH_FIXED;
    draw->first_u = 0;
    draw->left = (layer->parameters[4] << 4) + DEFOCUS_SCREEN_X_BIAS;
    right = layer->parameters[4] + layer->parameters[6];
    draw->second_v = DEFOCUS_SCREEN_HEIGHT_FIXED;
    draw->first_v = 0;
    draw->right = (right << 4) + DEFOCUS_SCREEN_X_END;
    draw->top = (layer->parameters[5] << 4) + DEFOCUS_SCREEN_Y_BIAS;
    bottom = layer->parameters[5] + layer->parameters[7];
    draw->bottom = (bottom << 4) + DEFOCUS_SCREEN_Y_END;
    vertex_color = layer->parameters[2];
    draw->second_color = vertex_color;
    draw->color = vertex_color;
    draw->second_flags = 0;
    draw->flags = 0;
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, &Head_0, 24);
    if (layer->parameters[1] != 0) {
        do {
            repeat++;
            sceVif1PkAddDataN(packet, draw, 28);
        } while (repeat < (u32)layer->parameters[1]);
    }
}

void DefocusMainType02(DefocusLayer *layer, XglPacket *packet)
{
    int tint_color;

    TestPrim_1.framebuffer = (u64)sRender.tex0_state_14 | 0x80000;
    TestPrim_1.texture = (u64)0x24020000 | (sRender.tex0_state_20 << 5) |
                        ((u64)0xc800 << 19);
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, &TestPrim_1, 60);
    tint_color = layer->parameters[1];
    if (tint_color != 0) {
        TestPrim_2.first_color = tint_color;
        TestPrim_2.source_frame = (u64)sRender.tex0_state_14 | 0x80000;
        TestPrim_2.second_color = layer->parameters[1];
        TestPrim_2.destination_frame = (u64)sRender.tex0_state_20 | 0x80000;
        sceVif1PkCnt(packet, 0);
        sceVif1PkAddDataN(packet, &TestPrim_2, 36);
        return;
    }
    TestPrim_3[4] = (u64)sRender.tex0_state_20 | 0x80000;
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, TestPrim_3, 12);
}

void DefocusMainType03(DefocusLayer *layer, XglPacket *packet)
{
    DefocusTexturedPacket *draw = (DefocusTexturedPacket *)SCRATCHPAD_BASE;
    u8 *color;
    int right;
    int bottom;
    int bottom_origin;
    int bottom_extent;
    u32 repeat = 0;

    Head_4_002BC4F0.texture = GetTex0(2, 1);
    draw->gif_tag = 0x51000006;
    draw->vif_command = DEFOCUS_PACKET_VIF_COMMAND;
    draw->gs_mode = 0x50ab4000;
    draw->gs_registers = 0x53531;
    draw->header[0] = 0;
    draw->header[1] = 0;
    draw->header[2] = 0;
    draw->header_padding = 0;
    color = (u8 *)&layer->parameters[1];
    draw->red = color[0];
    draw->green = color[1];
    draw->blue = color[2];
    draw->alpha = color[3];
    draw->second_u = DEFOCUS_SCREEN_WIDTH_FIXED;
    draw->first_u = 0;
    draw->left = (layer->parameters[2] << 4) + DEFOCUS_SCREEN_X_BIAS;
    right = layer->parameters[2] + layer->parameters[4];
    draw->second_v = DEFOCUS_SCREEN_HEIGHT_FIXED;
    draw->first_v = 0;
    draw->right = (right << 4) + DEFOCUS_SCREEN_X_END;
    draw->top = (layer->parameters[3] << 4) + DEFOCUS_SCREEN_Y_BIAS;
    bottom_origin = layer->parameters[3];
    bottom_extent = layer->parameters[5];
    draw->second_color = 0xffffffff;
    draw->color = 0xffffffff;
    bottom = bottom_origin + bottom_extent;
    draw->bottom = (bottom << 4) + DEFOCUS_SCREEN_Y_END;
    draw->second_flags = 0;
    draw->flags = 0;
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, &Head_4_002BC4F0, 24);
    if (layer->parameters[0] != 0) {
        do {
            repeat++;
            sceVif1PkAddDataN(packet, draw, 28);
        } while (repeat < (u32)layer->parameters[0]);
    }
}

void DefocusMainType05(DefocusLayer *layer, XglPacket *packet)
{
    const u8 *color;
    int x;
    int y;
    int right;
    int bottom;
    u32 draw_color;
    u64 texture;
    u64 width_dimensions;
    u64 height_dimensions;
    u64 texture_dimensions;

    texture = GetTex0(0, 0);
    Head_5.texture = texture;
    width_dimensions =
        (((long long)layer->parameters[6] ^ 0x3ffULL) << 4) | 15;
    height_dimensions =
        ((long long)layer->parameters[7] ^ 0x3ffULL) << 24;
    texture_dimensions = width_dimensions | height_dimensions;
    Head_5.texture_dimensions = texture_dimensions;
    DEFOCUS_TYPE05_PACKET->gif_tag = 0x51000006;
    DEFOCUS_TYPE05_PACKET->vif_command = DEFOCUS_PACKET_VIF_COMMAND;
    DEFOCUS_TYPE05_PACKET->header[0] = 0;
    DEFOCUS_TYPE05_PACKET->header[1] = 0;
    DEFOCUS_TYPE05_PACKET->header[2] = 0;
    DEFOCUS_TYPE05_PACKET->gs_mode = 0x50ab4000;
    DEFOCUS_TYPE05_PACKET->gs_registers = 0x53531;
    DEFOCUS_TYPE05_PACKET->header_padding = 0;
    color = (const u8 *)&layer->parameters[1];
    DEFOCUS_TYPE05_PACKET->red = color[0];
    DEFOCUS_TYPE05_PACKET->green = color[1];
    DEFOCUS_TYPE05_PACKET->blue = color[2];
    DEFOCUS_TYPE05_PACKET->alpha = color[3];
    x = layer->parameters[2] << 4;
    DEFOCUS_TYPE05_PACKET->first_u = x;
    DEFOCUS_TYPE05_PACKET->left = x + DEFOCUS_SCREEN_X_BIAS;
    y = layer->parameters[3] << 4;
    DEFOCUS_TYPE05_PACKET->first_v = y;
    DEFOCUS_TYPE05_PACKET->top = y + DEFOCUS_SCREEN_Y_BIAS;
    right = (layer->parameters[2] + layer->parameters[4]) << 4;
    DEFOCUS_TYPE05_PACKET->second_u = right;
    DEFOCUS_TYPE05_PACKET->right = right + DEFOCUS_SCREEN_X_BIAS;
    bottom = (layer->parameters[3] + layer->parameters[5]) << 4;
    DEFOCUS_TYPE05_PACKET->second_v = bottom;
    DEFOCUS_TYPE05_PACKET->bottom = bottom + DEFOCUS_SCREEN_Y_BIAS;
    draw_color = layer->parameters[0];
    DEFOCUS_TYPE05_PACKET->second_flags = 0;
    DEFOCUS_TYPE05_PACKET->second_color = draw_color;
    DEFOCUS_TYPE05_PACKET->color = draw_color;
    DEFOCUS_TYPE05_PACKET->flags = 0;
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, &Head_5, 28);
    sceVif1PkAddDataN(packet, DEFOCUS_TYPE05_PACKET, 28);
}

void DefocusMainType06(DefocusLayer *layer, XglPacket *packet)
{
    DefocusSolidPacket *draw;
    unsigned char *color;
    u32 left;
    u32 top;
    u32 draw_color;

    draw = DEFOCUS_SCRATCH;
    color = (unsigned char *)&layer->parameters[1];
    draw->gif_tag = DEFOCUS_SOLID_GIF_TAG;
    draw->vif_command = DEFOCUS_PACKET_VIF_COMMAND;
    draw->gs_registers = DEFOCUS_SOLID_GS_MODE;
    draw->register_count = DEFOCUS_SOLID_GS_REGISTERS;
    draw->header[0] = 0;
    draw->header[1] = 0;
    draw->header[2] = 0;
    draw->header_padding = 0;
    draw->red = color[0];
    draw->green = color[1];
    draw->blue = color[2];
    draw->alpha = color[3];
    left = ((u32)layer->parameters[2] << 4) + DEFOCUS_SCREEN_X_BIAS;
    draw->left = left;
    draw->right = left + ((u32)layer->parameters[4] << 4);
    top = ((u32)layer->parameters[3] << 4) + DEFOCUS_SCREEN_Y_BIAS;
    draw->top = top;
    draw->bottom = top + ((u32)layer->parameters[5] << 4);
    draw_color = layer->parameters[0];
    draw->color = draw_color;
    draw->second_color = draw_color;
    draw->flags = 0;
    draw->second_flags = 0;
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, &Head_6_002BC5C0, DEFOCUS_HEAD_WORDS);
    sceVif1PkAddDataN(packet, DEFOCUS_SCRATCH, DEFOCUS_SOLID_WORDS);
}

void DefocusMainType07(DefocusLayer *layer, XglPacket *packet)
{
    const u8 *color;
    float angle;
    float next_angle;
    float sin_angle;
    float cos_angle;
    float sin_next;
    float cos_next;
    float uv_radius_x;
    float uv_radius_y;
    float screen_radius_x;
    float screen_radius_y;
    int ring;
    int step;
    int offset_u;
    int offset_v;
    int offset_x;
    int offset_y;
    u32 flags;
    u32 skip_flag = 0x8000;
    DefocusWaveHeader *header = DEFOCUS_WAVE_HEADER;

    Head_7.texture = GetTex0(2, 0);
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, &Head_7, 24);
    header->gif_tag = 0x51000443;
    header->gs_mode = 0x10aa4000;
    header->vif_command = 1;
    header->gs_registers = 1;
    header->header[0] = 0;
    header->header[1] = 0;
    header->header[2] = 0;
    header->header_padding = 0;
    color = (const u8 *)&layer->parameters[1];
    header->red = color[0];
    header->green = color[1];
    header->blue = color[2];
    header->alpha = color[3];
    header->sprite_tag_low = 0x8220;
    header->sprite_tag_high = 0x20000000;
    header->sprite_registers = 0x53;
    header->sprite_padding = 0;
    sceVif1PkAddDataN(packet, DEFOCUS_WAVE_HEADER, 16);
    for (ring = 0; ring < 32; ring++) {
        angle = I2F(ring) * 0.03125f * 2.0f * 3.1415927f;
        next_angle = I2F(ring + 1) * 0.03125f * 2.0f * 3.1415927f;
        sin_angle = xglSin(angle);
        cos_angle = xglCos(angle);
        sin_next = xglSin(next_angle);
        cos_next = xglCos(next_angle);
        for (step = 0; step < 8; step++) {
            flags = 0;
            if (step == 0) {
                flags = skip_flag;
            }
            screen_radius_x = I2F(layer->parameters[4] * (8 - step)) * 0.125f;
            screen_radius_y = I2F(layer->parameters[5] * (8 - step)) * 0.125f;
            uv_radius_x = I2F(layer->parameters[4]) -
                          I2F(layer->parameters[4] * step * layer->parameters[6]) *
                              0.00048828125f;
            uv_radius_y = I2F(layer->parameters[5]) -
                          I2F(layer->parameters[5] * step * layer->parameters[6]) *
                              0.00048828125f;
            offset_u = F2I(cos_angle * uv_radius_x);
            offset_v = F2I(sin_angle * uv_radius_y);
            offset_x = F2I(cos_angle * screen_radius_x);
            offset_y = F2I(sin_angle * screen_radius_y);
            ((int *)SCRATCHPAD_BASE)[0] = (layer->parameters[2] + offset_u) << 4;
            ((int *)SCRATCHPAD_BASE)[1] = (layer->parameters[3] + offset_v) << 4;
            ((int *)SCRATCHPAD_BASE)[4] =
                ((layer->parameters[2] + offset_x) << 4) + DEFOCUS_SCREEN_X_BIAS;
            ((int *)SCRATCHPAD_BASE)[5] =
                ((layer->parameters[3] + offset_y) << 4) + DEFOCUS_SCREEN_Y_BIAS;
            ((int *)SCRATCHPAD_BASE)[6] = layer->parameters[0];
            ((int *)SCRATCHPAD_BASE)[7] = flags;
            offset_u = F2I(cos_next * uv_radius_x);
            offset_v = F2I(sin_next * uv_radius_y);
            offset_x = F2I(cos_next * screen_radius_x);
            offset_y = F2I(sin_next * screen_radius_y);
            ((int *)SCRATCHPAD_BASE)[8] = (layer->parameters[2] + offset_u) << 4;
            ((int *)SCRATCHPAD_BASE)[9] = (layer->parameters[3] + offset_v) << 4;
            ((int *)SCRATCHPAD_BASE)[12] =
                ((layer->parameters[2] + offset_x) << 4) + DEFOCUS_SCREEN_X_BIAS;
            ((int *)SCRATCHPAD_BASE)[13] =
                ((layer->parameters[3] + offset_y) << 4) + DEFOCUS_SCREEN_Y_BIAS;
            ((int *)SCRATCHPAD_BASE)[14] = layer->parameters[0];
            ((int *)SCRATCHPAD_BASE)[15] = flags;
            sceVif1PkAddDataN(packet, DEFOCUS_RADIAL_VERTICES, 16);
        }
        ((int *)SCRATCHPAD_BASE)[0] = layer->parameters[2] << 4;
        ((int *)SCRATCHPAD_BASE)[1] = layer->parameters[3] << 4;
        ((int *)SCRATCHPAD_BASE)[4] = (layer->parameters[2] << 4) + DEFOCUS_SCREEN_X_BIAS;
        ((int *)SCRATCHPAD_BASE)[5] = (layer->parameters[3] << 4) + DEFOCUS_SCREEN_Y_BIAS;
        ((int *)SCRATCHPAD_BASE)[6] = layer->parameters[0];
        ((int *)SCRATCHPAD_BASE)[7] = 0;
        sceVif1PkAddDataN(packet, DEFOCUS_RADIAL_VERTICES, 8);
    }
}

void DefocusMainType08(DefocusLayer *layer, XglPacket *packet)
{
    DefocusStarHeader *header = DEFOCUS_STAR_HEADER;
    float base_angle;
    float spread;
    float angle;
    float sine;
    float cosine;
    u32 index = 0;
    int x;
    int y;
    int spark_depth;
    int spark_x;
    int spark_y;
    int along_x;
    int along_y;
    int across_x;
    int across_y;
    int segment;
    int depth;
    int ring;
    u32 row;
    u32 spawn_x;
    u32 spawn_y;
    u32 lifetime;
    u32 *slot;
    int *shared;
    DefocusStarVertex *strip;
    u32 phase;
    int shade0;
    int shade1;
    int shade2;
    int shade3;
    int shade4;
    int shade5;
    int level2;
    int level3;

    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, &Head_8, 16);
    header->gif_tag = 0x51000009;
    header->vif_command = DEFOCUS_PACKET_VIF_COMMAND;
    header->gs_mode = 0x8026c000;
    header->gs_registers = 0x55555151;
    header->header[0] = 0;
    header->header[1] = 0;
    header->header[2] = 0;
    header->header_padding = 0;
    base_angle = I2F(layer->parameters[3]) / 180.0f * 3.1415927f;
    spread = I2F(layer->parameters[4]) / 180.0f * 3.1415927f;
    spark_depth = layer->parameters[0];
    if (layer->parameters[2] != 0) {
        do {
            index++;
            spark_x = xglSRand() & 0x1fff;
            spark_y = xglSRand() & 0x1fff;
            angle = base_angle + (xglFRand() - 1.5f) * spread;
            spark_x += 28672;
            spark_y += 29184;
            along_x = F2I(xglCos(angle) * 4.0f * 16.0f);
            along_y = F2I(xglSin(angle) * 4.0f * 16.0f);
            across_x = F2I(xglSin(angle) * 128.0f * 16.0f);
            across_y = F2I(-xglCos(angle) * 128.0f * 16.0f);
            DEFOCUS_STAR_WORD(8) = 255;
            DEFOCUS_STAR_WORD(9) = 255;
            DEFOCUS_STAR_WORD(10) = 255;
            DEFOCUS_STAR_WORD(11) = 64;
            DEFOCUS_STAR_WORD(12) = spark_x;
            DEFOCUS_STAR_WORD(13) = spark_y;
            DEFOCUS_STAR_WORD(14) = spark_depth;
            DEFOCUS_STAR_WORD(15) = 0;
            DEFOCUS_STAR_WORD(16) = 255;
            DEFOCUS_STAR_WORD(17) = 255;
            DEFOCUS_STAR_WORD(18) = 255;
            DEFOCUS_STAR_WORD(19) = 0;
            DEFOCUS_STAR_WORD(20) = spark_x + along_x;
            DEFOCUS_STAR_WORD(21) = spark_y + along_y;
            DEFOCUS_STAR_WORD(36) = spark_x + along_x;
            DEFOCUS_STAR_WORD(37) = spark_y + along_y;
            DEFOCUS_STAR_WORD(22) = spark_depth;
            DEFOCUS_STAR_WORD(38) = spark_depth;
            DEFOCUS_STAR_WORD(23) = 0;
            DEFOCUS_STAR_WORD(39) = 0;
            DEFOCUS_STAR_WORD(24) = spark_x + across_x;
            DEFOCUS_STAR_WORD(25) = spark_y + across_y;
            DEFOCUS_STAR_WORD(26) = spark_depth;
            DEFOCUS_STAR_WORD(27) = 0;
            DEFOCUS_STAR_WORD(28) = spark_x - along_x;
            DEFOCUS_STAR_WORD(29) = spark_y - along_y;
            DEFOCUS_STAR_WORD(30) = spark_depth;
            DEFOCUS_STAR_WORD(31) = 0;
            DEFOCUS_STAR_WORD(32) = spark_x - across_x;
            DEFOCUS_STAR_WORD(33) = spark_y - across_y;
            DEFOCUS_STAR_WORD(34) = spark_depth;
            DEFOCUS_STAR_WORD(35) = 0;
            sceVif1PkAddDataN(packet, DEFOCUS_STAR_HEADER, 40);
        } while (index < (u32)layer->parameters[2]);
    }
    for (ring = 0; ring <= 32; ring++) {
        sine = xglSin(I2F(ring) * 2.0f * 3.1415927f * (1.0f / 32.0f));
        cosine = xglCos(I2F(ring) * 2.0f * 3.1415927f * (1.0f / 32.0f));
        DEFOCUS_RING_TABLE->inner[ring].x = F2I(cosine * 8.0f * 16.0f);
        DEFOCUS_RING_TABLE->inner[ring].y = F2I(sine * 8.0f * 16.0f);
        DEFOCUS_RING_TABLE->middle[ring].x = F2I(cosine * 28.0f * 16.0f);
        DEFOCUS_RING_TABLE->middle[ring].y = F2I(sine * 28.0f * 16.0f);
        DEFOCUS_RING_TABLE->outer[ring].x = F2I(cosine * 48.0f * 16.0f);
        DEFOCUS_RING_TABLE->outer[ring].y = F2I(sine * 48.0f * 16.0f);
    }
    DEFOCUS_STAR_WORD(3) = 0x51000181;
    DEFOCUS_STAR_WORD(4) = 0x80c0;
    DEFOCUS_STAR_WORD(5) = 0x20264000;
    DEFOCUS_STAR_WORD(6) = 0x51;
    DEFOCUS_STAR_WORD(0) = 0;
    DEFOCUS_STAR_WORD(1) = 0;
    DEFOCUS_STAR_WORD(2) = 0;
    DEFOCUS_STAR_WORD(7) = 0;
    for (row = 0; row < (u32)layer->parameters[5]; row++) {
        slot = (u32 *)&layer->parameters[8 + row];
        strip = DEFOCUS_STAR_STRIP;
        if ((*slot >> 24) == 0) {
            spawn_x = xglSRand();
            spawn_y = xglSRand();
            lifetime = xglSRand();
            *slot = ((((spawn_x & 0x1fff) + 28672) & 0xfff0) << 8) + (((spawn_y & 0x1fff) + 29184) >> 4) +
                    ((lifetime % (u32)layer->parameters[6] + 1) << 24);
        }
        *slot -= 0x1000000;
        x = (*slot >> 8) & 0xfff0;
        y = (*slot << 4) & 0xfff0;
        shared = layer->parameters;
        depth = *shared;
        phase = xglSRand() & 3;
        for (segment = 0; segment < 32; segment++) {
            shade0 = 0;
            if (!(rate_9[phase + segment] & 1)) {
                shade0 = 255;
            }
            strip[0].red = shade0;
            strip[0].green = shade0;
            strip[0].blue = shade0;
            strip[0].alpha = 0;
            strip[0].x = x + DEFOCUS_RING_TABLE->inner[segment].x;
            strip[0].y = y + DEFOCUS_RING_TABLE->inner[segment].y;
            strip[0].z = depth;
            strip[0].flags = 0x8000;
            shade1 = 0;
            if (!(rate_9[phase + segment + 1] & 1)) {
                shade1 = 255;
            }
            strip[1].alpha = 0;
            strip[1].red = shade1;
            strip[1].green = shade1;
            strip[1].blue = shade1;
            strip[1].x = x + DEFOCUS_RING_TABLE->inner[segment + 1].x;
            strip[1].y = y + DEFOCUS_RING_TABLE->inner[segment + 1].y;
            strip[1].z = depth;
            strip[1].flags = 0x8000;
            level2 = rate_9[phase + segment];
            shade2 = 0;
            if (!(level2 & 1)) {
                shade2 = 255;
            }
            strip[2].alpha = level2;
            strip[2].red = shade2;
            strip[2].green = shade2;
            strip[2].blue = shade2;
            strip[2].x = x + DEFOCUS_RING_TABLE->middle[segment].x;
            strip[2].y = y + DEFOCUS_RING_TABLE->middle[segment].y;
            strip[2].z = depth;
            strip[2].flags = 0;
            level3 = rate_9[phase + segment + 1];
            shade3 = 0;
            if (!(level3 & 1)) {
                shade3 = 255;
            }
            strip[3].red = shade3;
            strip[3].green = shade3;
            strip[3].blue = shade3;
            strip[3].alpha = level3;
            strip[3].x = x + DEFOCUS_RING_TABLE->middle[segment + 1].x;
            strip[3].y = y + DEFOCUS_RING_TABLE->middle[segment + 1].y;
            strip[3].z = depth;
            strip[3].flags = 0;
            shade4 = 0;
            if (!(rate_9[phase + segment] & 1)) {
                shade4 = 255;
            }
            strip[4].red = shade4;
            strip[4].green = shade4;
            strip[4].blue = shade4;
            strip[4].alpha = 0;
            strip[4].x = x + DEFOCUS_RING_TABLE->outer[segment].x;
            strip[4].y = y + DEFOCUS_RING_TABLE->outer[segment].y;
            strip[4].z = depth;
            strip[4].flags = 0;
            shade5 = 0;
            if (!(rate_9[phase + segment + 1] & 1)) {
                shade5 = 255;
            }
            strip[5].red = shade5;
            strip[5].green = shade5;
            strip[5].blue = shade5;
            strip[5].alpha = 0;
            strip[5].x = x + DEFOCUS_RING_TABLE->outer[segment + 1].x;
            strip[5].y = y + DEFOCUS_RING_TABLE->outer[segment + 1].y;
            strip[5].z = depth;
            strip[5].flags = 0;
            strip += 6;
        }
        sceVif1PkAddDataN(packet, DEFOCUS_STAR_HEADER, 1544);
    }
}

void DefocusMainType09(DefocusLayer *layer, XglPacket *packet)
{
    u64 *extbuf;

    if (layer->state == 0) {
        layer->parameters[0] = ScanLineInterpolate;
        layer->parameters[1] = (u32)DispEnv.flags & 1;
        if (layer->parameters[2] == 0) {
            ScanLineInterpolate = 2;
        }
        layer->state = 1;
    } else {
        ScanLineInterpolate = 2;
        DispEnv.dispfb2.fbp = sRender.tex0_state_14;
        DispEnv.pmode.en1 = 1;
        DispEnv.pmode.alp = 255;
        DispEnv.display2.dy = DispEnv.display1.dy;
    }
    extbuf = &DispEnv.extbuf;
    *extbuf = 0x2120000 | (sRender.tex0_state_14 << 5);
    DispEnv.extdata.sx = DispEnv.display1.dx + 7;
    DispEnv.extdata.sy = DispEnv.display1.dy / 2;
    DispEnv.extdata.smph = DispEnv.display1.magh;
    DispEnv.extdata.smpv = DispEnv.display1.magv;
    DispEnv.extdata.ww = 511;
    DispEnv.extdata.wh = 223;
    DispEnv.extwrite |= 1;
}

static void DefocusMainType09Final(DefocusLayer *layer)
{
    u64 enable = (u32)layer->parameters[1];

    ScanLineInterpolate = layer->parameters[0];
    DispEnv.pmode.en1 = enable;
    DispEnv.extwrite &= ~1ULL;
}

void DefocusMainType10(DefocusLayer *layer, XglPacket *packet)
{
    extern void GameDefocusSet(int index, int type, const int *settings);
    DefocusTexturedPacket *draw = (DefocusTexturedPacket *)SCRATCHPAD_BASE;
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;
    u8 red_decrement;
    u8 green_decrement;
    u8 blue_decrement;
    u8 alpha_decrement;
    int right;
    u32 vertex_color;

    switch (layer->state) {
    case 0:
        Copy_10.texture = 0x80000 | (u64)(sRender.tex0_state_20 << 5) |
                          ((u64)sRender.tex0_state_14 << 37) |
                          (0x8000ULL << 36);
        sceVif1PkCnt(packet, 0);
        sceVif1PkAddDataN(packet, &Copy_10, 28);
        layer->parameters[5] = DEFOCUS_COLOR_NEUTRAL;
        layer->state = 1;
        layer->parameters[6] = 0;
        layer->parameters[7] = 0;
        layer->parameters[8] = 0;
        layer->parameters[9] = 0;
        /* The initial frame copy is followed by the first fade update. */
    case 1:
        red_decrement = *(const u8 *)&layer->parameters[0];
        green_decrement = ((const u8 *)&layer->parameters[0])[1];
        blue_decrement = ((const u8 *)&layer->parameters[0])[2];
        alpha_decrement = ((const u8 *)&layer->parameters[0])[3];
        red = *(u8 *)&layer->parameters[5];
        green = ((u8 *)&layer->parameters[5])[1];
        blue = ((u8 *)&layer->parameters[5])[2];
        alpha = ((u8 *)&layer->parameters[5])[3];
        red = red >= red_decrement ? red - red_decrement : 0;
        green = green >= green_decrement ? green - green_decrement : 0;
        blue = blue >= blue_decrement ? blue - blue_decrement : 0;
        alpha = alpha >= alpha_decrement ? alpha - alpha_decrement : 0;
        if (alpha == 0) {
            GameDefocusSet(layer->index, 0, NULL);
            return;
        }
        *((u8 *)&layer->parameters[5]) = red;
        draw->red = red;
        ((u8 *)&layer->parameters[5])[1] = green;
        draw->green = green;
        ((u8 *)&layer->parameters[5])[2] = blue;
        draw->blue = blue;
        ((u8 *)&layer->parameters[5])[3] = alpha;
        draw->alpha = alpha;
        Head_11.texture = GetTex0(2, 0);
        draw->vif_command = DEFOCUS_PACKET_VIF_COMMAND;
        draw->gif_tag = 0x51000006;
        draw->gs_mode = 0x50ab4000;
        draw->gs_registers = 0x53531;
        draw->header[0] = 0;
        draw->header[1] = 0;
        draw->header[2] = 0;
        draw->header_padding = 0;
        layer->parameters[6] += layer->parameters[1];
        layer->parameters[7] += layer->parameters[2];
        layer->parameters[8] += layer->parameters[3];
        layer->parameters[9] += layer->parameters[4];
        draw->second_u = DEFOCUS_SCREEN_WIDTH_FIXED;
        draw->first_u = 0;
        draw->left = (layer->parameters[6] << 4) + DEFOCUS_SCREEN_X_BIAS;
        right = layer->parameters[6] + layer->parameters[8];
        draw->second_v = DEFOCUS_SCREEN_HEIGHT_FIXED;
        draw->first_v = 0;
        draw->right = (right << 4) + DEFOCUS_SCREEN_X_END;
        draw->top = (layer->parameters[7] << 4) + DEFOCUS_SCREEN_Y_BIAS;
        draw->bottom = ((layer->parameters[7] + layer->parameters[9]) << 4) + DEFOCUS_SCREEN_Y_END;
        vertex_color = 0xffffffff;
        draw->second_color = vertex_color;
        draw->color = vertex_color;
        draw->second_flags = 0;
        draw->flags = 0;
        sceVif1PkCnt(packet, 0);
        sceVif1PkAddDataN(packet, &Head_11, 24);
        sceVif1PkAddDataN(packet, draw, 28);
        break;
    }
}

void DefocusMainType11(DefocusLayer *layer, XglPacket *packet)
{
    const u8 *color;
    float phase_x;
    float phase_y;
    int index;
    int row;
    int column;
    int *wave_x;
    int *wave_y;
    int x;
    int y;
    int next_x;
    int next_y;
    int sprite_y;
    int sprite_next_y;
    DefocusWaveHeader *header = DEFOCUS_WAVE_HEADER;

    Head_12.texture = GetTex0(2, 0);
    sceVif1PkCnt(packet, 0);
    sceVif1PkAddDataN(packet, &Head_12, 24);
    header->gif_tag = 0x51000383;
    header->gs_mode = 0x10ab4000;
    header->header[0] = 0;
    header->header[1] = 0;
    header->header[2] = 0;
    header->vif_command = 1;
    header->header_padding = 0;
    header->gs_registers = 1;
    color = (const u8 *)&layer->parameters[1];
    header->red = color[0];
    header->green = color[1];
    header->blue = color[2];
    header->alpha = color[3];
    header->sprite_tag_low = 0x80e0;
    header->sprite_tag_high = 0x40000000;
    header->sprite_registers = 0x5353;
    header->sprite_padding = 0;
    sceVif1PkAddDataN(packet, DEFOCUS_WAVE_HEADER, 16);
    layer->parameters[8] += layer->parameters[4];
    layer->parameters[9] += layer->parameters[7];
    phase_x = I2F(layer->parameters[8] & 0xfff) * 0.0015339808f;
    phase_y = I2F(layer->parameters[9] & 0xfff) * 0.0015339808f;
    for (index = 0; index <= 16; index++) {
        DEFOCUS_WAVE_TABLE->x[index] =
            F2I(xglSin(phase_x) * I2F(layer->parameters[2]));
        phase_x += I2F(layer->parameters[3]) * (1.0f / 4096.0f);
        DEFOCUS_WAVE_TABLE->y[index] =
            F2I(xglSin(phase_y) * I2F(layer->parameters[5]));
        phase_y += I2F(layer->parameters[6]) * (1.0f / 4096.0f);
    }
    ((int *)SCRATCHPAD_BASE)[16] = 0;
    ((int *)SCRATCHPAD_BASE)[0] = 0;
    ((int *)SCRATCHPAD_BASE)[31] = 0;
    ((int *)SCRATCHPAD_BASE)[17] = 0;
    for (row = 0, wave_y = DEFOCUS_WAVE_TABLE->y, y = 0; row < 14; row++, y = row * 512) {
        column = 0;
        sprite_y = y + DEFOCUS_SCREEN_Y_BIAS;
        next_y = y + 512;
        sprite_next_y = y + 29687;
        wave_x = DEFOCUS_WAVE_TABLE->x;
        for (; column < 16; column++) {
            x = column * 512;
            next_x = x + 512;
            ((int *)SCRATCHPAD_BASE)[32] = x + wave_x[0];
            ((int *)SCRATCHPAD_BASE)[33] = y + wave_y[0];
            ((int *)SCRATCHPAD_BASE)[36] = x + DEFOCUS_SCREEN_X_BIAS;
            ((int *)SCRATCHPAD_BASE)[37] = sprite_y;
            ((int *)SCRATCHPAD_BASE)[38] = layer->parameters[0];
            ((int *)SCRATCHPAD_BASE)[39] = 0;
            ((int *)SCRATCHPAD_BASE)[40] = next_x + wave_x[1];
            ((int *)SCRATCHPAD_BASE)[41] = next_y + wave_y[1];
            ((int *)SCRATCHPAD_BASE)[44] = x + 29176;
            ((int *)SCRATCHPAD_BASE)[45] = sprite_next_y;
            ((int *)SCRATCHPAD_BASE)[46] = layer->parameters[0];
            ((int *)SCRATCHPAD_BASE)[47] = 0;
            sceVif1PkAddDataN(packet, DEFOCUS_WAVE_SPRITE, 16);
            wave_x++;
        }
        wave_y++;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/game_defocus", DefocusMain);

INCLUDE_ASM("asm/main/nonmatchings/game_defocus", GameDefocusCheck);

void GameDefocusFinalize(int index)
{
    DefocusLayer *layer;

    layer = &GameDefocusParam[index];
    if (layer->type == 9) {
        DefocusMainType09Final(layer);
    }
    layer->index = index;
    layer->state = 0;
    layer->type = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/game_defocus", GameDefocusSet);

INCLUDE_ASM("asm/main/nonmatchings/game_defocus", GameDefocusQuickSet);
