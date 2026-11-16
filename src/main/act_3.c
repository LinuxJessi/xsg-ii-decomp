#include "common.h"
#include "shared.h"
#include "gs.h"
#include "quadword.h"
#include "vu0_macro.h"
#include "main/sdv_033.h"
#include "main/tu093.h"

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

typedef struct {
    u32 low;
    u32 high;
} GifDmaTag;

/* One shadow quad packet: the back passes send RGBAQ + four XYZ2 (GIF
 * registers 0x055551ee), the front pass RGBAQ + four ST/XYZ2 pairs
 * (0x252521ee), so the vertex block is either point kind. */
typedef struct {
    GifDmaTag dma_tag;
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

void pxyAddEntry(int, void *, int);

typedef unsigned int ActShadowQuadword __attribute__((mode(TI)));

typedef union {
    ActShadowQuadword quadword;
    DropShadowPacketPoint value;
} ActZeldaShadowPoint;

typedef union {
    ActShadowQuadword quadword;
    struct {
        u32 red;
        u32 green;
        u32 blue;
        u32 alpha;
    } value;
} ActZeldaShadowColor;

typedef struct {
    ActZeldaShadowColor color;
    ActZeldaShadowPoint point;
} ActZeldaShadowVertex;

typedef struct {
    u64 command_words[10];
    ActZeldaShadowVertex vertices[6];
    u64 trailer[2];
} ActZeldaShadowPacket;

typedef struct {
    Vector4 translation;
    ActShadowMatrix matrix;
    ActShadowMatrix transform;
    Vector4 vertices[5];
    ActZeldaShadowPoint projected[5];
    float height;
    float angle;
    float radius;
    u32 camera_index;
    ActZeldaShadowPacket packet;
} ActShadowScratchpad;

void xglMatrixMul(Matrix4 destination, Matrix4 left, Matrix4 right);
void xglMatrixStackUnit(void);
void xglMatrixStackTrans(const float translation[4]);
void xglMatrixStackRotY(float angle);
void xglMatrixStackMulVector(Vector4 *destination, const Vector4 *source);
/* local view: xgl_2.c takes Vector4 *destination and const Vector4 *source; this file passes its own vertex views. */
void xglRotTransPersN(void *destination, Matrix4 matrix,
                      const void *vertices, int count, int camera_index);
int F2I(float value);
float I2F(int value);

#define SCREEN_CLIP_MASK 0xffff0000u
#define SCRATCHPAD_BASE 0x70000000
#define GROUND_HEIGHT_NONE (-1000.0f)
#define SHADOW_GROUND_LIFT 0.02f
#define SHADOW_CAMERA_SLOT_COUNT 8 /* camera slots searched for the active camera */
#define SHADOW_RING_CORNER_SCALE 0.7f /* diagonal ring points sit at 0.7 of the radius */
#define SHADOW_ALPHA_SCALE 90.0f /* shadow_alpha, 0..1, becomes the packet alpha 0..90 */
#define SHADOW_GROUND_SEARCH_START 100000.0f /* above any joint height: the lowest joint wins */
#define SHADOW_LINK_MAGIC 0x797870 /* "pxy" in the low three bytes of the link record */

/* Status of a drop-shadow quad, set by DrawDropShadowSubChk: 1 back-facing, 2 front-facing. */
enum {
    DROP_SHADOW_STATUS_BACK = 1,
    DROP_SHADOW_STATUS_FRONT = 2,
};

/* Shadow kinds of ACT_DrawShadow (actor->shadow_type). */
enum {
    SHADOW_TYPE_CIRCLE = 4,
    SHADOW_TYPE_ZELDA = 5,
    SHADOW_TYPE_DROP_CIRCLE = 6,
    SHADOW_TYPE_DROP_SHADOW = 7,
};

XglCamera *xglStudioGetCamera2(int camera_index);
float atan2f(float, float);
void xglMatrixUnit(Matrix4 matrix);
void xglMatrixRotY(Matrix4 destination, Matrix4 source, float angle);
extern u8 shadowCirclePointFlags[];
extern u8 D_0063E230[];
extern u8 D_0063E280[];

typedef struct {
    u64 command_words[4];
    ActZeldaShadowVertex vertices[8];
} ActCircleShadowPacket;

typedef struct {
    u8 unmodeled_000[0x10];
    ActShadowMatrix transform;
    ActShadowMatrix model;
    Vector4 vertices[16];
    ActZeldaShadowPoint projected[16];
    u32 camera_index;
    u8 unmodeled_294[0x0c];
    ActCircleShadowPacket packet;
} ActCircleShadowScratchpad;

typedef struct {
    ActShadowMatrix matrices[73];
} ActShadowMatrixSet;

typedef struct {
    u64 magic;
    u8 unmodeled_08[84];
    int count;
} ActDropCircleLink;

/* The actor fields the shadow code reads: the circle and Zelda shadows, the drop circle and
 * the drop shadow. shadow_center_offset is the local offset of the drop-circle centre, moved
 * through shadow_matrix. The ground search runs only while ground_link is set; link is the
 * record (magic "pxy") whose count says how many joint matrices, from shadow_matrices->matrices[2]
 * on, give candidate ground heights. The Zelda shadow reads light_direction (alternate_light
 * .w == 0) or alternate_light (w != 0) as its ground direction, the same selection the drop
 * shadow makes for its light. */
typedef struct {
    u8 unmodeled_000[0x10];
    float position[3];
    u8 unmodeled_01c[0x54 - 0x1c];
    float yaw;
    u8 unmodeled_058[0x80 - 0x58];
    Vector4 shadow_center_offset;
    u8 unmodeled_090[0xb0 - 0x90];
    float shadow_radius;
    u8 unmodeled_0b4[0xc4 - 0xb4];
    int shadow_type;
    u8 unmodeled_0c8[0xcc - 0xc8];
    float shadow_alpha;
    u8 unmodeled_0d0[0x100 - 0xd0];
    ActShadowMatrix shadow_matrix;
    u8 unmodeled_140[0x190 - 0x140];
    Vector4 light_direction;
    u8 unmodeled_1a0[0x280 - 0x1a0];
    Vector4 alternate_light;
    u8 unmodeled_290[0x5ec - 0x290];
    ActDropCircleLink *link;
    u8 unmodeled_5f0[0x62c - 0x5f0];
    void *ground_link;
    u8 unmodeled_630[0x720 - 0x630];
    ActShadowMatrixSet *shadow_matrices;
    u8 unmodeled_724[0x754 - 0x724];
    float height_override;
    u8 unmodeled_758[0x798 - 0x758];
    u32 kind;
} ActShadowActor;

/* Draws the round shadow of an actor: a sixteen-point ring projected from the
 * camera, skipped when a point is outside the GS window or behind the camera. */
void DrawCircleShadow(void *actor_pointer)
{
    ActShadowActor *actor = actor_pointer;
    ActCircleShadowScratchpad *scratchpad = (ActCircleShadowScratchpad *)SCRATCHPAD_BASE;
    float radius;
    float corner;
    int camera_index;
    int i;
    int batch;
    ActZeldaShadowPoint *projected;
    ActZeldaShadowVertex *output;
    u8 *point_flags;

    xglMatrixUnit(scratchpad->model.elements);
    scratchpad->model.elements[3][0] = actor->position[0];
    scratchpad->model.elements[3][2] = actor->position[2];
    if (actor->height_override == GROUND_HEIGHT_NONE) {
        scratchpad->model.elements[3][1] = actor->position[1] + SHADOW_GROUND_LIFT;
    } else {
        scratchpad->model.elements[3][1] = actor->height_override + SHADOW_GROUND_LIFT;
    }
    xglMatrixRotY(scratchpad->transform.elements, scratchpad->model.elements, actor->yaw);
    for (camera_index = 0; camera_index < SHADOW_CAMERA_SLOT_COUNT; camera_index++) {
        if (xglStudioGetCamera2(camera_index)->active == 1) {
            break;
        }
    }
    radius = actor->shadow_radius;
    corner = radius * SHADOW_RING_CORNER_SCALE;
    scratchpad->vertices[0].x = scratchpad->vertices[1].x =
        scratchpad->vertices[5].x = scratchpad->vertices[14].x =
        scratchpad->vertices[15].x = 0.0f;
    scratchpad->vertices[6].x = scratchpad->vertices[7].x =
        scratchpad->vertices[8].x = scratchpad->vertices[9].x =
        scratchpad->vertices[13].x = -scratchpad->vertices[0].x;
    scratchpad->vertices[2].x = scratchpad->vertices[4].x =
        radius - (radius - corner);
    scratchpad->vertices[10].x = scratchpad->vertices[12].x =
        -scratchpad->vertices[2].x;
    scratchpad->vertices[3].x = radius;
    scratchpad->vertices[11].x = -radius;
    scratchpad->vertices[1].y =
        scratchpad->vertices[2].y = scratchpad->vertices[3].y =
        scratchpad->vertices[4].y = scratchpad->vertices[5].y =
        scratchpad->vertices[6].y = scratchpad->vertices[7].y =
        scratchpad->vertices[8].y = scratchpad->vertices[9].y =
        scratchpad->vertices[10].y = scratchpad->vertices[11].y =
        scratchpad->vertices[12].y = scratchpad->vertices[13].y =
        scratchpad->vertices[14].y = scratchpad->vertices[15].y =
        scratchpad->vertices[0].x;
    scratchpad->camera_index = camera_index;
    scratchpad->vertices[0].y = scratchpad->vertices[0].x;
    scratchpad->vertices[3].z = scratchpad->vertices[7].z =
        scratchpad->vertices[8].z = scratchpad->vertices[11].z =
        scratchpad->vertices[15].z = scratchpad->vertices[0].x;
    scratchpad->vertices[0].z = scratchpad->vertices[0].x;
    scratchpad->vertices[2].z = scratchpad->vertices[12].z = corner;
    scratchpad->vertices[4].z = scratchpad->vertices[10].z = -corner;
    scratchpad->vertices[1].z = scratchpad->vertices[13].z =
        scratchpad->vertices[14].z = radius;
    scratchpad->vertices[5].z = scratchpad->vertices[6].z =
        scratchpad->vertices[9].z = -radius;
    scratchpad->vertices[0].w = scratchpad->vertices[1].w = scratchpad->vertices[2].w =
        scratchpad->vertices[3].w = scratchpad->vertices[4].w =
        scratchpad->vertices[5].w = scratchpad->vertices[6].w =
        scratchpad->vertices[7].w = scratchpad->vertices[8].w =
        scratchpad->vertices[9].w = scratchpad->vertices[10].w =
        scratchpad->vertices[11].w = scratchpad->vertices[12].w =
        scratchpad->vertices[13].w = scratchpad->vertices[14].w =
        scratchpad->vertices[15].w = 1.0f;
    xglRotTransPersN(scratchpad->projected, scratchpad->transform.elements,
                     scratchpad->vertices, 16, camera_index);
    {
        ActZeldaShadowPoint *clip = scratchpad->projected;
        int clip_index;

        for (clip_index = 0; clip_index < 16; clip_index++, clip++) {
            if (clip->value.x & SCREEN_CLIP_MASK) {
                return;
            }
            if (clip->value.y & SCREEN_CLIP_MASK) {
                return;
            }
            if (clip->value.z < 0) {
                return;
            }
        }
    }
    pxyAddEntry(8, D_0063E230, 5);
    point_flags = shadowCirclePointFlags;
    scratchpad->packet.command_words[1] = 0x5100001100000000ULL;
    scratchpad->packet.command_words[2] = 0x0026c00000008001ULL;
    scratchpad->packet.command_words[3] = 0x5151515151515151ULL;
    projected = scratchpad->projected;
    scratchpad->packet.command_words[0] = 0;
    for (batch = 0; batch < 2; batch++) {
        output = scratchpad->packet.vertices;
        for (i = 7; i >= 0; i--, output++, projected++, point_flags++) {
            output->color.value.red = 0;
            output->color.value.green = 0;
            output->color.value.blue = 0;
            if ((*point_flags & 1) == 0) {
                output->color.value.alpha = (u32)(int)(actor->shadow_alpha * SHADOW_ALPHA_SCALE);
            } else {
                output->color.value.alpha = 0;
            }
            output->point.value.x = projected->value.x;
            output->point.value.y = projected->value.y;
            output->point.value.z = projected->value.z;
            output->point.value.w = (*point_flags & 2) ? 0x8000 : 0;
        }
        pxyAddEntry(8, &scratchpad->packet, 18);
    }
    pxyAddEntry(8, D_0063E280, 3);
}

/* Projects the five points of one Zelda shadow contact and sends them as one
 * packet. The shadow is skipped when a point is outside the GS window or behind
 * the camera. */
void DrawZeldaShadowSub(ActShadowScratchpad *scratchpad, ActShadowMatrix *matrix)
{
    float alpha;
    int i;
    ActZeldaShadowPoint *point;

    xglMatrixMul(scratchpad->transform.elements, scratchpad->matrix.elements,
                 matrix->elements);
    scratchpad->translation.x = scratchpad->transform.elements[3][0];
    scratchpad->translation.y = scratchpad->transform.elements[3][1];
    scratchpad->translation.z = scratchpad->transform.elements[3][2];
    scratchpad->translation.w = 1.0f;
    xglMatrixStackUnit();
    xglMatrixStackTrans(&scratchpad->translation.x);
    xglMatrixStackRotY(scratchpad->angle);

    scratchpad->translation.x = 0.0f;
    scratchpad->translation.y = 0.0f;
    scratchpad->translation.z = 0.0f;
    xglMatrixStackMulVector(&scratchpad->vertices[0], &scratchpad->translation);
    scratchpad->translation.x = -0.1f;
    xglMatrixStackMulVector(&scratchpad->vertices[1], &scratchpad->translation);
    scratchpad->translation.x = 0.1f;
    xglMatrixStackMulVector(&scratchpad->vertices[3], &scratchpad->translation);
    scratchpad->translation.x = 0.0f;
    scratchpad->translation.z = -0.15f;
    xglMatrixStackMulVector(&scratchpad->vertices[2], &scratchpad->translation);
    scratchpad->translation.z = scratchpad->radius;
    xglMatrixStackMulVector(&scratchpad->vertices[4], &scratchpad->translation);

    xglRotTransPersN(scratchpad->projected, 0, scratchpad->vertices, 5,
                     scratchpad->camera_index);
    for (point = scratchpad->projected, i = 0; i < 5; i++, point++) {
        if (point->value.x & SCREEN_CLIP_MASK) {
            return;
        }
        if (point->value.y & SCREEN_CLIP_MASK) {
            return;
        }
        if (point->value.z < 0) {
            return;
        }
        point->value.w = 0;
    }

    scratchpad->packet.command_words[1] = 0x5100001100000000ULL;
    scratchpad->packet.command_words[2] = 0x0026c00000008001ULL;
    scratchpad->packet.command_words[3] = 0xe515151515151eeeULL;
    scratchpad->packet.command_words[4] = 0x0000000131000000ULL;
    scratchpad->packet.command_words[0] = 0;
    scratchpad->packet.command_words[5] = 0x4e;
    scratchpad->packet.command_words[6] = 0x53001;
    scratchpad->packet.command_words[7] = 0x47;
    scratchpad->packet.command_words[8] = 0x44;
    scratchpad->packet.command_words[9] = 0x42;
    /* Alpha falls off with the matrix height above the shadow height, clamped to 0..1. */
    alpha = (0.05f -
             ((matrix->elements[3][1] - scratchpad->height) -
              0.13f)) * 20.0f;
    if (alpha < 0.0f) {
        alpha = 0.0f;
    }
    if (alpha > 1.0f) {
        alpha = 1.0f;
    }

    scratchpad->packet.vertices[0].color.value.red = 0;
    scratchpad->packet.vertices[0].color.value.green = 0;
    scratchpad->packet.vertices[0].color.value.blue = 0;
    scratchpad->packet.vertices[0].color.value.alpha = F2I(alpha * 96.0f);
    scratchpad->packet.vertices[1].color.value.red = 0;
    scratchpad->packet.vertices[1].color.value.green = 0;
    scratchpad->packet.vertices[1].color.value.blue = 0;
    scratchpad->packet.vertices[1].color.value.alpha = 0;
    /* Vertex-copy block, kept as the original inline asm: vertex 0/1 points
     * from projected[0..1], then vertex 1's colour quadword (in $2) copied to
     * vertices 2..5 and the remaining points (in $3) from projected[1..4].
     * Plain 128-bit C copies emit the same lq/sq but cannot reproduce the
     * original $2/$3 allocation around the packet trailer. */
    __asm__ __volatile__(
        "lq $2, 0(%1)\n\t"
        "lq $3, 16(%1)\n\t"
        "sq $2, 16(%0)\n\t"
        "sq $3, 48(%0)\n\t"
        "lq $2, 32(%0)\n\t"
        "sq $2, 160(%0)\n\t"
        "sq $3, 176(%0)\n\t"
        "lq $3, 32(%1)\n\t"
        "sq $2, 64(%0)\n\t"
        "sq $3, 80(%0)\n\t"
        "lq $3, 48(%1)\n\t"
        "sq $2, 96(%0)\n\t"
        "sq $3, 112(%0)\n\t"
        "lq $3, 64(%1)\n\t"
        "sq $2, 128(%0)\n\t"
        "sq $3, 144(%0)\n\t"
        :
        : "r"(&scratchpad->packet.vertices[0].color),
          "r"(scratchpad->projected)
        : "$2", "$3", "memory");
    scratchpad->packet.trailer[0] = 0x31000000;
    scratchpad->packet.trailer[1] = 0x4e;
    pxyAddEntry(8, &scratchpad->packet, 18);
}

/* Draws the round shadow of a Zelda-style actor from its ground direction. */
void DrawZeldaShadow(void *actor_pointer)
{
    ActShadowActor *actor = actor_pointer;
    ActShadowScratchpad *scratchpad = (ActShadowScratchpad *)SCRATCHPAD_BASE;
    int camera_index;
    float *direction;

    if (actor->shadow_matrices == 0) {
        return;
    }
    if (actor->height_override == GROUND_HEIGHT_NONE) {
        scratchpad->height = actor->position[1] + SHADOW_GROUND_LIFT;
    } else {
        scratchpad->height = actor->height_override + SHADOW_GROUND_LIFT;
    }
    for (camera_index = 0; camera_index < SHADOW_CAMERA_SLOT_COUNT; camera_index++) {
        if (xglStudioGetCamera2(camera_index)->active == 1) {
            break;
        }
    }
    scratchpad->camera_index = camera_index;
    {
        ActShadowMatrix *destination = &scratchpad->matrix;
        ActShadowMatrix *matrix = &actor->shadow_matrices->matrices[1];
        /* Copies the four quadwords of the matrix through a scratch register. */
        QuadwordCopyMatrix(destination, matrix);
    }
    if (actor->alternate_light.w == 0.0f) {
        direction = &actor->light_direction.x;
    } else {
        direction = &actor->alternate_light.x;
    }
    scratchpad->radius = __builtin_sqrtf(1.0f - direction[1] * direction[1]) * 0.5f;
    if (scratchpad->radius < 0.15f) {
        scratchpad->radius = 0.15f;
    }
    scratchpad->angle = atan2f(-direction[0], -direction[2]);
    DrawZeldaShadowSub(scratchpad, &actor->shadow_matrices->matrices[9]);
    DrawZeldaShadowSub(scratchpad, &actor->shadow_matrices->matrices[5]);
}

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
                *status = DROP_SHADOW_STATUS_BACK;
            } else {
                *status = DROP_SHADOW_STATUS_FRONT;
            }
            status_sum += *status;
            status++;
            quad_indices += 4;
            processed++;
        } while (processed < quad_count);
    }
    return status_sum;
}

