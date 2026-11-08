#include "common.h"
#include "shared.h"

typedef union {
    u64 value;
    struct {
        u32 low;
        u32 high;
    } words;
} GifCommandData;

typedef struct {
    u32 control_low;
    u32 control_high;
    u32 registers_low;
    u32 registers_high;
} GifTag;

/* Each source point contributes three words to the rear-shadow packet. */
typedef struct {
    int x;
    int y;
    int z;
} DropShadowSourcePoint;

typedef struct {
    DropShadowSourcePoint *points[4];
} DropShadowBackEntry;

typedef struct {
    u32 x;
    u32 y;
    int z;
    u32 w;
} DropShadowPacketPoint;

typedef struct {
    DropShadowSourcePoint point;
    u32 valid;
} DropShadowSourceVertex;

typedef struct {
    float s;
    float t;
    u8 unmodeled_08[8];
    DropShadowPacketPoint point;
} DropShadowTexturedPoint;

/* One shadow quad packet: the back passes send RGBAQ + four XYZ2 (GIF
 * registers 0x055551ee), the front pass RGBAQ + four ST/XYZ2 pairs
 * (0x252521ee), so the vertex block is either point kind. */
typedef struct {
    GifCommandData dma_tag;
    u32 vif_nop;
    u32 vif_direct;
    GifTag gif_tag;
    u32 packet_header_20;
    u8 unmodeled_24[4];
    u32 test_environment_register;
    u8 unmodeled_2c[4];
    u32 render_state_register;
    u32 packet_header_34;
    u32 shadow_context_register;
    u32 unmodeled_3c;
    u32 color[4];
    union {
        DropShadowPacketPoint points[4];
        DropShadowTexturedPoint textured_points[4];
    };
} ActDropShadowPacket;

typedef struct {
    Matrix4 elements;
} ActShadowMatrix;

/* One 16-byte scratch vector: indexed as four floats while it is built, and
   passed to the Vector4 helpers and VU blocks as a whole. */
typedef union {
    float values[4];
    Vector4 vector;
} ActShadowScratchVector;

typedef struct {
    ActShadowScratchVector temporary;
    Vector4 outer_offset;
    Vector4 inner_offset;
    Vector4 axial_offset;
    u8 unmodeled_40[0x10];
    Vector4 perpendicular;
    ActShadowMatrix model;
    ActShadowMatrix flatten;
    ActShadowMatrix translated_end;
    ActShadowMatrix first_world;
    ActShadowMatrix second_world;
    Vector4 vertices[128];
    DropShadowSourceVertex source_vertices[128];
    u8 back_shadow_status[16];
    DropShadowBackEntry back_shadow_entries[16];
    u8 unmodeled_12b0[8];
    u8 camera_index;
    u8 unmodeled_12b9[2];
    u8 alpha;
    u8 unmodeled_12bc[4];
    ActDropShadowPacket packet;
} ActDropShadowWorkspace;

INCLUDE_ASM("asm/main/nonmatchings/act_3", DrawCircleShadow);

INCLUDE_ASM("asm/main/nonmatchings/act_3", DrawZeldaShadowSub);

INCLUDE_ASM("asm/main/nonmatchings/act_3", DrawZeldaShadow);

int DrawDropShadowSubChk(ActDropShadowWorkspace *shadow, u8 *quad_indices,
                               int source_index, int quad_count)
{
    int processed = 0;
    int status_sum = 0;
    u8 *status = shadow->back_shadow_status;
    DropShadowBackEntry *entry = shadow->back_shadow_entries;
    DropShadowSourceVertex *vertices;

    if (quad_count > 0) {
        vertices = &shadow->source_vertices[source_index];
        do {
            DropShadowSourceVertex *point0 = &vertices[quad_indices[0]];
            DropShadowSourceVertex *point1 = &vertices[quad_indices[1]];
            DropShadowSourceVertex *point2 = &vertices[quad_indices[2]];
            DropShadowSourceVertex *point3 = &vertices[quad_indices[3]];
            int x0;
            int x1;
            int x2;
            int y0;
            int y1;
            int y2;
            int orientation_word;

            if (point0->valid == 0 || point1->valid == 0 ||
                point2->valid == 0 || point3->valid == 0) {
                return 0;
            }
            x0 = point0->point.x;
            x2 = point2->point.x;
            y0 = point0->point.y;
            y1 = point1->point.y;
            x1 = point1->point.x;
            y2 = point2->point.y;
            entry->points[3] = &point3->point;
            entry->points[0] = &point0->point;
            entry->points[1] = &point1->point;
            entry->points[2] = &point2->point;
            entry++;

            orientation_word = x2 * y0 + x0 * y1 + x1 * y2 - x2 * y1 -
                               x0 * y2 - x1 * y0;
            if ((float)orientation_word >= 0.0f) {
                *status = 1;
            } else {
                *status = 2;
            }
            status_sum += *status;
            status++;
            quad_indices += 4;
            processed++;
        } while (processed < quad_count);
    }
    return status_sum;
}

INCLUDE_ASM("asm/main/nonmatchings/act_3", DrawDropShadowSubBack);

INCLUDE_ASM("asm/main/nonmatchings/act_3", DrawDropShadowSubFront);

INCLUDE_ASM("asm/main/nonmatchings/act_3", DrawDropShadowSub);

INCLUDE_ASM("asm/main/nonmatchings/act_3", DrawDropShadow);

INCLUDE_ASM("asm/main/nonmatchings/act_3", DrawDropCircle);

/* Named in the proposals: ACT_DrawShadowBegin (X1 ACT_DrawShadowBegin: sRender shadow state, 10 quadwords). */
INCLUDE_ASM("asm/main/nonmatchings/act_3", ACT_DrawShadowBegin);

void func_001C374C(int, void *, int);
extern unsigned char D_0063E4B0[];

/* Named in the proposals: ACT_DrawShadowEnd (X1 ACT_DrawShadowEnd: Tail_10, 3 quadwords). */
void ACT_DrawShadowEnd(void) {
    func_001C374C(8, D_0063E4B0, 3);
}

/* Named in the proposals: ACT_DrawShadow. Dispatches on shadow_type: 1 DrawCircleShadow (default),
 * 2 DrawZeldaShadow, 3 DrawDropCircle, 4 DrawDropShadow; the same switch as X1 ACT_DrawShadow. */
INCLUDE_ASM("asm/main/nonmatchings/act_3", ACT_DrawShadow);
