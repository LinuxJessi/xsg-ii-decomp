#include "common.h"
#include "shared.h"

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
    short width;
    short height;
    short entryCount;
    unsigned char unmodeled_16[6];
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

int svImageListAlloc(int typeListAddr);

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

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", func_00253B60);

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

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", func_00253C70);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", func_00253CB0);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", func_00253D28);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", func_00253F38);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", func_00253FD0);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", func_00254010);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", func_002540C8);

INCLUDE_ASM("asm/main/nonmatchings/sv_get_image_item_158", func_00254170);