/* Sends each back-facing shadow quad (status 1) as one packet of four XYZ2 points. */
void DrawDropShadowSubBack(ActDropShadowWorkspace *shadow, int quad_count,
                           int draw_mode)
{
    int remaining;
    u8 *status;
    DropShadowBackEntry *entry;
    int active_status;

    entry = shadow->back_shadow_entries;
    shadow->packet.vif_direct = 0x51000008;
    shadow->packet.gif_tag.control_high = 0x70024000;
    shadow->packet.gif_tag.registers_low = GIF_REGS_RGBAQ_XYZ2_X4;
    shadow->packet.packet_header_20 = 0x00071001;
    shadow->packet.packet_header_34 = 0x7fffffff;
    shadow->packet.color[3] = draw_mode;
    shadow->packet.color[0] = 0;
    shadow->packet.color[1] = 0;
    shadow->packet.color[2] = 0;
    shadow->packet.points[0].w = 0;
    shadow->packet.points[1].w = 0;
    shadow->packet.points[2].w = 0;
    shadow->packet.points[3].w = 0;

    if (quad_count <= 0) {
        return;
    }

    remaining = quad_count;
    active_status = DROP_SHADOW_STATUS_BACK;
    status = shadow->back_shadow_status;

    /* Status bytes and back-shadow entries advance together, one per quad. */
    do {
        if (*status == active_status) {
            /* Pointer form: a store through the vertex union itself is alias
             * set 0 and keeps the entry->points loads behind it. */
            (shadow->packet.points + 0)->x = entry->points[0]->x;
            (shadow->packet.points + 0)->y = entry->points[0]->y;
            (shadow->packet.points + 0)->z = entry->points[0]->z;
            (shadow->packet.points + 1)->x = entry->points[1]->x;
            (shadow->packet.points + 1)->y = entry->points[1]->y;
            (shadow->packet.points + 1)->z = entry->points[1]->z;
            (shadow->packet.points + 2)->x = entry->points[2]->x;
            (shadow->packet.points + 2)->y = entry->points[2]->y;
            (shadow->packet.points + 2)->z = entry->points[2]->z;
            (shadow->packet.points + 3)->x = entry->points[3]->x;
            (shadow->packet.points + 3)->y = entry->points[3]->y;
            (shadow->packet.points + 3)->z = entry->points[3]->z;
            pxyAddEntry(8, &shadow->packet, 9);
        }
        remaining--;
        status++;
        entry++;
    } while (remaining != 0);
}

