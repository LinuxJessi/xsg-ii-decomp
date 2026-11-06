#include "common.h"
#include "shared.h"
#include "main/sv_get_image_item_158.h"

extern void *memset(void *destination, int value, unsigned int count);

/* The image mapper: SV_TYPE_LIST_COUNT type lists of SV_ENTRY_COUNT entries. */
#define SV_TYPE_LIST_COUNT 40
#define SV_ENTRY_COUNT 0x100

/*
 * One 0x24-byte entry of the working image list svImageListCreate clears
 * and svGetImageItem indexes; only its address is ever taken in this
 * allocation, so its fields stay unmodeled.
 */
typedef struct SvImageItem {
    unsigned char unmodeled_00[0x24];
} SvImageItem;

/*
 * The single working image list: a 0x1C-byte header (untouched by this
 * allocation) followed by 0x100 SvImageItem entries (0x1C + 0x100*0x24 =
 * 0x241C, svImageListCreate's own clear size).
 */
typedef struct SvImageList {
    unsigned char unmodeled_header[0x1C];
    SvImageItem items[SV_ENTRY_COUNT];
} SvImageList;

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

typedef struct SvReferenceImageSlot {
    unsigned int words[2];
} SvReferenceImageSlot;

/* _imageMapper's symbol-table size is 40 mapper records followed by these
 * forty two-word reference-image slots. */
typedef struct SvImageMapperStorage {
    SvTypeList typeLists[SV_TYPE_LIST_COUNT];
    SvReferenceImageSlot referenceImages[SV_TYPE_LIST_COUNT];
} SvImageMapperStorage;

SvTypeList *svGetTypeList(int type);
extern SvImageList _imageList;
/* Forty 0x241C-byte mapper records followed by forty reference-image slots. */
extern SvImageMapperStorage _imageMapper;

void *svGetImageItem(int index) {
    return &_imageList.items[index];
}

void svImageListCreate(void) {
    memset(&_imageList, 0, sizeof(SvImageList));
}

/* Releases every entry of a type list (the used flags), then resets its counters, tag and dimensions. */
void svImageListDestroy(SvTypeList *typeList) {
    int index;

    for (index = 0; index < SV_ENTRY_COUNT; index++) {
        if (typeList->entries[index].used != 0) {
            typeList->entries[index].used = 0;
        }
    }
    typeList->savedSize = typeList->size;
    typeList->total = 0;
    typeList->entryCount = 0;
    typeList->tag = 0;
    typeList->dimensions = 0;
    typeList->pendingCount = 0;
}

int svImageListAlloc(int typeListAddr)
{
    SvTypeList *typeList;
    int index;

    typeList = (SvTypeList *)typeListAddr;
    for (index = 0; index < SV_ENTRY_COUNT; index++) {
        if (typeList->entries[index].used == 0) {
            return index;
        }
    }
    return -1;
}

SvTypeList * svGetTypeList(int type) {
    return &_imageMapper.typeLists[type];
}

void *svGetImageListItemSub(int type, unsigned int flags)
{
    SvTypeList *typeList;
    SvTypeList *storage;
    SvTypeListEntry *entry;
    int index;

    typeList = svGetTypeList(type);
    storage = typeList;
    entry = storage->entries;
    index = 0;
    while (index < SV_ENTRY_COUNT) {
        if (entry->used != 0 && entry->type == (unsigned char)flags) {
            return entry;
        }
        entry++;
        index++;
    }
    return 0;
}

/* Looks the list up with the type 0 for flag 0x100, else with the given type; a negative type returns 0. */
INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", func_00253C70);

/* Scans type list 0 for an active entry whose name equals `name`. */
INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", svGetResFromName);

/*
 * Builds the texture parameters of a bitmap and its CLUT from a base name,
 * looking the resource names up with svGetResFromName (name pieces "bmp",
 * "pxy", "clt", "txy"): texture register bits, width and height scaled by
 * 0.01. Clears the parameter and returns 0 when either resource is missing.
 */
INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", svGetPrmFromName);

/* Page count of a texture for its pixel format: 128x64 pages for PSMT8 (0x13), 128x128 for PSMT4 (0x14), else width * height / 128. */
INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", func_00253F38);

/* Buffer width of a texture in 64-pixel units, rounded up to an even number, for PSMT8 and PSMT4; 0 for other formats. */
INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", svGetTextureBufferWidth);

/* Uploads one texture (PSMT8 or PSMT4, buffer width in 64-pixel units) to the current packet. */
INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", svLoadTexture);

void *xglPacketGetCurrent(void);
int func_0012BA78(int, u16, int, int, const void *, int, int, int, int, int);

/* Uploads a palette: 16x16 for 8-bit entries, 8x2 for 4-bit, to the current packet. */
void svLoadClut(SvTypeListEntry *entry) {
    u16 basePointer;
    void *packet;

    basePointer = entry->basePointer;
    packet = xglPacketGetCurrent();
    if (entry->bitsPerPixel == 8) {
        func_0012BA78(packet, basePointer, 0, 1, entry->data, 0x40, 0, 0, 0x10, 0x10);
        return;
    }
    func_0012BA78(packet, basePointer, 0, 1, entry->data, 4, 0, 0, 8, 2);
}

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", svLoadImageList);
