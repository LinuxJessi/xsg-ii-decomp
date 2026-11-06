#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu272", func_0029C288);

#include "shared.h"

void mapTriangleEdgePlanes(void) {
    __asm__ __volatile__(
        "vmula.xz ACC, vf19, vf20\n"
        "vmadda.xz ACC, vf16, vf21\n"
        "vmadd.xz vf4, vf17, vf23\n"
        "vmula.xz ACC, vf19, vf21\n"
        "vmadda.xz ACC, vf17, vf22\n"
        "vmadd.xz vf5, vf18, vf23\n"
        "vmula.xz ACC, vf19, vf22\n"
        "vmadda.xz ACC, vf18, vf20\n"
        "vmadd.xz vf6, vf16, vf23\n"
        "vsubz.x vf1, vf4, vf4z\n"
        "vsubz.x vf2, vf5, vf5z\n"
        "vsubz.x vf3, vf6, vf6z\n"
        "vaddx.y vf1, vf0, vf2x\n"
        "vaddx.z vf1, vf0, vf3x\n"
        : : : "memory");
}

INCLUDE_ASM("asm/main/nonmatchings/tu272", func_0029C438);

INCLUDE_ASM("asm/main/nonmatchings/tu272", mapPointOutsideTriangle);

/* Distance from the point origin to the plane through point with normal normal. */
f32 mapPlaneDistance(const Vector4 *origin, const Vector4 *point, const Vector4 *normal) {
    f32 result;

    __asm__ __volatile__("lqc2 vf2, 0(%0)" : : "r"(origin) : "memory");
    __asm__ __volatile__("lqc2 vf3, 0(%0)" : : "r"(point) : "memory");
    __asm__ __volatile__("lqc2 vf4, 0(%0)" : : "r"(normal) : "memory");
    __asm__ __volatile__("vsub.xz vf1, vf3, vf2" : : : "memory");
    __asm__ __volatile__("vmul.xz vf5, vf1, vf4" : : : "memory");
    __asm__ __volatile__("vaddz.x vf1, vf5, vf5z" : : : "memory");
    __asm__ __volatile__("vaddy.x vf5, vf1, vf3y" : : : "memory");
    __asm__ __volatile__("qmfc2.ni %0, vf5" : "=r"(result) : : "memory");
    return result;
}

INCLUDE_ASM("asm/main/nonmatchings/tu272", mapFixupBlock);

INCLUDE_ASM("asm/main/nonmatchings/tu272", mapLoadBlock);

INCLUDE_ASM("asm/main/nonmatchings/tu272", mapFindBlockEntry);

INCLUDE_ASM("asm/main/nonmatchings/tu272", mapProbeInit);

INCLUDE_ASM("asm/main/nonmatchings/tu272", mapEdgeClosestPoint);

INCLUDE_ASM("asm/main/nonmatchings/tu272", func_0029C960);

int mapPointOutsideTriangle(const Vector4 *);

/* Partial layouts: only the offsets the functions of this file touch are named. */
typedef struct MapRecordKind {
    int kindCode;
    unsigned char unmodeled_04[4];
} MapRecordKind;

/* One collision triangle: three vertices, a plane vector and a kind. */
typedef struct MapRecord {
    unsigned short kind;
    unsigned char unmodeled_02[6];
    unsigned short vertexIndex[3];
    unsigned short planeIndex;
} MapRecord;

/* Triangle record set; also the probe state that the mapProbe* helpers fill. */
typedef struct MapRecordSet {
    unsigned char unmodeled_00[4];
    int recordCount;
    Vector4 *vertices;
    Vector4 *planes;
    MapRecord *records;
    MapRecordKind *kindTable;
    unsigned char unmodeled_18[0x18];
    Vector4 origin;
} MapRecordSet;