/* Texture coordinate origin and scale of the textured drop-shadow quads. */
#define DROP_SHADOW_TEXTURE_U_ORIGIN 0x6ff8
#define DROP_SHADOW_TEXTURE_V_ORIGIN 0x71f7
#define DROP_SHADOW_TEXTURE_COORDINATE_SCALE (1.0f / 8192.0f)

/* Sends each front-facing shadow quad (status 2) as one textured packet. */
void DrawDropShadowSubFront(ActDropShadowWorkspace *shadow, int quad_count)
{
    float coordinate_scale = DROP_SHADOW_TEXTURE_COORDINATE_SCALE;
    DropShadowTexturedPoint *points = shadow->packet.textured_points;
    DropShadowBackEntry *entry = shadow->back_shadow_entries;
    u8 *status;
    int remaining;

    shadow->packet.vif_direct = 0x5100000c;
    {
        u64 primitive = ((u64)shadow->alpha << 4) | 0x70009;
        u32 alpha = shadow->alpha;
        shadow->packet.gif_tag.control_high = 0xb02a4000;
        shadow->packet.gif_tag.registers_low = GIF_REGS_RGBAQ_ST_XYZ2_X4;
        shadow->packet.gif_tag.registers_high = 0x525;
        shadow->packet.packet_header_20 = primitive;
        shadow->packet.color[3] = alpha;
    }
    shadow->packet.packet_header_34 = 0;
    shadow->packet.color[0] = 0;
    shadow->packet.color[1] = 0;
    shadow->packet.color[2] = 0;
    points[0].point.w = 0;
    points[1].point.w = 0;
    points[2].point.w = 0;
    points[3].point.w = 0;
    if (quad_count > 0) {
        remaining = quad_count;
        status = shadow->back_shadow_status;
        do {
            if (*status == DROP_SHADOW_STATUS_FRONT) {
                points[0].s = I2F(entry->points[0]->x - DROP_SHADOW_TEXTURE_U_ORIGIN) * coordinate_scale;
                points[0].t = I2F(entry->points[0]->y - DROP_SHADOW_TEXTURE_V_ORIGIN) * coordinate_scale;
                points[0].point.x = entry->points[0]->x;
                points[0].point.y = entry->points[0]->y;
                points[0].point.z = entry->points[0]->z;
                points[1].s = I2F(entry->points[1]->x - DROP_SHADOW_TEXTURE_U_ORIGIN) * coordinate_scale;
                points[1].t = I2F(entry->points[1]->y - DROP_SHADOW_TEXTURE_V_ORIGIN) * coordinate_scale;
                points[1].point.x = entry->points[1]->x;
                points[1].point.y = entry->points[1]->y;
                points[1].point.z = entry->points[1]->z;
                points[2].s = I2F(entry->points[2]->x - DROP_SHADOW_TEXTURE_U_ORIGIN) * coordinate_scale;
                points[2].t = I2F(entry->points[2]->y - DROP_SHADOW_TEXTURE_V_ORIGIN) * coordinate_scale;
                points[2].point.x = entry->points[2]->x;
                points[2].point.y = entry->points[2]->y;
                points[2].point.z = entry->points[2]->z;
                points[3].s = I2F(entry->points[3]->x - DROP_SHADOW_TEXTURE_U_ORIGIN) * coordinate_scale;
                points[3].t = I2F(entry->points[3]->y - DROP_SHADOW_TEXTURE_V_ORIGIN) * coordinate_scale;
                points[3].point.x = entry->points[3]->x;
                points[3].point.y = entry->points[3]->y;
                points[3].point.z = entry->points[3]->z;
                pxyAddEntry(8, &shadow->packet, 13);
            }
            status++;
            remaining--;
            entry++;
        } while (remaining != 0);
    }
}

