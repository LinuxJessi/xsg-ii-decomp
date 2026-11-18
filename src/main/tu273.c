#include "common.h"
#include "shared.h"
#include "main/tu272.h"

/* Scan-tree constants: box axes, tree depth and capacity, node alignment, record state byte. */
#define MAP_AXIS_X 0
#define MAP_AXIS_Z 2
#define MAP_SCAN_MAX_DEPTH 8
#define MAP_SCAN_MAX_RECORDS 3073
#define MAP_SCAN_SLOT_COUNT 4
#define MAP_SCAN_ALIGN 16
#define MAP_SCAN_ALIGN_MASK 0xF
#define MAP_SCAN_TOLERANCE_RATIO 0.01f
#define MAP_RECORD_KIND_NO_PROBE 4
#define MAP_BLOCK_TYPE_RECORD_SETS 3
/* Record state byte: 0xFF once placed in a node, low 6 bits hold depth + 1, top 2 bits hold the side flags. */
#define MAP_RECORD_PLACED 0xFF
#define MAP_DEPTH_MASK 0x3F
#define MAP_CLASS_MASK 0xC0
#define MAP_CLASS_BACK 0x40
#define MAP_CLASS_FRONT 0x80
#define MAP_CLASS_STRADDLE 0xC0

/* One map vertex viewed as coord[4] for the split axis; the same 16 bytes as Vector4. */
typedef struct MapVertex {
    float coord[4];
} MapVertex;

/* Scan box corner, copied by value with doubleword moves. */
typedef union MapBoxCorner {
    float coord[4];
    u64 words[2];
} MapBoxCorner;

/* Scan-pool and record-state storage of this TU (uninitialized, static: .bss). Labels not used by name are merged. */
static DoublewordVector4 mapBoundsMin;
static DoublewordVector4 mapBoundsMax;
static unsigned char mapRecordStates[3072];
static MapScanNode D_0098ABF0[1601];
static unsigned char D_00991000[2048];
static unsigned char D_00991800[2048];
static unsigned char D_00992000[2048];
static unsigned char D_00992800[128];
static unsigned char D_00992880[3136];
static unsigned char D_009934C0[2240];
static unsigned char D_00993D80[512];
static unsigned char D_00993F80[64];
static unsigned char D_00993FC0[1088];
static unsigned char D_00994400[1200];
static unsigned char D_009948B0[1040];
static unsigned char D_00994CC0[272];
static unsigned char D_00994DD0[48];
static unsigned char D_00994E00[640];
static unsigned char D_00995080[256];
static unsigned char D_00995180[64];
static unsigned char D_009951C0[64];
static unsigned char D_00995200[256];
static unsigned char D_00995300[4224];
static unsigned char D_00996380[512];
static unsigned char D_00996580[832];
extern int mapScanIndexCount;

/* Builds the scan subtree for the records of one box: records whose flag is at this depth and overlap the box are classed against the split plane. */
MapScanNode *mapBuildScanNode(MapRecordSet *set, MapScanNode *node, MapBoxCorner lo, MapBoxCorner hi, unsigned char depth)
{
    MapRecord *record = set->records;
    /* Initial value is dead, but its store is part of the original code: keep it. */
    unsigned short *cursor = (unsigned short *)node;
    float tolerance;
    int backCount = 0;
    int recordIndex;
    int corner;
    int frontCount = 0;
    int straddleCount = 0;
    unsigned char classBits;

    node->splitAxis = MAP_AXIS_X;
    node->splitValue = (hi.coord[MAP_AXIS_X] + lo.coord[MAP_AXIS_X]) * 0.5f;
    node->recordCount = 0;
    node->frontOffset = 0;
    node->backOffset = 0;
    tolerance = __builtin_fabsf((hi.coord[MAP_AXIS_X] - lo.coord[MAP_AXIS_X]) * MAP_SCAN_TOLERANCE_RATIO);
    if (hi.coord[MAP_AXIS_X] - lo.coord[MAP_AXIS_X] < hi.coord[MAP_AXIS_Z] - lo.coord[MAP_AXIS_Z]) {
        node->splitAxis = MAP_AXIS_Z;
        node->splitValue = (hi.coord[MAP_AXIS_Z] + lo.coord[MAP_AXIS_Z]) * 0.5f;
        tolerance = __builtin_fabsf((hi.coord[MAP_AXIS_Z] - lo.coord[MAP_AXIS_Z]) * MAP_SCAN_TOLERANCE_RATIO);
    }

    recordIndex = 0;
    for (; set->recordCount > recordIndex; recordIndex++, record++) {
        classBits = 0;
        if (MAP_RECORD_PLACED != mapRecordStates[recordIndex]) {
            Vector4 *vertex[3];
            vertex[0] = set->vertices + record->vertexIndex[0];
            vertex[1] = set->vertices + record->vertexIndex[1];
            vertex[2] = set->vertices + record->vertexIndex[2];
            if (!(vertex[0]->x < lo.coord[MAP_AXIS_X] && vertex[1]->x < lo.coord[MAP_AXIS_X] && vertex[2]->x < lo.coord[MAP_AXIS_X])
                && !(vertex[0]->z < lo.coord[MAP_AXIS_Z] && vertex[1]->z < lo.coord[MAP_AXIS_Z] && vertex[2]->z < lo.coord[MAP_AXIS_Z])
                && !(hi.coord[MAP_AXIS_X] < vertex[0]->x && hi.coord[MAP_AXIS_X] < vertex[1]->x && hi.coord[MAP_AXIS_X] < vertex[2]->x)
                && !(hi.coord[MAP_AXIS_Z] < vertex[0]->z && hi.coord[MAP_AXIS_Z] < vertex[1]->z && hi.coord[MAP_AXIS_Z] < vertex[2]->z)
                && (mapRecordStates[recordIndex] & MAP_DEPTH_MASK) == depth) {
                for (corner = 0; corner < 3; corner++) {
                    /* local view: coord typed float[4] (MapVertex), the shared Vector4 field form changed the code */
                    float value = ((MapVertex *)vertex[corner])->coord[node->splitAxis];
                    if (node->splitValue - tolerance < value) {
                        classBits |= MAP_CLASS_FRONT;
                    }
                    if (node->splitValue + tolerance >= value) {
                        classBits |= MAP_CLASS_BACK;
                    }
                }
                mapRecordStates[recordIndex] = classBits | (depth + 1);
                switch (classBits) {
                case 0:
                    break;
                case MAP_CLASS_FRONT:
                    frontCount++;
                    break;
                case MAP_CLASS_BACK:
                    backCount++;
                    break;
                case MAP_CLASS_STRADDLE:
                    straddleCount++;
                    break;
                }
            }
        }
    }

    cursor = (unsigned short *)(node + 1);
    if (straddleCount != 0 || depth == MAP_SCAN_MAX_DEPTH) {
        for (recordIndex = 0; recordIndex < set->recordCount; recordIndex++) {
            if (mapRecordStates[recordIndex] != MAP_RECORD_PLACED && (mapRecordStates[recordIndex] & MAP_DEPTH_MASK) == depth + 1) {
                unsigned char recordClass = mapRecordStates[recordIndex] & MAP_CLASS_MASK;
                if (depth < MAP_SCAN_MAX_DEPTH ? recordClass == MAP_CLASS_STRADDLE : recordClass != 0) {
                    *cursor = recordIndex;
                    cursor++;
                    node->recordCount = node->recordCount + 1;
                    mapRecordStates[recordIndex] = MAP_RECORD_PLACED;
                }
            }
        }
        {
            int indexTotal = mapScanIndexCount;
            mapScanIndexCount = indexTotal + node->recordCount;
        }
    }
    /* unsigned int alignment arithmetic: an unsigned long form changes the code. */
    if ((unsigned int)cursor & MAP_SCAN_ALIGN_MASK) {
        cursor = (unsigned short *)(((unsigned int)cursor + MAP_SCAN_ALIGN) & ~MAP_SCAN_ALIGN_MASK);
    }
    if (depth < MAP_SCAN_MAX_DEPTH) {
        if (0 != frontCount) {
            float saved;
            node->frontOffset = (unsigned char *)cursor - (unsigned char *)node;
            saved = lo.coord[node->splitAxis];
            lo.coord[node->splitAxis] = node->splitValue;
            cursor = (unsigned short *)mapBuildScanNode(set, (MapScanNode *)cursor, lo, hi, depth + 1);
            lo.coord[node->splitAxis] = saved;
        }
        if (backCount != 0) {
            node->backOffset = (unsigned char *)cursor - (unsigned char *)node;
            hi.coord[node->splitAxis] = node->splitValue;
            cursor = (unsigned short *)mapBuildScanNode(set, (MapScanNode *)cursor, lo, hi, depth + 1);
        }
    }
    return (MapScanNode *)cursor;
}

