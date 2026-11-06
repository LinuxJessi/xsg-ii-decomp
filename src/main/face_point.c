#include "common.h"
#include "shared.h"
#include "main/face_point.h"

typedef float Matrix4[4][4];

extern int strcmp(const char *left, const char *right);
void xglVectorOuter(Vector4 *destination, const Vector4 *left,
                    const Vector4 *right);
void xglVectorNormal_001A9200(Vector4 *destination, const Vector4 *source);

typedef int s32;


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

/* _ApplyMatrix33: _ApplyMatrix33, verbatim X1 twin (high). */
INCLUDE_ASM("asm/main/nonmatchings/face_point", _ApplyMatrix33);

/* _ApplyMatrix: _ApplyMatrix, verbatim X1 twin (high). */
INCLUDE_ASM("asm/main/nonmatchings/face_point", _ApplyMatrix);

/* _ApplyMatrix2Mat: _ApplyMatrix2Mat, verbatim X1 twin (high). */
INCLUDE_ASM("asm/main/nonmatchings/face_point", _ApplyMatrix2Mat);

/* culling_matrix: culling_matrix (medium); builds the volume transform from the corner constants. */
INCLUDE_ASM("asm/main/nonmatchings/face_point", culling_matrix);

/* An empty per-volume debug display hook; the caller still passes the volume. */
void culling_cell_disp(CullingVolume *volume)
{
}

/* _FacePoint: _FacePoint, verbatim X1 twin (high). */
INCLUDE_ASM("asm/main/nonmatchings/face_point", _FacePoint);

/* _CheckLine: _CheckLine, verbatim X1 twin (high). */
INCLUDE_ASM("asm/main/nonmatchings/face_point", _CheckLine);

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

/* setup_occlusion: setup_occlusion, X1 twin (high). */
INCLUDE_ASM("asm/main/nonmatchings/face_point", setup_occlusion);

/* check_occlusion: check_occlusion (medium). */
INCLUDE_ASM("asm/main/nonmatchings/face_point", check_occlusion);

/* xglCullingCheck: xglCullingCheck, X1 twin (high). */
INCLUDE_ASM("asm/main/nonmatchings/face_point", xglCullingCheck);

s32 xglCullingExist(void) {
    return cullingExistFlag[0];
}

/* func_001B6CF8: no name proposed; no X1 twin with the same map-last-entry call. */
INCLUDE_ASM("asm/main/nonmatchings/face_point", func_001B6CF8);

extern signed char D_00966D10;
extern int D_00967AF4;

/* xglCullingMapInit: xglCullingMapInit (high); clears the culling state. */
void xglCullingMapInit(void) {
    cullingExistFlag[0] = 0;
    D_00966D10 = 0;
    D_00967AF4 = 0;
    s_nIgnoreCulling = 0;
}

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

/* xglCullingMapSet: xglCullingMapSet, X1 twin (high). */
INCLUDE_ASM("asm/main/nonmatchings/face_point", xglCullingMapSet);

/* xglCullingMapCreate: xglCullingMapCreate, X1 twin (high). */
INCLUDE_ASM("asm/main/nonmatchings/face_point", xglCullingMapCreate);

extern CullingVolume cullingVolumes[];

/* xglCullingMapDisp: xglCullingMapDisp (medium); loops culling_cell_disp over the volumes. */
void xglCullingMapDisp(void) {
    int i = 0;

    /* The index is stored as zero twice; both stores are in the original. */
    i = 0;
    while (i < cullingExistFlag[0]) {
        culling_cell_disp(&cullingVolumes[i]);
        i++;
    }
}

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