#define SHADOW_CONTACT_END 255
#define SHADOW_VERTEX_COUNT 16
#define SHADOW_CIRCLE_QUAD_COUNT 14
#define SHADOW_DROP_CONTACT_DEPTH 0.5f
#define SHADOW_CONTACT_COORDINATE_SCALE 0.01f
#define SHADOW_RENDER_STATE_FLAG 0x80000

/* One contact of a drop-shadow list: the matrix index, then either the segment
 * widths or, on the terminal record, the offset and axis of the end point. */
typedef union {
    struct {
        signed char end_width;
        signed char midpoint_width;
    } segment;
    struct {
        signed char offset;
        signed char axis;
    } terminal;
} DropShadowContactPayload;

typedef struct {
    u8 matrix_index;
    DropShadowContactPayload payload;
} DropShadowContactRecord;

typedef struct {
    u8 quad_indices[14][4];
} DropShadowQuadIndexTable;

extern const DropShadowQuadIndexTable dropShadowContactQuadIndices;
extern u8 dropShadowSeparatorQuads[];

/* local view: xgl_2.c defines xglMatrixTrans on Matrix4Block; this file's callers pass Matrix4 views. */
void xglMatrixTrans(Matrix4 destination, Matrix4 source,
                    const float translation[4]);
void xglVectorOuter(Vector4 *destination, const Vector4 *first,
                    const float *second);