/* Tests the node's records, descends the front child on the near side and walks the back-child chain. */
void mapScanNode(MapRecordSet *set, MapScanNode *node)
{
    unsigned short *indices;
    MapRecord *record;
    float distance;
    int kind;
    int i;

    indices = (unsigned short *)(node + 1);
    if (node->recordCount != 0) {
        for (i = 0; i < node->recordCount; i++) {
            record = set->records + indices[i];
            distance = mapRecordDistance(set, record);
            if (distance != MAP_HEIGHT_NONE) {
                kind = record->kind;
                if (set->kindTable != 0) {
                    kind = set->kindTable[kind].kindCode;
                }
                if (kind != MAP_RECORD_KIND_NO_PROBE) {
                    mapProbeRecordHit(set, record, distance);
                }
            }
        }
    }
    if (node->frontOffset != 0 && node->splitValue <= (&set->origin.x)[node->splitAxis]) {
        mapScanNode(set, (MapScanNode *)((unsigned char *)node + node->frontOffset));
    }
    if (node->backOffset == 0) {
        return;
    }
    if (!((&set->origin.x)[node->splitAxis] < node->splitValue)) {
        return;
    }
    node = (MapScanNode *)((unsigned char *)node + node->backOffset);
    mapScanNode(set, node);
}

extern MapScanNode *mapScanPoolCursor;
extern MapRegisteredNode mapRegisteredSlots[MAP_SCAN_SLOT_COUNT];

/* Bounds the records of one set with VU0 min/max over their vertices, builds the scan tree on that box and registers it in a free slot. */
MapScanNode *mapRegisterNode(MapRecordSet *set)
{
    MapScanNode *root;
    MapRecord *record;
    int recordsLeft;
    int slot;

    record = set->records;
    mapBoundsMin.vector.z = 1000000.0f;
    mapBoundsMin.vector.y = 1000000.0f;
    mapBoundsMax.vector.z = -1000000.0f;
    mapBoundsMax.vector.y = -1000000.0f;
    mapBoundsMin.vector.x = 1000000.0f;
    mapBoundsMax.vector.x = -1000000.0f;
    root = mapScanPoolCursor;
    __asm__ __volatile__(
        "lqc2 vf4, 0(%0)\n\t"
        "lqc2 vf5, 0(%1)\n\t"
        :
        : "r" (&mapBoundsMin), "r" (&mapBoundsMax)
        : "memory");
    if (set->recordCount > 0) {
        recordsLeft = set->recordCount;
        do {
            __asm__ __volatile__(
                "lqc2 vf1, 0(%0)\n\t"
                "lqc2 vf2, 0(%1)\n\t"
                "lqc2 vf3, 0(%2)\n\t"
                "vmini.xyz vf4, vf4, vf1\n\t"
                "vmax.xyz vf5, vf5, vf1\n\t"
                "vmini.xyz vf4, vf4, vf2\n\t"
                "vmax.xyz vf5, vf5, vf2\n\t"
                "vmini.xyz vf4, vf4, vf3\n\t"
                "vmax.xyz vf5, vf5, vf3\n\t"
                :
                : "r" (set->vertices + record->vertexIndex[0]),
                  "r" (set->vertices + record->vertexIndex[1]),
                  "r" (set->vertices + record->vertexIndex[2]));
            record++;
        } while (--recordsLeft != 0);
    }
    __asm__ __volatile__(
        "sqc2 vf4, 0(%0)\n\t"
        "sqc2 vf5, 0(%1)\n\t"
        :
        : "r" (&mapBoundsMin), "r" (&mapBoundsMax)
        : "memory");
    memset(mapRecordStates, 0, set->recordCount);
    mapScanPoolCursor = mapBuildScanNode(set, mapScanPoolCursor, *(MapBoxCorner *) &mapBoundsMin,
                               *(MapBoxCorner *) &mapBoundsMax, 0);
    for (slot = 0; slot < MAP_SCAN_SLOT_COUNT; slot++) {
        if (mapRegisteredSlots[slot].records == 0) {
            mapRegisteredSlots[slot].root = root;
            mapRegisteredSlots[slot].records = set->records;
            break;
        }
    }
    return root;
}

