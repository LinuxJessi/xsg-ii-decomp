#include "common.h"
#include "shared.h"

typedef float Matrix4[4][4];

extern int strcmp(const char *left, const char *right);

void xglVectorOuter(Vector4 *destination, const Vector4 *left,
                    const Vector4 *right);

void xglVectorNormal_001A9200(Vector4 *destination, const Vector4 *source);

typedef int s32;

void xglCullingIgnore(void);

/*
 * One registered culling volume: the three vectors xglCullingMapSet and
 * xglCullingMapCreate fill in from a 9-float box record, followed by the
 * derived state culling_matrix builds from them and setup_occlusion and
 * check_occlusion read back. position.w and scale.w are set to 1.0f by both
 * writers; rotation.w is never written.
 */
typedef struct CullingVolume {
    Vector4 position;
    Vector4 rotation;
    Vector4 scale;
    Vector4 normalizedDirection;
    Matrix4 transform;
    Matrix4 inverseTransform;
    Vector4 cullingCorners[4];
    Vector4 occlusionPlanes[5];
    float occlusionDepth;
    u8 unmodeled_154[12];
} CullingVolume;

/* The map table stores pointers to separately scaffolded volume and index data. */
typedef struct CullingMapEntry {
    const char *name;
    /* The unchanged loop tests these pointer fields only for nullness. */
    const float *entryCount;
    const s32 *source;
    u8 unmodeled_00C[4];
} CullingMapEntry;

/*
 * Declared as an incomplete array, not a scalar: cc1 -G8 would otherwise
 * classify a plain scalar extern as small data and emit a single
 * gp-relative load, but the original site is two absolute hi/lo
 * instructions; an incomplete-extent declaration keeps the target's
 * unknown size ineligible for that optimization.
 */
extern s32 cullingExistFlag[];

int strcmp(const char *, const char *);

extern int s_nIgnoreCulling;
extern const CullingMapEntry s_aCullingMap[70];

INCLUDE_ASM("asm/main/nonmatchings/face_point", func_001B5E80);

INCLUDE_ASM("asm/main/nonmatchings/face_point", func_001B5ECC);

INCLUDE_ASM("asm/main/nonmatchings/face_point", func_001B5F1C);

INCLUDE_ASM("asm/main/nonmatchings/face_point", func_001B5F94);

/* An empty per-volume debug display hook; the caller still passes the volume. */
void culling_cell_disp(CullingVolume *volume)
{
}

INCLUDE_ASM("asm/main/nonmatchings/face_point", func_001B61B8);

INCLUDE_ASM("asm/main/nonmatchings/face_point", func_001B626C);

void plane_from_points(const Vector4 *point0, const Vector4 *point1,
                              const Vector4 *point2, Vector4 *plane)
{
    Vector4 edge0;
    Vector4 edge1;
    Vector4 normal;

    edge0.x = point1->x - point0->x;
    edge0.y = point1->y - point0->y;
    edge0.z = point1->z - point0->z;
    edge1.x = point2->x - point0->x;
    edge1.y = point2->y - point0->y;
    edge1.z = point2->z - point0->z;
    xglVectorOuter(&normal, &edge0, &edge1);
    xglVectorNormal_001A9200(&normal, &normal);
    plane->x = normal.x;
    plane->y = normal.y;
    plane->z = normal.z;
    plane->w = -(normal.x * point0->x + normal.y * point0->y + normal.z * point0->z);
}

INCLUDE_ASM("asm/main/nonmatchings/face_point", func_001B6580);

INCLUDE_ASM("asm/main/nonmatchings/face_point", func_001B68E4);

INCLUDE_ASM("asm/main/nonmatchings/face_point", func_001B6BC4);

s32 xglCullingExist(void) {
    return cullingExistFlag[0];
}

INCLUDE_ASM("asm/main/nonmatchings/face_point", func_001B6CF8);

INCLUDE_ASM("asm/main/nonmatchings/face_point", func_001B6D78);

/*
 * Finds a named culling-map entry in the current map table and returns its
 * index or minus one. The table ends at the first row with entryCount and
 * source both zero.
 */
s32 check_culling_map(const char *name) {
    s32 result = -1;
    s32 i;

    for (i = 0; s_aCullingMap[i].entryCount != 0 || s_aCullingMap[i].source != 0; i++) {
        if (strcmp(s_aCullingMap[i].name, name) == 0) {
            result = i;
            break;
        }
    }
    return result;
}

INCLUDE_ASM("asm/main/nonmatchings/face_point", func_001B6E88);

INCLUDE_ASM("asm/main/nonmatchings/face_point", func_001B7130);

INCLUDE_ASM("asm/main/nonmatchings/face_point", func_001B72AC);

void xglCullingIgnore(void)
{
    s_nIgnoreCulling = 1;
}

void xglCullingIgnoreOff(void)
{
    s_nIgnoreCulling = 0;
}

/* Empty in this build: the original body is a bare return. */
void xglCullingMapDebug(void)
{
}