void xglVectorLength(float *length, const Vector4 *vector);
void xglVectorScaleXYZ(Vector4 *destination, const Vector4 *source, float scale);
void xglVectorMulMat(Vector4 *destination, Matrix4 matrix, const Vector4 *vector);

/* Projects each contact segment's shadow onto the ground and sends its quads;
 * returns the contact list position after the terminal record. */
DropShadowContactRecord *DrawDropShadowSub(ActShadowActor *actor,
                                           ActDropShadowWorkspace *work,
                                           DropShadowContactRecord *contacts)
{
    float ground_height;
    float length;
    int segments;
    int vertex_count;
    int visible;
    int i;
    ActShadowMatrix *translated_end;
    Matrix4 *model;
    Vector4 *perpendicular;
    Vector4 *inner_offset;
    Vector4 *axial_offset;
    Matrix4 *second_world;
    float *light_direction;
    Vector4 *vertices = work->vertices;
    Matrix4 *first_world;
    float *first_position;
    float *second_position;
    Vector4 *points = vertices;
    DropShadowSourceVertex *projected;

    if (actor->height_override == GROUND_HEIGHT_NONE) {
        ground_height = actor->position[1] - SHADOW_DROP_CONTACT_DEPTH;
    } else {
        ground_height = actor->height_override - SHADOW_DROP_CONTACT_DEPTH;
    }
    segments = 0;
    if (contacts[0].matrix_index != SHADOW_CONTACT_END) {
        first_world = &work->first_world.elements;
        second_world = &work->second_world.elements;
        light_direction = &actor->light_direction.x;
        translated_end = &work->translated_end;
        model = &work->model.elements;
        perpendicular = &work->perpendicular;
        inner_offset = &work->inner_offset;
        axial_offset = &work->axial_offset;
        first_position = (*first_world)[3];
        second_position = (*second_world)[3];

        do {
            ActShadowMatrix *first = &actor->shadow_matrices->matrices[contacts[segments].matrix_index];
            ActShadowMatrix *second;
            float *selected_light;
            float *minuend;
            float *subtrahend;
            DropShadowContactRecord *next_contact = &contacts[segments + 1];
            DropShadowContactRecord *contact = &contacts[segments];

            if (next_contact->matrix_index != SHADOW_CONTACT_END) {
                second = &actor->shadow_matrices->matrices[next_contact->matrix_index];
            } else {
                work->temporary.values[0] = 0.0f;
                work->temporary.values[1] = 0.0f;
                work->temporary.values[2] = 0.0f;
                work->temporary.values[next_contact->payload.terminal.axis] =
                    I2F(next_contact->payload.terminal.offset) * SHADOW_CONTACT_COORDINATE_SCALE;
                work->temporary.values[3] = 1.0f;
                xglMatrixTrans(translated_end->elements, first->elements,
                               work->temporary.values);
                second = translated_end;
            }
            xglMatrixMul(*first_world, *model, first->elements);
            xglMatrixMul(*second_world, *model, second->elements);

            subtrahend = first_position;
            minuend = second_position;
            selected_light = light_direction;
            if (actor->alternate_light.w != 0.0f) {
                selected_light = &actor->alternate_light.x;
            }
            Vu0SubVectorXYZ(work->temporary.values, minuend, subtrahend);
            xglVectorOuter(perpendicular, &work->temporary.vector, selected_light);
            xglVectorLength(&length, perpendicular);
            xglVectorScaleXYZ(&work->outer_offset, perpendicular,
                              I2F(contact->payload.segment.end_width) * SHADOW_CONTACT_COORDINATE_SCALE / length);
            xglVectorScaleXYZ(inner_offset, perpendicular,
                              I2F(contact->payload.segment.midpoint_width) * SHADOW_CONTACT_COORDINATE_SCALE / length);
            xglVectorLength(&length, &work->temporary.vector);
            xglVectorScaleXYZ(axial_offset, &work->temporary.vector,
                              I2F(contact->payload.segment.end_width) * SHADOW_CONTACT_COORDINATE_SCALE / length);

            work->temporary.values[3] = ground_height;
            work->temporary.values[0] = first_position[0];
            work->temporary.values[1] = first_position[1];
            work->temporary.values[2] = first_position[2];
            Vu0AddVectorXYZ(&points[0], work->temporary.values, &work->outer_offset);
            xglVectorMulMat(&points[8], work->flatten.elements, &points[0]);
            points[0].w = 1.0f;
            points[8].w = 1.0f;
            Vu0SubVectorXYZ(&points[1], work->temporary.values, &work->outer_offset);
            xglVectorMulMat(&points[9], work->flatten.elements, &points[1]);
            points[1].w = 1.0f;
            points[9].w = 1.0f;
            Vu0SubVectorXYZ(&points[4], work->temporary.values, axial_offset);
            xglVectorMulMat(&points[12], work->flatten.elements, &points[4]);
            points[4].w = 1.0f;
            points[12].w = 1.0f;

            work->temporary.values[0] = second_position[0];
            work->temporary.values[1] = second_position[1];
            work->temporary.values[2] = second_position[2];
            Vu0AddVectorXYZ(&points[2], work->temporary.values, &work->outer_offset);
            xglVectorMulMat(&points[10], work->flatten.elements, &points[2]);
            points[2].w = 1.0f;
            points[10].w = 1.0f;
            Vu0SubVectorXYZ(&points[3], work->temporary.values, &work->outer_offset);
            xglVectorMulMat(&points[11], work->flatten.elements, &points[3]);
            points[3].w = 1.0f;
            points[11].w = 1.0f;
            Vu0AddVectorXYZ(&points[5], work->temporary.values, axial_offset);
            xglVectorMulMat(&points[13], work->flatten.elements, &points[5]);
            points[5].w = 1.0f;
            points[13].w = 1.0f;

            work->temporary.values[0] = (first_position[0] + second_position[0]) * 0.5f;
            work->temporary.values[1] = (first_position[1] + second_position[1]) * 0.5f;
            work->temporary.values[2] = (first_position[2] + second_position[2]) * 0.5f;
            Vu0AddVectorXYZ(&points[6], work->temporary.values, inner_offset);
            xglVectorMulMat(&points[14], work->flatten.elements, &points[6]);
            points[6].w = 1.0f;
            points[14].w = 1.0f;
            Vu0SubVectorXYZ(&points[7], work->temporary.values, inner_offset);
            xglVectorMulMat(&points[15], work->flatten.elements, &points[7]);
            points[7].w = 1.0f;
            points[15].w = 1.0f;
            points += 16;
            segments++;
        } while (contacts[segments].matrix_index != SHADOW_CONTACT_END);
    }

    /* Project every segment vertex, then mark the ones outside the GS window or
     * behind the camera as invalid. */
    vertex_count = segments * SHADOW_VERTEX_COUNT;
    xglRotTransPersN(work->source_vertices, 0, vertices, vertex_count,
                     work->camera_index);
    visible = 0;
    projected = work->source_vertices;
    if (vertex_count > 0) {
        int remaining = vertex_count;

        do {
            if ((projected->point.x & SCREEN_CLIP_MASK) || (projected->point.y & SCREEN_CLIP_MASK)
                || projected->point.z < 0) {
                projected->valid = 0;
            } else {
                visible = 1;
            }
            projected++;
        } while (--remaining != 0);
    }
    if (!visible) {
        return contacts + segments + 1;
    }

    work->alpha = (int)(actor->shadow_alpha * SHADOW_ALPHA_SCALE);
    work->packet.gif_tag.control_low = 0x8001;
    work->packet.test_environment_register = GS_TEST_1;
    work->packet.shadow_context_register = GS_FRAME_1;
    work->packet.dma_tag.low = 0;
    work->packet.dma_tag.high = 0;
    work->packet.vif_nop = 0;
    work->packet.render_state_register = sRender.display_base | SHADOW_RENDER_STATE_FLAG;
    for (i = 0; i < segments; i++) {
        DropShadowQuadIndexTable indices = dropShadowContactQuadIndices;

        if (DrawDropShadowSubChk(work, &indices.quad_indices[0][0],
                                 i * SHADOW_VERTEX_COUNT, SHADOW_CIRCLE_QUAD_COUNT)) {
            DrawDropShadowSubBack(work, SHADOW_CIRCLE_QUAD_COUNT, 0);
            pxyAddEntry(9, dropShadowSeparatorQuads, 3);
            DrawDropShadowSubFront(work, SHADOW_CIRCLE_QUAD_COUNT);
            DrawDropShadowSubBack(work, SHADOW_CIRCLE_QUAD_COUNT, 128);
        }
    }
    return contacts + segments + 1;
}