/* Plane distance of the probe origin to a record, or -1000.0f when the origin is outside the triangle. */
f32 mapRecordDistance(const MapRecordSet *set, const MapRecord *record)
{
  const Vector4 *origin = &set->origin;
  const Vector4 *vertices = set->vertices;
  const Vector4 *vertex0 = vertices + record->vertexIndex[0];
  const Vector4 *vertex1 = vertices + record->vertexIndex[1];
  const Vector4 *vertex2 = vertices + record->vertexIndex[2];
  __asm__ __volatile__("lqc2 vf16, 0(%0)" : : "r"(vertex0) : "memory");
  __asm__ __volatile__("lqc2 vf17, 0(%0)" : : "r"(vertex1) : "memory");
  __asm__ __volatile__("lqc2 vf18, 0(%0)" : : "r"(vertex2) : "memory");
  __asm__ __volatile__("vmr32.xyzw vf1, vf16" : : : "memory");
  __asm__ __volatile__("vmr32.xyzw vf2, vf17" : : : "memory");
  __asm__ __volatile__("vmr32.xyzw vf3, vf18" : : : "memory");
  __asm__ __volatile__("vmr32.xyzw vf20, vf1" : : : "memory");
  __asm__ __volatile__("vmr32.xyzw vf21, vf2" : : : "memory");
  __asm__ __volatile__("vmr32.xyzw vf22, vf3" : : : "memory");
  if (mapPointOutsideTriangle(origin) == 0)
  {
    return mapPlaneDistance(origin, vertex0, set->planes + record->planeIndex);
  }
  return -1000.0f;
}

INCLUDE_ASM("asm/main/nonmatchings/tu272", mapProbeRecordHit);

void mapProbeRecordHit(MapRecordSet *, MapRecord *, float);
int mapScanRegisteredNode(MapRecordSet *);

/* Scans the record table and passes each hit to mapProbeRecordHit unless its kind is 4. */
void mapProbeRecords(MapRecordSet *owner)
{
  MapRecord *record;
  int recordIndex;
  float value;
  int kind;
  MapRecordSet *set = owner;

  record = set->records;
  if (mapScanRegisteredNode(owner) == 0)
  {
    for (recordIndex = 0; recordIndex < set->recordCount; recordIndex++)
    {
      value = mapRecordDistance(set, record);
      if (value != -1000.0f)
      {
        kind = record->kind;
        if (set->kindTable != 0)
        {
          kind = set->kindTable[kind].kindCode;
        }
        if (kind != 4)
        {
          mapProbeRecordHit(set, record, value);
        }
      }
      record++;
    }
  }
}

INCLUDE_ASM("asm/main/nonmatchings/tu272", func_0029CF78);

INCLUDE_ASM("asm/main/nonmatchings/tu272", func_0029D0B8);

INCLUDE_ASM("asm/main/nonmatchings/tu272", func_0029D360);

INCLUDE_ASM("asm/main/nonmatchings/tu272", func_0029DA50);

INCLUDE_ASM("asm/main/nonmatchings/tu272", func_0029DB00);

INCLUDE_ASM("asm/main/nonmatchings/tu272", mapSetupCollisionObject);

INCLUDE_ASM("asm/main/nonmatchings/tu272", mapSnapToNearestTriangle);

int mapSetupCollisionObject(int, int, int);
void mapSnapToNearestTriangle(int, int);

void mapBindAndSnapObject(int firstValue, int secondValue) {
    if (mapSetupCollisionObject(firstValue, 0, secondValue) != 0) {
        mapSnapToNearestTriangle(firstValue, secondValue);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu272", func_0029DEF8);

INCLUDE_ASM("asm/main/nonmatchings/tu272", mapMoveProbe);

void mapProbeInit(int);
void mapMoveProbe(int, int, int);

/* Runs the move probe on the scratchpad probe state at 0x70000000. */
void mapProbeScratchMove(int position) {
    mapProbeInit(0x70000000);
    *(void **)0x7000001C = (void *)0x70000040;
    mapMoveProbe(position, 0, 0x70000000);
}

void func_0029E690(void) {

}

int func_0029E698(void) {
    return 0;
}
