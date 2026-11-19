#ifndef INCLUDE_MAIN_SV_GET_IMAGE_ITEM_H
#define INCLUDE_MAIN_SV_GET_IMAGE_ITEM_H

#include "common.h"

/* The image mapper: SV_TYPE_LIST_COUNT type lists of SV_ENTRY_COUNT entries. */
#define SV_TYPE_LIST_COUNT 40
#define SV_ENTRY_COUNT 0x100

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

/* SvTypeListEntry.resourceKind, equal to the chunk kind it was built from. */
enum {
    SV_RES_CLUT = 3,
    SV_RES_IMAGE = 4,
    SV_RES_SCRIPT = 5,
    SV_RES_MODEL = 6
};

typedef struct SvResourceChunk {
    unsigned int byteSize;
    unsigned char unmodeled_04[4];
    short kind;
    unsigned short typeFlags;
    char payload[0x34];
} SvResourceChunk;

typedef struct SvImageChunk {
    unsigned int byteSize;
    unsigned int dataSize;          /* bytes of pixel data after the header */
    unsigned char unmodeled_08[2];
    unsigned short typeFlags;
    unsigned short width;
    unsigned short height;
    short bitsPerPixel;
    unsigned char unmodeled_12[2];
    char name[0x2C];
} SvImageChunk;

/* A CLUT chunk: the payload's leading halfwords are the size and depth, the name follows at +0x14. */
typedef struct SvClutChunk {
    unsigned char unmodeled_00[10];
    unsigned short typeFlags;
    unsigned short width;
    unsigned short height;
    unsigned short bitsPerPixel;
    unsigned char unmodeled_12[2];
    char name[0x2C];
    unsigned char data[1];
} SvClutChunk;

extern int _draw3D;
/* The _draw3D modes: 2D items, 2D kind-8 sprites and 3D models. */
enum SvRenderMode {
    SV_RENDER_2D = 0,
    SV_RENDER_2D_SPRITE = 1,
    SV_RENDER_3D_MODEL = 2
};

/* Functions of src/main/sv_get_image_item.c that other files call. */

int svImageListAlloc(SvTypeList *typeList);

void svAddImageMapper(int typeListId, int entryCount, void *chunk, int tag);
void svDeleteImageMapper(int typeListId);
SvTypeListEntry *svGetScriptEntry(int effectId);

/* Effect slot functions of sv_get_image_item.c, the record table of srs.h (srsRecordTable). */
int srsLoadEffectSlot(void *buffer, int effectId, int arg);
int srsFindFreeEffectSlot(void);
void srsFreeEffectSlot(unsigned int index);
void srsUnloadEffectSlot(unsigned int index);
int srsEvictEffectSlot(void);
int srsEvictEffectSlotsForSize(int size);
int srsIsEffectKeyListed(int key);
void srsReleaseOtherOwnerEffectSlots(void);

#endif /* INCLUDE_MAIN_SV_GET_IMAGE_ITEM_H */