/* One A+D register write of the GIF packets: the value word, then the register address word. */
typedef struct {
    u64 data;
    u64 register_address;
} GifAdQuad;

typedef struct {
    u32 kind_mask;
    u32 kind_value;
    DropShadowContactRecord *contacts;
} DropShadowContactDefinition;

extern GifAdQuad dropShadowHeaderQuads[5];
extern GifAdQuad dropShadowTailQuads[3];
extern DropShadowContactDefinition dropShadowContactDefinitions[];

void DrawDropShadow(ActShadowActor *actor)
{
    ActDropShadowWorkspace *work = (ActDropShadowWorkspace *)SCRATCHPAD_BASE;
    DropShadowContactDefinition *entry;
    DropShadowContactRecord *contacts;
    Vector4 *light;
    u32 actor_kind;
    int camera_index;

    {
        u64 alpha_descriptor = ((u64)(int)(actor->shadow_alpha * SHADOW_ALPHA_SCALE) << 32) | 100;
        dropShadowHeaderQuads[2].data = alpha_descriptor;
    }
    {
        int render_state_bits = 0x24020000 | (sRender.display_base << 5);
        dropShadowHeaderQuads[3].data = ((u64)0xC800 << 19) | render_state_bits;
    }
    pxyAddEntry(8, dropShadowHeaderQuads, 5);
    for (camera_index = 0; camera_index < SHADOW_CAMERA_SLOT_COUNT; camera_index++) {
        if (xglStudioGetCamera2(camera_index)->active == 1) {
            break;
        }
    }
    work->camera_index = camera_index;
    QuadwordCopyMatrix(&work->model, &actor->shadow_matrix);
    light = &actor->light_direction;
    if (actor->alternate_light.w != 0.0f) {
        light = &actor->alternate_light;
    }
    {
        float light_x = light->x;
        float light_z = light->z;
        work->flatten.elements[0][0] = 1.0f;
        work->flatten.elements[0][1] = 0.0f;
        work->flatten.elements[0][2] = 0.0f;
        work->flatten.elements[0][3] = 0.0f;
        work->flatten.elements[1][0] = -light_x;
        work->flatten.elements[1][1] = 0.0f;
        work->flatten.elements[1][2] = -light_z;
        work->flatten.elements[1][3] = 0.0f;
        work->flatten.elements[2][0] = 0.0f;
        work->flatten.elements[2][1] = 0.0f;
        work->flatten.elements[2][2] = 1.0f;
        work->flatten.elements[2][3] = 0.0f;
        work->flatten.elements[3][0] = light_x;
        work->flatten.elements[3][1] = 1.0f;
        work->flatten.elements[3][2] = light_z;
        work->flatten.elements[3][3] = 0.0f;
    }
    /* Use the first definition whose kind mask matches the actor kind. */
    actor_kind = actor->kind;
    for (entry = dropShadowContactDefinitions; entry->contacts != 0; entry++) {
        if ((actor_kind & entry->kind_mask) == entry->kind_value) {
            contacts = entry->contacts;
            while (contacts->matrix_index != SHADOW_CONTACT_END) {
                /* Draws one run of contact records and returns the record after it. */
                contacts = DrawDropShadowSub(actor, work, contacts);
            }
            break;
        }
    }
    {
        u64 shadow_register = (u64)sRender.display_base | SHADOW_RENDER_STATE_FLAG;
        dropShadowTailQuads[2].data = shadow_register;
    }
    pxyAddEntry(8, dropShadowTailQuads, 3);
}