/* Scans the registered tree of this record set, or returns 0 when the set is too large or unregistered. */
int mapScanRegisteredNode(MapRecordSet *set)
{
    int slot;

    mapScanIndexCount = 0;
    if (set->recordCount >= MAP_SCAN_MAX_RECORDS) {
        return 0;
    }
    for (slot = 0; slot < MAP_SCAN_SLOT_COUNT; slot++) {
        if (mapRegisteredSlots[slot].records == set->records) {
            mapScanNode(set, mapRegisteredSlots[slot].root);
            return 1;
        }
    }
    return 0;
}

typedef struct MapBlockSub {
    unsigned char unmodeled_00[2];
    unsigned char entryCount;
    unsigned char flags;
} MapBlockSub;

/* Entry header of one registered record set; its record array follows it. */
typedef struct MapBlockEntry {
    unsigned short recordCount;
    unsigned char unmodeled_02[2];
    unsigned int vertexOffset;
    unsigned char unmodeled_08[8];
} MapBlockEntry;

/* Clears the registry, starts the scan pool and registers every entry of the block's sub-blocks. */
void mapRegisterBlock(MapBlockHeader *block)
{
    MapRecordSet set;
    MapBlockSub *sub;
    MapBlockEntry *entry;
    MapRegisteredNode *slot;
    unsigned int *entryOffset;
    int subBlockIndex;
    int entryIndex;
    int clearIndex;

    slot = mapRegisteredSlots;
    for (clearIndex = MAP_SCAN_SLOT_COUNT - 1; clearIndex >= 0; clearIndex--) {
        slot->records = 0;
        slot->root = 0;
        slot++;
    }
    mapScanPoolCursor = D_0098ABF0;
    if (block == 0 || block->type != MAP_BLOCK_TYPE_RECORD_SETS || block->subBlockCount == 0) {
        return;
    }
    subBlockIndex = 0;
    do {
        /* Block offsets as unsigned int: a (unsigned char *) byte-pointer form changes the code. */
        sub = (MapBlockSub *)(((unsigned int *)(block + 1))[subBlockIndex] + (unsigned int)block);
        if (sub->flags == 0) {
            entryIndex = 0;
            if (sub->entryCount != 0) {
                entryOffset = (unsigned int *)(sub + 1);
                do {
                    entry = (MapBlockEntry *)(entryOffset[entryIndex] + (unsigned int)block);
                    set.recordCount = entry->recordCount;
                    set.vertices = (Vector4 *)(entry->vertexOffset + (unsigned int)block);
                    set.records = (MapRecord *)(entry + 1);
                    mapRegisterNode(&set);
                    entryIndex++;
                } while (entryIndex < sub->entryCount);
            }
        }
        subBlockIndex++;
    } while (subBlockIndex < block->subBlockCount);
}

/* Symbols named by the pointer words of the tables below (other TUs and the scaffold own them). */
extern char D_0068F9F8[];
extern char D_0068FA00[];
void func_00129350();
void func_00129318();
void func_00129548();
void func_001296B8();
void func_00129968();
void func_00128AD0();
void func_00129040();
extern char D_00690328[];
extern char D_00690320[];
extern char D_00690310[];
extern char D_00690308[];
extern char D_00690300[];
extern char D_006902F8[];
extern char D_00690348[];
extern char D_00690340[];
extern char D_00690338[];

