#include "common.h"
#include "shared.h"
#include "main/sv_get_image_item_158.h"
#include "main/sv_get_image_item_159.h"
#define SV_ENTRY_COUNT 0x100

/* SvTypeListEntry.resourceKind, equal to the chunk kind it was built from. */
enum {
    SV_RES_CLUT = 3,
    SV_RES_IMAGE = 4,
    SV_RES_SCRIPT = 5,
    SV_RES_MODEL = 6
};

/*
 * The image-mapper table has a 0x1C-byte header followed by 0x100 entries
 * of 0x24 bytes. These entry fields are evidenced by the lookup routines:
 * the active word is at +0x08, the type byte at +0x0E, and the resource
 * name pointer at +0x20.
 */
typedef struct SvTypeListEntry {
    const void *data;
    unsigned char unmodeled_04[4];
    int used;
    short resourceKind;
    union {
        unsigned short typeFlags; /* +0x0e */
        struct { unsigned char type; unsigned char unmodeled_0f; };
    };
    unsigned char unmodeled_10[2];
    unsigned short basePointer;
    unsigned short bitsPerPixel;
    unsigned short bufferWidthPixels;
    unsigned short width;
    unsigned short height;
    unsigned short textureWidthExponent;
    unsigned short textureHeightExponent;
    const char *name;
} SvTypeListEntry;

typedef struct SvTypeList {
    int total;
    int size;
    int capacity;
    unsigned int savedSize;
    int dimensions;                 /* +0x10: width and height halfwords, cleared as one word */
    short entryCount;               /* +0x14 */
    short tag;                      /* +0x16: the fourth svAddImageMapper argument */
    short pendingCount;             /* +0x18: flushed by svImageListDestroy when nonzero */
    unsigned char unmodeled_1A[2];
    SvTypeListEntry entries[SV_ENTRY_COUNT];
} SvTypeList;

/*
 * One svAddScript2 sub-record, at +0x1C of a 0x24-byte slot of the type
 * list's item array. Only the fields this function itself writes are
 * modeled, at their byte offset from this sub-record's own base: `codePtr`
 * (+0x00, the script's own +0xC0 address), `active` (+0x08, always 1),
 * `kind` (+0x0C, always 5, matching this function's own "script" name),
 * `id` (+0x0E, the script's own +0xA halfword), two cleared halfwords
 * (+0x18/+0x1A) and `dataPtr` (+0x20, the script's own +0x14 address).
 * +0x20 (slot +0x3C) lands past the slot's own 0x24-byte stride, into the
 * next slot's leading bytes -- evidenced by the original bytes, not
 * treated as a bug.
 */
typedef struct SvScriptSlot {
    void *codePtr;                  /* 0x00 */
    unsigned char unmodeled_04[4];
    int active;                      /* 0x08 */
    short kind;                       /* 0x0C */
    short id;                          /* 0x0E */
    unsigned char unmodeled_10[8];
    short clearedA;                     /* 0x18 */
    short clearedB;                      /* 0x1A */
    unsigned char unmodeled_1C[4];
    void *dataPtr;                        /* 0x20 */
} SvScriptSlot;

typedef struct SvResourceChunk {
    unsigned int byteSize;
    unsigned char unmodeled_04[4];
    short kind;
    unsigned short typeFlags;
    char payload[0x34];
} SvResourceChunk;

int svAddModel(SvTypeList *typeList, SvResourceChunk *chunk);
int svAddScript(SvTypeList *typeList, SvResourceChunk *chunk);

int svGetSizeBit(unsigned int value)
{
    unsigned int mask;
    int bitIndex;

    mask = 2;
    bitIndex = 1;
    while (bitIndex < 32) {
        if ((value & mask) != 0) {
            return bitIndex;
        }
        mask <<= 1;
        bitIndex++;
    }
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_159", svAddImage);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_159", svAddClut);

int svAddModel(SvTypeList *typeList, SvResourceChunk *chunk)
{
    SvTypeListEntry *entry;
    unsigned short typeFlags;
    int index;

    index = svImageListAlloc((int)typeList);
    if (index < 0) {
        return -1;
    }
    entry = &typeList->entries[index];
    entry->resourceKind = SV_RES_MODEL;
    entry->name = &chunk->payload[8];
    typeFlags = chunk->typeFlags | 0x2000;
    entry->data = &chunk->payload[sizeof(chunk->payload)];
    entry->typeFlags = typeFlags;
    entry->used = 1;
    entry->height = 0;
    entry->width = 0;
    return index;
}

int svAddScript(SvTypeList *typeList, SvResourceChunk *chunk)
{
    SvTypeListEntry *entry;
    unsigned short typeFlags;
    int index;

    index = svImageListAlloc((int)typeList);
    if (index < 0) {
        return -1;
    }
    entry = &typeList->entries[index];
    entry->resourceKind = SV_RES_SCRIPT;
    entry->name = &chunk->payload[8];
    typeFlags = chunk->typeFlags;
    entry->data = &chunk->payload[sizeof(chunk->payload)];
    entry->typeFlags = typeFlags;
    entry->used = 1;
    entry->height = 0;
    entry->width = 0;
    return index;
}

/* Registers a script resource of the list: its code sits at +0xC0 of the chunk, its data at +0x14, its id at +0xA. */
int svAddScript2(int typeListAddr, void *script)
{
  int index;
  void *codePtr;
  void *dataPtr;
  void *itemSlot;
  SvScriptSlot *scriptSlot;

  index = svImageListAlloc(typeListAddr);
  codePtr = (unsigned char *) script + 0xC0;
  dataPtr = (unsigned char *) script + 0x14;
  itemSlot = (void *) (typeListAddr + (index * 0x24));
  if (index < 0)
  {
    return -1;
  }
  scriptSlot = (SvScriptSlot *) ((unsigned char *) itemSlot + 0x1C);
  scriptSlot->kind = 5;
  scriptSlot->dataPtr = dataPtr;
  scriptSlot->codePtr = codePtr;
  scriptSlot->id = (short) *((u16 *) (((unsigned char *) script) + 0xA));
  scriptSlot->active = 1;
  scriptSlot->clearedB = 0;
  scriptSlot->clearedA = 0;
  return index;
}

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_159", svAnalyzeChunk);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_159", svInitImageMapper);

void svImageListDestroy(SvTypeList *list);
void svAnalyzeChunk(SvTypeList *list, void *chunk);
SvTypeList *svGetTypeList(int type);

void svAddImageMapper(int typeListId, int entryCount, void *chunk, int tag) {
    SvTypeList *list;

    list = svGetTypeList(typeListId);
    if (list->pendingCount != 0) {
        svImageListDestroy(list);
    }
    list->tag = tag;
    list->entryCount = entryCount;
    list->pendingCount = list->pendingCount + 1;
    svAnalyzeChunk(list, chunk);
}

void svDeleteImageMapper(int typeListId) {
    SvTypeList *list;

    list = svGetTypeList(typeListId);
    if (list->pendingCount != 0) {
        svImageListDestroy(list);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_159", svDeleteImageMapperByEntryCount);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_159", svDeleteImageMapperByDimensions);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_159", svDeleteImageMapperByTag);