extern const DropShadowQuadIndexTable dropCircleQuadIndices;
extern GifAdQuad dropCircleHeaderQuads[5];
extern GifAdQuad dropCircleSeparatorQuads[3];
extern GifAdQuad dropCircleTailQuads[3];

void DrawDropCircle(ActShadowActor *actor)
{
    ActDropShadowWorkspace *scratch = (ActDropShadowWorkspace *)SCRATCHPAD_BASE;
    Vector4 *vertices = scratch->vertices;
    float *center = scratch->temporary.values;
    float radius;
    float corner;
    float ground;
    int camera;
    int visible;
    int i;
    DropShadowQuadIndexTable indices;
    DropShadowSourceVertex *projected;
    ActDropCircleLink *link;

    {
        u64 alpha_descriptor = ((u64)(int)(actor->shadow_alpha * SHADOW_ALPHA_SCALE) << 32) | 100;
        dropCircleHeaderQuads[2].data = alpha_descriptor;
    }
    {
        u64 render_state_bits = (u64)(0x24020000 | (sRender.display_base << 5));
        dropCircleHeaderQuads[3].data = render_state_bits | ((u64)0xC800 << 19);
    }
    pxyAddEntry(8, dropCircleHeaderQuads, 5);
    for (camera = 0; camera < SHADOW_CAMERA_SLOT_COUNT; camera++) {
        if (xglStudioGetCamera2(camera)->active == 1) {
            break;
        }
    }
    scratch->camera_index = camera;
    center[0] = actor->position[0];
    center[2] = actor->position[2];
    if (actor->height_override == GROUND_HEIGHT_NONE) {
        center[1] = actor->position[1] + SHADOW_GROUND_LIFT;
    } else {
        center[1] = actor->height_override + SHADOW_GROUND_LIFT;
    }
    if (actor->ground_link != 0) {
        if (actor->shadow_center_offset.x != 0.0f || actor->shadow_center_offset.y != 0.0f ||
            actor->shadow_center_offset.z != 0.0f) {
            xglVectorMulMat(&scratch->temporary.vector, actor->shadow_matrix.elements, &actor->shadow_center_offset);
            link = actor->link;
            if (link != 0 && (link->magic & 0xffffff) == SHADOW_LINK_MAGIC) {
                /* The lowest joint height (matrix translation y) is the ground under the circle. */
                ActShadowMatrix *joint = &actor->shadow_matrices->matrices[2];
                ground = SHADOW_GROUND_SEARCH_START;
                i = link->count;
                if (i > 0) {
                    int remaining = i;
                    do {
                        if (joint->elements[3][1] + actor->position[1] < ground) {
                            ground = joint->elements[3][1] + actor->position[1];
                        }
                        joint++;
                        remaining--;
                    } while (remaining != 0);
                }
                center[1] = ground;
            }
        }
    }
    radius = actor->shadow_radius;
    /* Vertices 0-7 form the ring at the ground height; 8-15 copy their x/z below it. */
    vertices[4].x = vertices[5].x = center[0];
    corner = radius * SHADOW_RING_CORNER_SCALE;
    vertices[0].x = vertices[2].x = center[0] + corner;
    vertices[1].x = vertices[3].x = center[0] - corner;
    vertices[6].x = center[0] + radius;
    vertices[7].x = center[0] - radius;
    vertices[0].y = vertices[1].y = vertices[2].y = vertices[3].y =
        vertices[4].y = vertices[5].y = vertices[6].y = vertices[7].y = center[1] + 0.2f;
    vertices[6].z = vertices[7].z = center[2];
    vertices[0].z = vertices[1].z = center[2] + corner;
    vertices[2].z = vertices[3].z = center[2] - corner;
    vertices[4].z = center[2] + radius;
    vertices[5].z = center[2] - radius;
    vertices[0].w = vertices[1].w = vertices[2].w = vertices[3].w =
        vertices[4].w = vertices[5].w = vertices[6].w = vertices[7].w = 1.0f;
    vertices[8].x = vertices[0].x;
    vertices[8].z = vertices[0].z;
    vertices[9].x = vertices[1].x;
    vertices[9].z = vertices[1].z;
    vertices[10].x = vertices[2].x;
    vertices[10].z = vertices[2].z;
    vertices[11].x = vertices[3].x;
    vertices[11].z = vertices[3].z;
    vertices[12].x = vertices[4].x;
    vertices[12].z = vertices[4].z;
    vertices[13].x = vertices[5].x;
    vertices[13].z = vertices[5].z;
    vertices[14].x = vertices[6].x;
    vertices[14].z = vertices[6].z;
    vertices[15].x = vertices[7].x;
    vertices[15].z = vertices[7].z;
    vertices[8].y = vertices[9].y = vertices[10].y = vertices[11].y =
        vertices[12].y = vertices[13].y = vertices[14].y = vertices[15].y = center[1] - 0.5f;
    vertices[8].w = vertices[9].w = vertices[10].w = vertices[11].w =
        vertices[12].w = vertices[13].w = vertices[14].w = vertices[15].w = 1.0f;
    xglRotTransPersN(scratch->source_vertices, 0, scratch->vertices, 16, scratch->camera_index);
    projected = scratch->source_vertices;
    visible = 0;
    for (i = 0; i < SHADOW_VERTEX_COUNT; i++, projected++) {
        if ((projected->point.x & SCREEN_CLIP_MASK) != 0) {
            projected->valid = 0;
        } else if ((projected->point.y & SCREEN_CLIP_MASK) != 0) {
            projected->valid = 0;
        } else if (projected->point.z < 0) {
            projected->valid = 0;
        } else {
            visible = 1;
        }
    }
    if (visible == 0) {
        return;
    }
    scratch->packet.gif_tag.control_low = 0x8001;
    scratch->alpha = (u8)(int)(actor->shadow_alpha * SHADOW_ALPHA_SCALE);
    scratch->packet.test_environment_register = GS_TEST_1;
    scratch->packet.render_state_register = SHADOW_RENDER_STATE_FLAG | sRender.display_base;
    scratch->packet.shadow_context_register = GS_FRAME_1;
    indices = dropCircleQuadIndices;
    scratch->packet.dma_tag.low = 0;
    scratch->packet.dma_tag.high = 0;
    scratch->packet.vif_nop = 0;
    if (DrawDropShadowSubChk(scratch, &indices.quad_indices[0][0], 0, SHADOW_CIRCLE_QUAD_COUNT) != 0) {
        DrawDropShadowSubBack(scratch, SHADOW_CIRCLE_QUAD_COUNT, 0);
        pxyAddEntry(9, dropCircleSeparatorQuads, 3);
        DrawDropShadowSubFront(scratch, SHADOW_CIRCLE_QUAD_COUNT);
        DrawDropShadowSubBack(scratch, SHADOW_CIRCLE_QUAD_COUNT, 128);
        dropCircleTailQuads[2].data = (u64)sRender.display_base | SHADOW_RENDER_STATE_FLAG;
        pxyAddEntry(8, dropCircleTailQuads, 3);
    }
}