/* Registry slots, keep-off flags and the scan pool and small globals of this TU (.data and .sdata). */
MapRegisteredNode mapRegisteredSlots[MAP_SCAN_SLOT_COUNT] = { 0 };
int elvKeepOffFlags[36] = {
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x49497350U, 0x6B62696CU, 0x6C6E7265U, 0x30313732U
};
unsigned int D_00646F30[] = { 0 };
unsigned int D_00646F38[32] = {
    0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
    0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
    0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU,
    0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU
};
unsigned int D_00646FB8[10] = {
    0x00000000U, 0x00000000U, 0x00000000U, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, (unsigned int) D_0068F9F8, 0xFFFFFFFFU,
    0xFFFFFFFFU, (unsigned int) D_0068FA00
};
unsigned int D_00646FE0[8] = {
    0x00000000U, 0x00000000U, 0x00000083U, (unsigned int) func_00129350, 0x0000005AU, (unsigned int) func_00129318, 0x00000000U, 0x00000000U
};
unsigned int D_00647000[490] = {
    0x27BDFFE0U, 0x24050026U, 0xFFB00000U, 0x0080802DU, 0xFFBF0010U, 0x3C048007U, 0x0C01D07AU, 0x24844700U,
    0x3C028007U, 0x3C05FFFFU, 0x3C0603FFU, 0x3C070C00U, 0x24434780U, 0x0000202DU, 0x34A5C402U, 0x34C6FFFFU,
    0x8C620000U, 0x56020007U, 0x24840001U, 0x16050009U, 0x8C620004U, 0x00021082U, 0x00461024U, 0x10000005U,
    0x00471025U, 0x2C820005U, 0x1440FFF5U, 0x24630008U, 0x0000102DU, 0xDFBF0010U, 0xDFB00000U, 0x03E00008U,
    0x27BD0020U, 0x00000000U, 0x3C058007U, 0x8C830000U, 0x8CA247A8U, 0x2406FFFEU, 0x00661824U, 0x2407FFF9U,
    0x30420001U, 0x2408FFF7U, 0x00621825U, 0x2409FFEFU, 0xAC830000U, 0x240AE01FU, 0x00671824U, 0x3C06FFFFU,
    0x8CA247A8U, 0x34C61FFFU, 0x24A747A8U, 0x30420006U, 0x00621825U, 0xAC830000U, 0x00681824U, 0x8CA247A8U,
    0x30420008U, 0x00621825U, 0xAC830000U, 0x00691824U, 0x8CA247A8U, 0x30420010U, 0x00621825U, 0xAC830000U,
    0x006A1824U, 0x8CA247A8U, 0x30421FE0U, 0x00621825U, 0xAC830000U, 0x00661824U, 0x8CA247A8U, 0x3042E000U,
    0x00621825U, 0xAC830000U, 0x94E20002U, 0x03E00008U, 0xA4820002U, 0x00000000U, 0x3C058007U, 0x8C820000U,
    0x8CA347A8U, 0x2406FFFEU, 0x30420001U, 0x2407FFF9U, 0x00661824U, 0x2408FFF7U, 0x00621825U, 0x2409FFEFU,
    0xACA347A8U, 0x240AE01FU, 0x00671824U, 0x3C06FFFFU, 0x8C820000U, 0x34C61FFFU, 0x24A747A8U, 0x30420006U,
    0x00621825U, 0xACA347A8U, 0x00681824U, 0x8C820000U, 0x30420008U, 0x00621825U, 0xACA347A8U, 0x00691824U,
    0x8C820000U, 0x30420010U, 0x00621825U, 0xACA347A8U, 0x006A1824U, 0x8C820000U, 0x30421FE0U, 0x00621825U,
    0xACA347A8U, 0x00661824U, 0x8C820000U, 0x3042E000U, 0x00621825U, 0xACA347A8U, 0x94820002U, 0x03E00008U,
    0xA4E20002U, 0x00000000U, 0x3C06BC00U, 0x8CC603C0U, 0x10C00011U, 0x3C088007U, 0x3C02BC00U, 0x00C23021U,
    0x25074700U, 0x24C6000FU, 0x0000282DU, 0x00000000U, 0x00C51021U, 0x00E52021U, 0x90430000U, 0x24A50001U,
    0x28A20026U, 0xA0830000U, 0x1440FFF9U, 0x00000000U, 0x10000002U, 0xDD034700U, 0xDD034700U, 0x000316B8U,
    0x0002103FU, 0x30420007U, 0x14400015U, 0x00000000U, 0x2402FEFFU, 0x00021438U, 0x3442FFFFU, 0x00021438U,
    0x3442FFFFU, 0x2404F3FFU, 0x00042438U, 0x3484FFFFU, 0x00042438U, 0x3484FFFFU, 0x00621024U, 0x3C03FFFFU,
    0x34630FFFU, 0x00031C38U, 0x3463FFFFU, 0x00031C38U, 0x3463FFFFU, 0x00441024U, 0x00431024U, 0xFD024700U,
    0x03E00008U, 0x00000000U, 0x00A63821U, 0x2CE20081U, 0x14400008U, 0x0080502DU, 0x2CC20080U, 0x10400003U,
    0x24020080U, 0x10000003U, 0x00462823U, 0x24060080U, 0x0000282DU, 0x00A62821U, 0x00C5102BU, 0x1040000FU,
    0x0000402DU, 0x00A0382DU, 0x3C098007U, 0x3C058007U, 0x24A247B0U, 0x01482021U, 0x00C21021U, 0x25080001U,
    0x90430000U, 0x24C60001U, 0x00C7102BU, 0x1440FFF8U, 0xA0830000U, 0x10000003U, 0xDD234700U, 0x3C098007U,
    0xDD234700U, 0x000316B8U, 0x0002103FU, 0x30420007U, 0x14400003U, 0x00000000U, 0x03E00008U, 0x0000102DU,
    0x0003133EU, 0x03E00008U, 0x3042000FU, 0x00000000U, 0x00A0182DU, 0x00662821U, 0x2CA20081U, 0x14400009U,
    0x0080482DU, 0x2CC20080U, 0x10400003U, 0x24020080U, 0x10000003U, 0x00461823U, 0x24060080U, 0x0000182DU,
    0x00662821U, 0x00C5102BU, 0x1040000CU, 0x0000382DU, 0x3C088007U, 0x00000000U, 0x01271021U, 0x250347B0U,
    0x00C31821U, 0x90440000U, 0x24C60001U, 0x24E70001U, 0x00C5102BU, 0x1440FFF8U, 0xA0640000U, 0x03E00008U,
    0x00000000U, 0x00000000U, 0x27BDFFF0U, 0x3C021000U, 0xFFBF0000U, 0x24030004U, 0x3442F000U, 0x3C041000U,
    0xAC430000U, 0x3484F000U, 0x8C820000U, 0x30420004U, 0x00000000U, 0x00000000U, 0x00000000U, 0x1040FFFAU,
    0x00000000U, 0x3C021000U, 0x24040004U, 0x3442F000U, 0x3C038007U, 0xAC440000U, 0x8C624760U, 0x0040F809U,
    0x00000000U, 0x3C028007U, 0x24050002U, 0x8C434764U, 0x24060001U, 0x0060F809U, 0x24040001U, 0x3C028007U,
    0x8C43474CU, 0x0060F809U, 0x3404DFFDU, 0x3C028007U, 0x8C434750U, 0x0060F809U, 0x00000000U, 0x3C028007U,
    0x8C43475CU, 0x0060F809U, 0x2404007FU, 0x3C028007U, 0x8C434754U, 0x0060F809U, 0x00000000U, 0x3C028007U,
    0x8C434758U, 0x0060F809U, 0x00000000U, 0xDFBF0000U, 0x03E00008U, 0x27BD0010U, 0x27BDFF50U, 0x3C028007U,
    0xFFBE0090U, 0x3C038007U, 0xFFB70080U, 0x241E0010U, 0xFFB60070U, 0x3C178007U, 0xFFB50060U, 0x3C168007U,
    0xFFB40050U, 0x0080A82DU, 0xFFB20030U, 0x00C0A02DU, 0xFFB10020U, 0x3C128007U, 0xFFB00010U, 0x2411004CU,
    0x8C484728U, 0x24100001U, 0xFFBF00A0U, 0xAFA50000U, 0xFFB30040U, 0x8D130000U, 0x8C624730U, 0xAFA70004U,
    0x0040F809U, 0x0260202DU, 0x3C038007U, 0x0260202DU, 0x8C624734U, 0x0040F809U, 0x0000282DU, 0x00000000U,
    0x8EC24748U, 0x02221021U, 0x8C420008U, 0x50400010U, 0x26100001U, 0x5213000EU, 0x26100001U, 0x145E0006U,
    0x8EE24744U, 0x8E424740U, 0x0040F809U, 0x0200202DU, 0x10000007U, 0x26100001U, 0x0040F809U, 0x0200202DU,
    0x8E424740U, 0x0040F809U, 0x0200202DU, 0x26100001U, 0x2A020100U, 0x1440FFEAU, 0x2631004CU, 0x3C038007U,
    0x8C62473CU, 0x0040F809U, 0x0000802DU, 0x3C038007U, 0x8C624738U, 0x0040F809U, 0x00000000U, 0x3C038007U,
    0x8C62472CU, 0x0C01D0F2U, 0xAC400000U, 0x3C028007U, 0x1A80000DU, 0x8C444778U, 0x3C118007U, 0x8FA50004U,
    0x00101880U, 0x8E224770U, 0x26100001U, 0x00651821U, 0x0040F809U, 0x8C650000U, 0x0040202DU, 0x0214102AU,
    0x1440FFF7U, 0x8FA50004U, 0x2402004CU, 0x3C038007U, 0x02621018U, 0x8EC54748U, 0x8C644778U, 0x3C038007U,
    0x8C66476CU, 0x00451021U, 0xAC440038U, 0x8FA50000U, 0xAC540034U, 0xAC55000CU, 0xAC550030U, 0xAC450014U,
    0xA440001AU, 0xA4400018U, 0xAC400024U, 0xAC40001CU, 0x00C0F809U, 0xAC400020U, 0x3C028007U, 0x8C434768U,
    0x0060F809U, 0x00000000U, 0x02A0102DU, 0xDFBF00A0U, 0xDFBE0090U, 0xDFB70080U, 0xDFB60070U, 0xDFB50060U,
    0xDFB40050U, 0xDFB30040U, 0xDFB20030U, 0xDFB10020U, 0xDFB00010U, 0x03E00008U, 0x27BD00B0U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000040U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x800125ECU, 0x800125F4U, 0x80004970U, 0x80004288U, 0x800021B0U, 0x80004E68U,
    0x80003F00U, 0x80003E00U, 0x80017400U, 0x8000B8D0U, 0x8000B900U, 0x8000B7A8U, 0x8000B840U, 0x8000AD68U,
    0x8000AA60U, 0x8000A060U, 0x80002A80U, 0x80002AC0U, 0x80005560U, 0x80012600U, 0x80012608U, 0x00000000U,
    0x0000004AU, 0x80074138U, 0x0000004BU, 0x80074088U, 0x0000006EU, 0x80074350U, 0x0000006FU, 0x800742A8U,
    0xFFFFC402U, 0x80074498U
};
unsigned int D_006477A8[6] = {
    0x0000005AU, (unsigned int) func_00129548, 0x0000005BU, 0x80074000U, 0xFFFFC402U, 0x00000000U
};
unsigned int D_006477C0[] = {
    0x0000005AU, (unsigned int) func_001296B8
};
unsigned int D_006477C8[464] = {
    0x3C028007U, 0x0000282DU, 0x24436710U, 0x00000000U, 0x8C620000U, 0x14820003U, 0x24A50001U, 0x03E00008U,
    0x8C620004U, 0x2CA20006U, 0x1440FFF9U, 0x24630008U, 0x03E00008U, 0x0000102DU, 0x00A4202AU, 0x10800003U,
    0x3C020001U, 0x03E00008U, 0x00A21025U, 0x03E00008U, 0x00A0102DU, 0x00000000U, 0x27BDFF80U, 0xFFB30030U,
    0x3C138007U, 0xFFB60060U, 0xFFB50050U, 0x0080B02DU, 0xFFB00000U, 0x00A0A82DU, 0x8E626700U, 0x0000802DU,
    0xFFBF0070U, 0xFFB40040U, 0xFFB20020U, 0x18400028U, 0xFFB10010U, 0x3C148007U, 0x24120014U, 0x00000000U,
    0x26916740U, 0x02121018U, 0x02C0202DU, 0x00511021U, 0x0C01D80EU, 0x94450000U, 0x02A2102AU, 0x10400018U,
    0x8E626700U, 0x2444FFFFU, 0x0090182AU, 0x14600018U, 0x00921018U, 0x00511821U, 0x68650007U, 0x6C650000U,
    0x6866000FU, 0x6C660008U, 0x8C670010U, 0xB065001BU, 0xB4650014U, 0xB0660023U, 0xB466001CU, 0xAC670024U,
    0x2484FFFFU, 0x2463FFECU, 0x0090102AU, 0x00000000U, 0x1040FFF1U, 0x00000000U, 0x10000006U, 0x0200102DU,
    0x26100001U, 0x0202102AU, 0x1440FFDDU, 0x24120014U, 0x0200102DU, 0xDFBF0070U, 0xDFB60060U, 0xDFB50050U,
    0xDFB40040U, 0xDFB30030U, 0xDFB20020U, 0xDFB10010U, 0xDFB00000U, 0x03E00008U, 0x27BD0080U, 0x00000000U,
    0x27BDFF70U, 0x3C02B000U, 0xFFB70070U, 0x34421800U, 0xFFB60060U, 0x00C0B82DU, 0xFFB50050U, 0x00A0B02DU,
    0xFFB30030U, 0x3C158007U, 0xFFBF0080U, 0x3093FFFFU, 0xFFB20020U, 0xFFB10010U, 0xFFB00000U, 0xFFB40040U,
    0x8EA36700U, 0x8C540000U, 0x28630040U, 0x14600008U, 0x02749821U, 0x1000002EU, 0x2402FFFFU, 0x0060902DU,
    0x00621014U, 0x00821025U, 0x1000000DU, 0xFCA26708U, 0x3C058007U, 0x0000182DU, 0xDCA46708U, 0x00641016U,
    0x30420001U, 0x1040FFF5U, 0x24020001U, 0x24630001U, 0x28620040U, 0x1440FFFAU, 0x00641016U, 0x2412FFFFU,
    0x0640001BU, 0x0240102DU, 0x0380882DU, 0x0260282DU, 0x0C01D816U, 0x0280202DU, 0x24040014U, 0x3C088007U,
    0x00441018U, 0x25036740U, 0x8EA56700U, 0x24700004U, 0x24A50001U, 0x00432021U, 0x00623821U, 0x00508021U,
    0xA4940002U, 0xA4930000U, 0x00E0302DU, 0xAE120000U, 0x00C0182DU, 0xACF10010U, 0x95046740U, 0xACD60008U,
    0xAC77000CU, 0x0C01D918U, 0xAEA56700U, 0x8E020000U, 0xDFBF0080U, 0xDFB70070U, 0xDFB60060U, 0xDFB50050U,
    0xDFB40040U, 0xDFB30030U, 0xDFB20020U, 0xDFB10010U, 0xDFB00000U, 0x03E00008U, 0x27BD0090U, 0x00000000U,
    0x27BDFFD0U, 0x3C0C8007U, 0xFFB10010U, 0x0080682DU, 0x8D826700U, 0x0180882DU, 0xFFBF0020U, 0x2406FFFFU,
    0x18400058U, 0xFFB00000U, 0x18400056U, 0x0000402DU, 0x3C0B8007U, 0x24030014U, 0x25656740U, 0x01032018U,
    0x00A41021U, 0x8C430004U, 0x15A3004AU, 0x8D826700U, 0x3C03B000U, 0x00852021U, 0x34631820U, 0x94850000U,
    0x8C620000U, 0x14A20008U, 0x24030014U, 0x3C021000U, 0x3442F000U, 0x8C430000U, 0x30631000U, 0x14600043U,
    0x2402FFFFU, 0x24030014U, 0x8D896700U, 0x01031818U, 0x25646740U, 0x2522FFFFU, 0x0100382DU, 0x0102102AU,
    0x00641821U, 0x10400019U, 0x94700002U, 0x3C0A8007U, 0x24E30001U, 0x24050014U, 0x00651018U, 0x00E52018U,
    0x25666740U, 0x0060382DU, 0x00462821U, 0x00862021U, 0x2522FFFFU, 0x68A30007U, 0x6CA30000U, 0x68A6000FU,
    0x6CA60008U, 0x8CAE0010U, 0xB0830007U, 0xB4830000U, 0xB086000FU, 0xB4860008U, 0x00E2102AU, 0x1440FFECU,
    0xAC8E0010U, 0x10000003U, 0x24020001U, 0x3C0A8007U, 0x24020001U, 0x8D846700U, 0xDD436708U, 0x01A21014U,
    0x00021027U, 0x2484FFFFU, 0x00621824U, 0xAD846700U, 0x15000003U, 0xFD436708U, 0x0C01D918U, 0x95646740U,
    0x8E226700U, 0x14400004U, 0x24030083U, 0x3C02B000U, 0x34421810U, 0xAC430000U, 0x3C02B000U, 0x0200202DU,
    0x34421800U, 0x0C01D80EU, 0x8C450000U, 0x10000005U, 0x00503023U, 0x25080001U, 0x0102102AU, 0x1440FFAEU,
    0x24030014U, 0x0000000FU, 0x00C0102DU, 0xDFBF0020U, 0xDFB10010U, 0xDFB00000U, 0x03E00008U, 0x27BD0030U,
    0x27BDFFF0U, 0xFFBF0000U, 0x0C01D858U, 0x3084FFFFU, 0x0000000FU, 0xDFBF0000U, 0x03E00008U, 0x27BD0010U,
    0x3C02B000U, 0x34421820U, 0xAC440000U, 0x0000000FU, 0x3C02B000U, 0x24030583U, 0x34421810U, 0x03E00008U,
    0xAC430000U, 0x00000000U, 0x27BDFF50U, 0x0000402DU, 0xFFBF00A0U, 0xFFB70090U, 0xFFB60080U, 0xFFB50070U,
    0xFFB40060U, 0xFFB30050U, 0xFFB20040U, 0xFFB10030U, 0xFFB00020U, 0x3C118007U, 0x3C128007U, 0x00000000U,
    0x8E226700U, 0x0102102AU, 0x1040000AU, 0x24030014U, 0x26446740U, 0x01031818U, 0x96456740U, 0x00641821U,
    0x94620000U, 0x10A2FFF6U, 0x25080001U, 0x0C01D918U, 0x0040202DU, 0x3C028007U, 0x0220B02DU, 0x24546740U,
    0x24130014U, 0x3C158007U, 0x10000004U, 0x24170001U, 0x96426740U, 0x1462003EU, 0x8E226700U, 0x8EC26700U,
    0x0000402DU, 0x26466740U, 0x68C30007U, 0x6CC30000U, 0x68C4000FU, 0x6CC40008U, 0x8CC50010U, 0xB3A30007U,
    0xB7A30000U, 0xB3A4000FU, 0xB7A40008U, 0xAFA50010U, 0x2442FFFFU, 0x1840001AU, 0xAEC26700U, 0x8E296700U,
    0x8FAA0010U, 0x8FA60004U, 0x97A70000U, 0x00000000U, 0x01131818U, 0x25020001U, 0x0040402DU, 0x00742821U,
    0x00531818U, 0x00742021U, 0x688B0007U, 0x6C8B0000U, 0x688C000FU, 0x6C8C0008U, 0x8C8D0010U, 0xB0AB0007U,
    0xB4AB0000U, 0xB0AC000FU, 0xB4AC0008U, 0x0109182AU, 0x1460FFEFU, 0xACAD0010U, 0x10000004U, 0x00000000U,
    0x8FAA0010U, 0x8FA60004U, 0x97A70000U, 0x0380802DU, 0x0140E02DU, 0xDEA36708U, 0x00D71014U, 0x00021027U,
    0x3C040008U, 0x00621824U, 0x8FA50008U, 0x8FA8000CU, 0x34842000U, 0x0C01D9A0U, 0xFEA36708U, 0x0200E02DU,
    0x8E226700U, 0x1C40FFC2U, 0x97A30000U, 0x8E226700U, 0x18400005U, 0x24030483U, 0x0C01D918U, 0x96446740U,
    0x10000004U, 0x00000000U, 0x3C02B000U, 0x34421810U, 0xAC430000U, 0x0000000FU, 0x42000038U, 0xDFBF00A0U,
    0xDFB70090U, 0xDFB60080U, 0xDFB50070U, 0xDFB40060U, 0xDFB30050U, 0xDFB20040U, 0xDFB10030U, 0xDFB00020U,
    0x03E00008U, 0x27BD00B0U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x3C1A8007U, 0xAF5F6C40U, 0x3C1A8007U, 0xAF5D6C50U, 0x40847000U, 0x0000040FU, 0x00A0182DU, 0x00C0202DU,
    0x00E0282DU, 0x0100302DU, 0x401A6000U, 0x375A0012U, 0x409A6000U, 0x0000040FU, 0x42000018U, 0x00000000U,
    0x40016000U, 0x241AFFE4U, 0x003A0824U, 0x40816000U, 0x0000040FU, 0x3C1A8007U, 0x8F5F6C40U, 0x3C1A8007U,
    0x03E00008U, 0x8F5D6C50U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x000000FCU, 0x80076440U, 0x000000FEU, 0x80076440U,
    0x000000FDU, 0x800762A0U, 0x000000FFU, 0x800762A0U, 0x0000012CU, 0x80076488U, 0x00000008U, 0x800766C0U
};
unsigned int D_00647F08[10] = {
    0x3C1D0008U, 0x0060F809U, 0x27BD1FC0U, 0x2403FFF8U, 0x0000000CU, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U
};
unsigned int D_00647F30[16] = {
    0x0000005AU, (unsigned int) func_00129968, 0x0000005BU, 0x80076000U, 0x000000FCU, 0x00000000U, 0x000000FEU, 0x00000000U,
    0x000000FDU, 0x00000000U, 0x000000FFU, 0x00000000U, 0x0000012CU, 0x00000000U, 0x00000008U, 0x00000000U
};
unsigned int D_00647F70[6] = {
    0x00000000U, 0x009D5D70U, 0x00000000U, 0x00000000U, 0x00121D68U, 0x00000000U
};
unsigned int D_00647F88[204] = {
    0x3C028007U, 0x0000282DU, 0x24435300U, 0x00000000U, 0x8C620000U, 0x14820003U, 0x24A50001U, 0x03E00008U,
    0x8C620004U, 0x2CA20006U, 0x1440FFF9U, 0x24630008U, 0x03E00008U, 0x0000102DU, 0x00051602U, 0x304300F0U,
    0x24020030U, 0x10620014U, 0x2C620031U, 0x10400009U, 0x24020010U, 0x1062000EU, 0x2C620011U, 0x1040000AU,
    0x24020020U, 0x1060000CU, 0x2402FFFFU, 0x10000014U, 0x00000000U, 0x24020050U, 0x10620005U, 0x2C620051U,
    0x10400003U, 0x24020040U, 0x10620003U, 0x00000000U, 0x03E00008U, 0x2402FFFFU, 0x40842800U, 0x40855000U,
    0x40861000U, 0x40871800U, 0x0000040FU, 0x42000006U, 0x0000040FU, 0x42000008U, 0x0000040FU, 0x40020000U,
    0x03E00008U, 0x00000000U, 0x2C820030U, 0x1040000BU, 0x2402FFFFU, 0x40840000U, 0x40852800U, 0x40865000U,
    0x40871000U, 0x40881800U, 0x0000040FU, 0x42000002U, 0x0000040FU, 0x03E00008U, 0x0080102DU, 0x03E00008U,
    0x00000000U, 0x00000000U, 0x2C820030U, 0x14400003U, 0x00000000U, 0x03E00008U, 0x2402FFFFU, 0x40840000U,
    0x0000040FU, 0x42000001U, 0x0000040FU, 0x40022800U, 0xACA20000U, 0x40035000U, 0xACC30000U, 0x40021000U,
    0xACE20000U, 0x40031800U, 0xAD030000U, 0x03E00008U, 0x0080102DU, 0x00000000U, 0x40845000U, 0x0000040FU,
    0x42000008U, 0x0000040FU, 0x40040000U, 0x04810003U, 0x00000000U, 0x10000009U, 0x2404FFFFU, 0x42000001U,
    0x0000040FU, 0x40022800U, 0xACA20000U, 0x40031000U, 0xACC30000U, 0x40021800U, 0xACE20000U, 0x03E00008U,
    0x0080102DU, 0x00000000U, 0x27BDFFD0U, 0xFFB00010U, 0x0080802DU, 0x32020FFFU, 0x14400007U, 0xFFBF0020U,
    0x3C02000FU, 0x2603FFFFU, 0x3442FFFEU, 0x0043102BU, 0x14400003U, 0x3C047000U, 0x1000003BU, 0x2402FFFFU,
    0x03A0282DU, 0x34844000U, 0x37A60004U, 0x0C01D456U, 0x37A70008U, 0x0040282DU, 0x04A10009U, 0x00000000U,
    0x12000031U, 0x0000102DU, 0x40053000U, 0x24A20001U, 0x40823000U, 0x0000040FU, 0x10000013U, 0x00000000U,
    0x16000011U, 0x24A2FFFFU, 0x3C03E001U, 0x00021340U, 0x00433021U, 0x40023000U, 0x2442FFFFU, 0x40823000U,
    0x40850000U, 0x40802800U, 0x40865000U, 0x40801000U, 0x40801800U, 0x0000040FU, 0x42000002U, 0x0000040FU,
    0x10000019U, 0x0000102DU, 0x3C02FFFFU, 0x26041000U, 0x3442F000U, 0x3C067000U, 0x00822024U, 0xAFA00000U,
    0x02021024U, 0x00042182U, 0x00021182U, 0x3484001FU, 0x3442001FU, 0x34C64000U, 0xAFA20004U, 0xAFA40008U,
    0x40850000U, 0x0000182DU, 0x40832800U, 0x40865000U, 0x40821000U, 0x40841800U, 0x0000040FU, 0x42000002U,
    0x0000040FU, 0x00A0102DU, 0xDFBF0020U, 0xDFB00010U, 0x03E00008U, 0x27BD0030U, 0x00000000U, 0x00000000U,
    0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U,
    0x00000055U, 0x80075038U, 0x00000056U, 0x800750C8U, 0x00000057U, 0x80075108U, 0x00000058U, 0x80075158U,
    0x00000059U, 0x800751A8U, 0x00000003U, 0x80075330U
};
unsigned int D_006482B8[] = { 0 };
unsigned int D_006482C0[16] = { 0 };
unsigned int D_00648300[] = { 0 };
unsigned int D_00648308[16] = {
    0x0000005AU, (unsigned int) func_00128AD0, 0x0000005BU, 0x80075000U, 0x00000054U, (unsigned int) func_00129040, 0x00000055U, 0x00000000U,
    0x00000056U, 0x00000000U, 0x00000057U, 0x00000000U, 0x00000058U, 0x00000000U, 0x00000059U, 0x00000000U
};
unsigned int D_00648348[52] = {
    0x00000000U, 0x70000000U, 0x80000007U, 0x00000007U, 0x00006000U, 0xFFFF8000U, 0x00001E1FU, 0x00001F1FU,
    0x00000000U, 0x10000000U, 0x00400017U, 0x00400053U, 0x00000000U, 0x10002000U, 0x00400097U, 0x004000D7U,
    0x00000000U, 0x10004000U, 0x00400117U, 0x00400157U, 0x00000000U, 0x10006000U, 0x00400197U, 0x004001D7U,
    0x00000000U, 0x10008000U, 0x00400217U, 0x00400257U, 0x00000000U, 0x1000A000U, 0x00400297U, 0x004002D7U,
    0x00000000U, 0x1000C000U, 0x00400313U, 0x00400357U, 0x00000000U, 0x1000E000U, 0x00400397U, 0x004003D7U,
    0x0001E000U, 0x11000000U, 0x00440017U, 0x00440415U, 0x0001E000U, 0x12000000U, 0x00480017U, 0x00480415U,
    0x01FFE000U, 0x1E000000U, 0x00780017U, 0x007C0017U
};
unsigned int D_00648418[72] = {
    0x0007E000U, 0x00080000U, 0x0000201FU, 0x0000301FU, 0x0007E000U, 0x00100000U, 0x0000401FU, 0x0000501FU,
    0x0007E000U, 0x00180000U, 0x0000601FU, 0x0000701FU, 0x001FE000U, 0x00200000U, 0x0000801FU, 0x0000C01FU,
    0x001FE000U, 0x00400000U, 0x0001001FU, 0x0001401FU, 0x001FE000U, 0x00600000U, 0x0001801FU, 0x0001C01FU,
    0x007FE000U, 0x00800000U, 0x0002001FU, 0x0003001FU, 0x007FE000U, 0x01000000U, 0x0004001FU, 0x0005001FU,
    0x007FE000U, 0x01800000U, 0x0006001FU, 0x0007001FU, 0x0007E000U, 0x20080000U, 0x00002017U, 0x00003017U,
    0x0007E000U, 0x20100000U, 0x00004017U, 0x00005017U, 0x0007E000U, 0x20180000U, 0x00006017U, 0x00007017U,
    0x001FE000U, 0x20200000U, 0x00008017U, 0x0000C017U, 0x001FE000U, 0x20400000U, 0x00010017U, 0x00014017U,
    0x001FE000U, 0x20600000U, 0x00018017U, 0x0001C017U, 0x007FE000U, 0x20800000U, 0x00020017U, 0x00030017U,
    0x007FE000U, 0x21000000U, 0x00040017U, 0x00050017U, 0x007FE000U, 0x21800000U, 0x00060017U, 0x00070017U
};
unsigned int D_00648538[32] = {
    0x0007E000U, 0x30100000U, 0x0000403FU, 0x0000503FU, 0x0007E000U, 0x30180000U, 0x0000603FU, 0x0000703FU,
    0x001FE000U, 0x30200000U, 0x0000803FU, 0x0000C03FU, 0x001FE000U, 0x30400000U, 0x0001003FU, 0x0001403FU,
    0x001FE000U, 0x30600000U, 0x0001803FU, 0x0001C03FU, 0x007FE000U, 0x30800000U, 0x0002003FU, 0x0003003FU,
    0x007FE000U, 0x31000000U, 0x0004003FU, 0x0005003FU, 0x007FE000U, 0x31800000U, 0x0006003FU, 0x0007003FU
};
unsigned int D_006485B8[14] = {
    0x0000000DU, 0x00000012U, 0x00000008U, 0x00000000U, (unsigned int) D_00648348, (unsigned int) D_00648418, (unsigned int) D_00648538, 0x00000000U,
    0x00000000U, 0x00000000U, 0x49497350U, 0x6762696CU, 0x68706172U, 0x30303732U
};
unsigned int D_006485F0[4] = {
    0x00020001U, 0x00030001U, 0x00000000U, 0x00000000U
};
unsigned int D_00648600[8] = {
    0x01000404U, 0x20000000U, 0x00000000U, 0x05000000U, 0x06000000U, 0x03000000U, 0x02000000U, 0x04000000U
};
unsigned int D_00648620[12] = {
    0x06000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x49497350U, 0x7062696CU, 0x2020746BU, 0x30303732U,
    0x49497350U, 0x7062696CU, 0x20206461U, 0x30313732U
};
unsigned int D_00648650[] = {
    0x00000000U, 0x00000001U
};
unsigned int D_00648658[8] = {
    (unsigned int) D_00690328, (unsigned int) D_00690320, (unsigned int) D_00690310, (unsigned int) D_00690320, (unsigned int) D_00690320, (unsigned int) D_00690308, (unsigned int) D_00690300, (unsigned int) D_006902F8
};
unsigned int D_00648678[4] = {
    (unsigned int) D_00690348, (unsigned int) D_00690340, (unsigned int) D_00690338, 0x00000000U
};
int mapScanIndexCount = 0;
MapScanNode *mapScanPoolCursor = D_0098ABF0;
unsigned char elvActiveMask = 0;
unsigned char elvNoOutMask[7] = { 0 };