extern GifAdQuad actShadowBeginQuads[10];

void ACT_DrawShadowBegin(void) {
    actShadowBeginQuads[5].data = GS_SET_FRAME(sRender.display_base, 8, 0, 0x00ffffff);
    actShadowBeginQuads[9].data = GS_SET_FRAME(sRender.display_base, 8, 0, 0);
    pxyAddEntry(8, actShadowBeginQuads, 10);
}

extern u8 D_0063E4B0[];

void ACT_DrawShadowEnd(void) {
    pxyAddEntry(8, D_0063E4B0, 3);
}

/* Draws the shadow selected by shadow_type. Types 0-3 and 8-9 draw nothing, 4 and
 * every value from 10 up draw a circle shadow, 5 a Zelda shadow, 6 a drop circle and
 * 7 a drop shadow. */
void ACT_DrawShadow(ActShadowActor *actor)
{
    if (actor->shadow_matrices == 0) {
        return;
    }
    switch (actor->shadow_type) {
    default:
    case SHADOW_TYPE_CIRCLE:
        DrawCircleShadow(actor);
        break;
    case SHADOW_TYPE_ZELDA:
        DrawZeldaShadow(actor);
        break;
    case SHADOW_TYPE_DROP_CIRCLE:
        DrawDropCircle(actor);
        break;
    case SHADOW_TYPE_DROP_SHADOW:
        DrawDropShadow(actor);
        break;
    case 0:
    case 1:
    case 2:
    case 3:
    case 8:
    case 9:
        break;
    }
}
