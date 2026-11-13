#include "common.h"
#include "shared.h"
#include "main/test_128.h"

/* Heap count of the heap windows (the assert text names it HEAP_MAX). */
#define HEAP_MAX 6
/* Size of one heap arena (the 2048 bytes InitRgHeap is given). */
#define HEAP_ARENA_SIZE 0x800
/* Smallest block a heap keeps: the 0x40-byte RgHeapBlock header plus one byte. */
#define HEAP_MIN_BLOCK_SIZE 65
/* Length of the block magic stamp (RgHeapBlock.magic holds it plus a terminator). */
#define HEAP_MAGIC_SIZE 15
/* Heap map: 256 pixels wide, painted over the arena in colour HEAP_MAP_ARENA_COLOR and the blocks in HEAP_MAP_BLOCK_COLOR. */
#define HEAP_MAP_WIDTH 256
#define HEAP_MAP_ARENA_COLOR 0x80000080
#define HEAP_MAP_BLOCK_COLOR 0x80008080
/* Window placement: the per-heap window (heapWindowCreate), the allocation position window (heapWindowOpen)
 * and the overview of all heaps (heapOverviewWindowOpen). */
#define HEAP_WINDOW_X 100
#define HEAP_WINDOW_Y 80
#define HEAP_WINDOW_WIDTH 256
#define HEAP_WINDOW_HEIGHT 300
#define HEAP_ALLOC_WINDOW_X 10
#define HEAP_ALLOC_WINDOW_Y 120
#define HEAP_ALLOC_WINDOW_WIDTH 480
#define HEAP_ALLOC_WINDOW_HEIGHT 200
#define HEAP_OVERVIEW_WINDOW_X 100
#define HEAP_OVERVIEW_WINDOW_Y 100
#define HEAP_OVERVIEW_WINDOW_WIDTH 200
#define HEAP_OVERVIEW_WINDOW_HEIGHT 140
/* Text scale options of textScaleApplyOptions and the clamp range of the scale (1.0 is 0x10000). */
#define TEXT_SCALE_OPTION_DOUBLE 0x10
#define TEXT_SCALE_OPTION_HALF 0x80
#define TEXT_SCALE_MIN 1
#define TEXT_SCALE_MAX 0x10000

/*
 * RgHeapBlock: the fixed 0x40-byte header RgHeapAlloc places immediately
 * before the block it returns (RgHeapDumpBlock reads it at pPtr - 0x40).
 * pre/next link the block into whichever circular list currently owns it
 * (RgHeap's free chain from _SetHeapHead, or its allocated chain from
 * _Link/_InsertNext), evidenced by "pre %p next %p\n". size is the block's
 * byte size and mark is set/cleared by _MarkAlloc/_UnmarkAlloc/_IsAllocated,
 * evidenced by "size %p mark %d\n" (size itself is dumped through %p in that
 * string). module and line record the RgHeapAlloc call site (module is
 * strncpy'd from its source_file argument and terminated at module[31]),
 * evidenced by "module '%s' line %d\n". The dumps print module + 1, and
 * RgHeapFree reads module[0] as the owning heap id before it looks the heap
 * up. magic holds the 15-character
 * s_szMagicString stamp _SetMagicString/_IsCollectMagicString read and write
 * at block+0x30. Every byte from 0x00 to 0x3F is accounted for by these six
 * members (4+4+4+2+2+32+16 == 0x40).
 */
struct RgHeapBlock {
    struct RgHeapBlock *pre;
    struct RgHeapBlock *next;
    u32 size;
    u16 mark;
    u16 line;
    char module[32];
    char magic[16];
};

/*
 * The heap head: top and size are printed as "heap = %p top = %p size = %d",
 * +0x08 is the top of the free chain that the dump prints as "FREE MAP top",
 * +0x0C heads the allocated chain. +0x10 (mapStride) is stored as 0x100 by
 * InitRgHeap; heapWindowPaintMap uses it both as the map row length and as the
 * bytes per map cell.
 */
struct RgHeap {
    void *top;
    int size;
    struct RgHeapBlock *freeHead;
    struct RgHeapBlock *allocHead;
    int mapStride;
};

int printf(const char *format, ...);
char *strcpy(char *destination, const char *source);
void func_A9229C(int, void *);
void RgHeapDump_sub(struct RgHeap *pHeap, const char *pComment, const char *pSourceFile, int line);
extern char D_0068BA50[];
extern char D_0068BA78[];
extern char D_0068BA90[];
extern char D_0068BAB0[];
extern char D_0068BAC0[];
extern char D_0068BAD0[];

/*
 * Heap assertion handler: prints the failed expression with its call site,
 * dumps the heap when one is given, then spins forever. The second argument
 * is the tag the callers pass ("make block", "init-2"); this build never
 * prints it.
 */
void _Error(const char *pExpression, const void *unused, struct RgHeap *pHeap,
                   const char *pSourceFile, int line)
{
    printf(D_0068BA50);
    printf(D_0068BA78, pExpression, pSourceFile, line);
    if (pHeap != 0) {
        printf(D_0068BA90, pHeap, pHeap->top, pHeap->size);
        RgHeapDump_sub(pHeap, D_0068BAB0, D_0068BAC0, 0xA1);
    } else {
        printf(D_0068BAD0);
    }
    for (;;) {
    }
}

void _Link(struct RgHeapBlock *pBlock, struct RgHeapBlock *pPre,
                  struct RgHeapBlock *pNext)
{
    pBlock->pre = pPre;
    pBlock->next = pNext;
}

void _InsertPre(struct RgHeapBlock *pBlock, struct RgHeapBlock *pNew)
{
    _Link(pNew, pBlock->pre, pBlock);
    pBlock->pre->next = pNew;
    pBlock->pre = pNew;
}

void _InsertNext(struct RgHeapBlock *pBlock, struct RgHeapBlock *pNew)
{
    _Link(pNew, pBlock, pBlock->next);
    pBlock->next->pre = pNew;
    pBlock->next = pNew;
}

void _Unlink(struct RgHeapBlock *pBlock)
{
    pBlock->next->pre = pBlock->pre;
    pBlock->pre->next = pBlock->next;
}

void _MarkAlloc(struct RgHeapBlock *pBlock)
{
    pBlock->mark = 1;
}

void _UnmarkAlloc(struct RgHeapBlock *pBlock)
{
    pBlock->mark = 0;
}

int _IsAllocated(struct RgHeapBlock *pBlock)
{
    return pBlock->mark != 0;
}

extern unsigned char *heapMagicStringPtr;

/* Stamps the 15-byte magic (read through the heapMagicStringPtr pointer) into the block header. */
void _SetMagicString(struct RgHeapBlock *pBlock)
{
    char *pMagic = pBlock->magic;
    int index = 0;

    do {
        *pMagic = heapMagicStringPtr[index];
        index += 1;
        pMagic += 1;
    } while (index < HEAP_MAGIC_SIZE);
}

/* Returns 1 when the 15 magic bytes at block+0x30 equal the stamp heapMagicStringPtr points to, else 0. */
int _IsCollectMagicString(struct RgHeapBlock *pBlock)
{
    int index = 0;

    do {
        if ((unsigned char)pBlock->magic[index] != (signed char)heapMagicStringPtr[index]) {
            return 0;
        }
        index += 1;
    } while (index < HEAP_MAGIC_SIZE);
    return 1;
}

extern char D_0068BAF8[];
extern char D_0068BB10[];

struct RgHeapBlock *_SetHeapHead(struct RgHeapBlock *pBlock, u32 size)
{
    if (size < HEAP_MIN_BLOCK_SIZE) {
        _Error(D_0068BAF8, D_0068BB10, 0, D_0068BAC0, 269);
    }
    _Link(pBlock, pBlock, pBlock);
    pBlock->size = size;
    return pBlock;
}

/*
 * Walks the circular free list once and folds every block that ends exactly
 * where its successor starts into that block.
 */
void _MergeBlocks(struct RgHeapBlock *pHead)
{
    struct RgHeapBlock *pBlock = pHead;
    struct RgHeapBlock *pNext;

    if (pHead == 0) {
        return;
    }
    do {
        pNext = pBlock->next;
        while ((u8 *)pBlock + pBlock->size == (u8 *)pNext) {
            pBlock->next = pNext->next;
            pBlock->size += pNext->size;
            pNext->next->pre = pBlock;
            pNext = pBlock->next;
        }
        pBlock = pNext;
    } while (pBlock != pHead);
}

/* InstanceOfRgHeapById: the heap heapId (0x20 bytes apart); its 0x800-byte arena is set up by the first call, tracked by a halfword flag per id. */
extern unsigned char heapInstances[HEAP_MAX][0x20];
extern unsigned char heapArenas[HEAP_MAX][HEAP_ARENA_SIZE];
extern short heapInitFlags[HEAP_MAX];
void InitRgHeap(struct RgHeap *pHeap, void *pMemory, int size);

struct RgHeap *InstanceOfRgHeapById(u32 heapId)
{
    if (heapInitFlags[heapId] == 0) {
        heapInitFlags[heapId] = 1;
        InitRgHeap((struct RgHeap *)heapInstances[heapId], heapArenas[heapId], HEAP_ARENA_SIZE);
    }
    return (struct RgHeap *)heapInstances[heapId];
}

void ClearRgHeap(struct RgHeap *pHeap)
{
    pHeap->freeHead = _SetHeapHead(pHeap->top, pHeap->size);
}

extern char D_0068BB38[];
extern char D_0069DAB0[];
void *memset(void *destination, int value, unsigned int count);

void InitRgHeap(struct RgHeap *pHeap, void *pMemory, int size)
{
    struct RgHeapBlock *pFreeBlock;

    if (size & 0xF) {
        _Error(D_0068BB38, D_0069DAB0, 0, D_0068BAC0, 321);
    }
    memset(pMemory, 0, size);
    pHeap->size = size;
    pHeap->top = pMemory;
    pFreeBlock = _SetHeapHead(pMemory, size);
    pHeap->allocHead = 0;
    pHeap->freeHead = pFreeBlock;
    pHeap->mapStride = 0x100;
}

extern char D_0068BB50[];
extern char D_0068BB78[];
extern char D_0068BB88[];
extern char D_0068BBA0[];
extern char D_0068BBD0[];
extern char D_0068BBE0[];
extern char D_0068BC18[];
extern char D_0068BC40[];
extern char D_0068BC50[];
extern char D_0068BC80[];
int sprintf(char *destination, const char *format, ...);
void func_002AB1A8(char *pDest, const char *pSrc, u32 count);

void *RgHeapAllocById(u32 heapId, u32 size, const char *pSourceFile, int line)
{
    char buf[32];
    struct RgHeap *pHeap;
    struct RgHeapBlock *pFreeHead;
    struct RgHeapBlock *pBlock;
    struct RgHeapBlock *pFit;
    struct RgHeapBlock *pSplit;
    u32 bestSize;
    u32 blockSize;
    u32 remainder;
    void *pPayload;

    pHeap = InstanceOfRgHeapById(heapId);
    if (size == 0) {
        printf(D_0068BB50, pSourceFile, line);
    }
    if (pHeap == 0) {
        _Error(D_0068BB78, D_0068BB88, 0, D_0068BAC0, 347);
    }
    pFreeHead = pHeap->freeHead;
    if (pFreeHead == 0) {
        printf(D_0068BBA0, pSourceFile, line);
        return 0;
    }
    pFit = 0;
    blockSize = ((size + 15) & -16) + sizeof(struct RgHeapBlock);
    bestSize = 0;
    pBlock = pFreeHead;
    do {
        if (pBlock->size >= blockSize) {
            if (pBlock->size < bestSize || bestSize == 0) {
                bestSize = pBlock->size;
                pFit = pBlock;
            }
        }
        pBlock = pBlock->next;
    } while (pBlock != pFreeHead);
    if (pFit == 0) {
        sprintf(buf, D_0068BBD0, heapId);
        RgHeapDump_sub(pHeap, buf, D_0068BAC0, 375);
        printf(D_0068BBE0, size, pSourceFile, line);
        return 0;
    }
    if (bestSize < blockSize) {
        _Error(D_0068BC18, D_0068BC40, pHeap, D_0068BAC0, 385);
    }
    remainder = bestSize - blockSize;
    if (remainder >= HEAP_MIN_BLOCK_SIZE) {
        pSplit = (struct RgHeapBlock *)((u8 *)pFit + blockSize);
        _SetHeapHead(pSplit, remainder);
    } else {
        blockSize = bestSize;
        pSplit = 0;
    }
    if (pSplit != 0) {
        _InsertNext(pFit, pSplit);
    }
    if (pFit == pFreeHead) {
        pHeap->freeHead = pFit->next;
    }
    _Unlink(pFit);
    _SetHeapHead(pFit, blockSize);
    _MarkAlloc(pFit);
    if (pSourceFile != 0) {
        func_002AB1A8(pFit->module + 1, pSourceFile, 30);
        pFit->module[31] = 0;
        pFit->line = line;
    } else {
        pFit->module[1] = 0;
        pFit->line = 0;
    }
    _SetMagicString(pFit);
    pFit->module[0] = heapId;
    if (pHeap->allocHead == 0) {
        pHeap->allocHead = pFit;
        _Link(pFit, pFit, pFit);
    } else {
        _InsertNext(pHeap->allocHead, pFit);
    }
    pPayload = pFit + 1;
    if (((u32)pPayload & 15) != 0) {
        _Error(D_0068BC50, D_0068BC80, pHeap, D_0068BAC0, 429);
    }
    return pPayload;
}

extern char D_0068BC98[];
extern char D_0068BCA8[];
extern char D_0068BCB8[];
extern char D_0068BCD8[];
extern char D_0068BCE8[];
extern char D_0068BD28[];
extern char D_0068BD48[];
extern char D_0068BD80[];
extern char D_0068BDB0[];
int RgHeapIsInSelf(struct RgHeap *pHeap, void *pPtr);

void RgHeapFree(void *pPtr, const char *pSourceFile, int line)
{
    struct RgHeapBlock *pBlock;
    struct RgHeap *pHeap;
    struct RgHeapBlock *pAllocHead;
    struct RgHeapBlock *pNext;
    struct RgHeapBlock *pCur;

    pBlock = (struct RgHeapBlock *)pPtr - 1;
    pHeap = InstanceOfRgHeapById(pBlock->module[0]);
    if (pHeap == 0) {
        _Error(D_0068BC98, D_0068BCA8, 0, D_0068BAC0, 443);
    }
    if (pPtr == 0) {
        printf(D_0068BCB8, pSourceFile, line);
    }
    if (_IsAllocated(pBlock) == 0) {
        if (_IsCollectMagicString(pBlock) != 0) {
            RgHeapDump_sub(pHeap, D_0068BCD8, D_0068BAC0, 456);
            printf(D_0068BCE8, pPtr, pSourceFile, line, pBlock->module + 1, pBlock->line);
        } else {
            printf(D_0068BD28, pPtr, pSourceFile, line);
        }
    }
    if (_IsCollectMagicString(pBlock) == 0) {
        printf(D_0068BD48, pPtr, pSourceFile, line);
    }
    if (RgHeapIsInSelf(pHeap, pBlock) == 0) {
        printf(D_0068BD80, pSourceFile, pHeap, pSourceFile, line);
    }
    pAllocHead = pHeap->allocHead;
    pNext = pAllocHead->next;
    if (pNext == pAllocHead) {
        if (pBlock != pNext) {
            printf(D_0068BDB0);
        }
        pHeap->allocHead = 0;
    } else {
        if (pAllocHead == pBlock) {
            _Unlink(pBlock);
            pHeap->allocHead = pNext;
        } else {
            _Unlink(pBlock);
        }
    }
    _UnmarkAlloc(pBlock);
    if (pHeap->freeHead == 0) {
        pHeap->freeHead = _SetHeapHead(pBlock, pBlock->size);
        return;
    }
    pCur = pHeap->freeHead;
    _Link(pBlock, 0, 0);
    do {
        if (pBlock < pCur) {
            _InsertPre(pCur, pBlock);
            if (pBlock < pHeap->freeHead) {
                pHeap->freeHead = pBlock;
            }
            break;
        }
        pCur = pCur->next;
    } while (pCur != pHeap->freeHead);
    if (pBlock->next == 0) {
        _InsertPre(pHeap->freeHead, pBlock);
    }
    _MergeBlocks(pHeap->freeHead);
}

int RgHeapIsInSelf(struct RgHeap *pHeap, void *pPtr)
{
    if (pPtr < pHeap->top || (char *)pPtr >= (char *)pHeap->top + pHeap->size) {
        return 0;
    }
    return 1;
}

int RgHeapIsInvalidMemory(struct RgHeap *pHeap, void *pPtr)
{
    struct RgHeapBlock *pBlock;

    if (pPtr == 0) {
        return 1;
    }
    pBlock = (struct RgHeapBlock *)pPtr - 1;
    if (_IsAllocated(pBlock) == 0) {
        return 1;
    }
    if (_IsCollectMagicString(pBlock) == 0) {
        return 1;
    }
    return RgHeapIsInSelf(pHeap, pBlock) == 0;
}

extern char D_0068BDE0[];
extern char D_0068BE08[];
extern char D_0068BE28[];
extern char D_0068BE50[];
extern char D_0068BE70[];
extern char D_0068BE98[];

void RgHeapDump_sub(struct RgHeap *pHeap, const char *pComment, const char *pSourceFile, int line)
{
    struct RgHeapBlock *pBlock;

    pBlock = pHeap->freeHead;
    printf(D_0068BDE0);
    printf(D_0068BE08, pHeap, pSourceFile, line);
    printf(D_0068BE28, pBlock, pComment);
    do {
        printf(D_0068BE50, pBlock, pBlock->size, pBlock->size, pBlock->next);
        pBlock = pBlock->next;
    } while (pBlock != pHeap->freeHead);
    pBlock = pHeap->allocHead;
    printf(D_0068BE70, pBlock, pComment);
    if (pBlock != 0) {
        do {
            printf(D_0068BE98, pBlock, pBlock->size - sizeof(struct RgHeapBlock), pBlock->size, pBlock->next,
                   pBlock->module + 1, pBlock->line);
            pBlock = pBlock->next;
        } while (pBlock != pHeap->allocHead);
    }
}

extern char D_0068BEC8[];
extern char D_0068BEE0[];
extern char D_0068BEF0[];
extern char D_0068BF08[];
extern char D_0068BF20[];

void RgHeapDumpBlock(struct RgHeap *pHeap, void *pPtr)
{
    struct RgHeapBlock *pBlock;

    printf(D_0068BEC8);
    pBlock = (struct RgHeapBlock *) pPtr - 1;
    if (RgHeapIsInSelf(pHeap, pPtr) != 0) {
        printf(D_0068BEE0, pBlock->pre, pBlock->next);
        printf(D_0068BEF0, pBlock->size, pBlock->mark);
        printf(D_0068BF08, pBlock->module + 1, pBlock->line);
        return;
    }
    printf(D_0068BF20, pHeap);
}

extern char D_0068BF40[];
struct RgHeap *InstanceOfRgHeapById(u32 heapId);

void RgHeapDumpById(u32 heapId)
{
    char sourceFile[0x80];
    struct RgHeap *pHeap;

    if (heapId < HEAP_MAX) {
        pHeap = InstanceOfRgHeapById(heapId);
        sprintf(sourceFile, D_0068BF40, heapId);
        RgHeapDump_sub(pHeap, sourceFile, D_0068BAC0, 611);
    }
}

/*
 * Clamps the text scale of the record returned by func_00A920EC: doubled when
 * option flag 0x10 is set, halved for 0x80, then kept within 1..0x10000.
 */
struct TextScaleRecord {
    unsigned char unmodeled_000[0x10];
    int scale;
};

extern u16 textOptionFlags[];
void *func_00A920EC(void);

void textScaleApplyOptions(void)
{
    struct TextScaleRecord *pRecord;
    u16 flags;

    pRecord = func_00A920EC();
    flags = textOptionFlags[0];
    if ((flags & TEXT_SCALE_OPTION_DOUBLE) != 0) {
        pRecord->scale = pRecord->scale * 2;
    }
    if ((flags & TEXT_SCALE_OPTION_HALF) != 0) {
        pRecord->scale = pRecord->scale / 2;
    }
    if (pRecord->scale <= 0) {
        pRecord->scale = TEXT_SCALE_MIN;
    }
    if (pRecord->scale > TEXT_SCALE_MAX) {
        pRecord->scale = TEXT_SCALE_MAX;
    }
}

struct HeapWindowInfo {
    unsigned char unmodeled_00[0xC];
    int rowCount;
};

struct RgHeap *func_A920EC(int window);
void func_A91AF4(int window, struct HeapWindowInfo *pInfo);
void func_A90DF8(int window, int x0, int y0, int x1, int y1, u32 color);

/*
 * Paints the heap map of a window. The arena is first filled in map rows
 * numbered from 1 with colour 0x80000080; a map row is mapStride cells wide
 * (InitRgHeap stores 0x100 there) and a cell is mapStride bytes, so the
 * partial last row is scaled to its share of the 256-pixel width. Every block
 * of the allocated chain (allocHead) is then painted over it with colour
 * 0x80008080, one run per map row. func_A91AF4 supplies the row count in
 * info.rowCount.
 */
void heapWindowPaintMap(int window)
{
    struct HeapWindowInfo info;
    struct RgHeap *pHeap;
    struct RgHeapBlock *pBlock;
    int arenaCells;
    int blockRow;
    u32 blockCells;
    int mapRow = 1;
    u32 blockCol;
    u32 runCells;
    u32 byteOffset;
    u32 cellIndex;

    pHeap = func_A920EC(window);
    func_A91AF4(window, &info);

    arenaCells = (u32)pHeap->size / pHeap->mapStride;
    if (arenaCells != 0) {
        do {
            if (arenaCells >= pHeap->mapStride) {
                func_A90DF8(window, 0, mapRow, HEAP_MAP_WIDTH, mapRow, HEAP_MAP_ARENA_COLOR);
                arenaCells -= pHeap->mapStride;
            } else {
                func_A90DF8(window, 0, mapRow, (arenaCells << 8) / pHeap->mapStride, mapRow, HEAP_MAP_ARENA_COLOR);
                arenaCells = 0;
            }
            mapRow += 1;
        } while (mapRow < info.rowCount && arenaCells != 0);
    }

    pBlock = pHeap->allocHead;
    if (pBlock != 0) {
        do {
            byteOffset = (u32)pBlock - (u32)pHeap->top;
            cellIndex = byteOffset / pHeap->mapStride;
            blockCells = pBlock->size / pHeap->mapStride;
            blockRow = cellIndex / pHeap->mapStride;
            blockCol = cellIndex % pHeap->mapStride;

            runCells = pHeap->mapStride - blockCol;
            if (blockCells < runCells) {
                runCells = blockCells;
            }
            func_A90DF8(window, blockCol, blockRow, ((blockCol + runCells) << 8) / pHeap->mapStride,
                        blockRow, HEAP_MAP_BLOCK_COLOR);
            blockCells -= runCells;
            blockRow += 1;

            do {
                runCells = pHeap->mapStride;
                if (blockCells < runCells) {
                    runCells = blockCells;
                }
                func_A90DF8(window, 0, blockRow, (runCells << 8) / pHeap->mapStride, blockRow, HEAP_MAP_BLOCK_COLOR);
                blockCells -= runCells;
                blockRow += 1;
            } while (blockRow <= info.rowCount && blockCells != 0);
            pBlock = pBlock->next;
        } while (pBlock != pHeap->allocHead);
    }
}

struct HeapWindowRow {
    void *callbacks[2];
    char *pModuleName;
    u16 *pLine;
    unsigned char unmodeled_10[0x24];
    union {
        struct RgHeap *pHeap;
        u32 heapId;
    } heap;
    unsigned char unmodeled_38[0x18];
};

extern struct HeapWindowRow heapWindowAllocRowTemplate;
void func_A923D8(int window, struct HeapWindowRow *pRow);

/*
 * One window row per allocated block of heap 0. Every row starts as a copy of
 * the heapWindowAllocRowTemplate template; pModuleName points at module[1] (module[0] holds the
 * owning heap id, see RgHeapFree) and pLine at the block's line field.
 */
void heapWindowAddAllocRows(int window)
{
    struct HeapWindowRow row = heapWindowAllocRowTemplate;
    struct RgHeap *pHeap = InstanceOfRgHeapById(0);
    struct RgHeapBlock *pBlock = pHeap->allocHead;

    if (pBlock != 0) {
        do {
            row = heapWindowAllocRowTemplate;
            row.pModuleName = &pBlock->module[1];
            row.pLine = &pBlock->line;
            func_A923D8(window, &row);
            pBlock = pBlock->next;
        } while (pBlock != pHeap->allocHead);
    }
}

void func_A91E04(void);

/* Window callback registered by heapWindowOpen: calls func_A91E04 on the window, then rebuilds its heap
 * contents with heapWindowAddAllocRows, the same call heapWindowOpen makes once when it opens the window. */
void heapWindowRefresh(int window)
{
    func_A91E04();
    heapWindowAddAllocRows(window);
}

struct WindowCreateParam {
    int x;
    int y;
    int width;
    int height;
    char name[32];
};

extern char *heapWindowTitles[];
extern char heapWindowIdAssertText[];
extern struct HeapWindowRow heapWindowRowTemplate;
extern char D_0069DAB8[8];
int func_A917D4(int parentWindow, const char *pName);
void func_A91668(int window);
int func_A92704(struct WindowCreateParam *pParam, int parentWindow);
void assertFailed(const char *pExpression, const char *pSourceFile, int line);

/*
 * Heap window for the heap id func_A920EC gives parentWindow (below HEAP_MAX). If
 * func_A917D4 finds a window for the named slot, it is shown with func_A91668
 * and nothing else happens. Otherwise a window is created with func_A92704 at
 * (100, 80), 256 by 300, and the heap's row is passed to func_A923D8.
 */
void heapWindowCreate(int parentWindow)
{
    struct WindowCreateParam param;
    struct HeapWindowRow row;
    u32 nId;
    int window;

    nId = func_A920EC(parentWindow);
    if (nId >= HEAP_MAX) {
        assertFailed(heapWindowIdAssertText, D_0068BAC0, 800);
    }
    window = func_A917D4(parentWindow, heapWindowTitles[nId]);
    if (window >= 0) {
        func_A91668(window);
        return;
    }
    param.x = HEAP_WINDOW_X;
    param.y = HEAP_WINDOW_Y;
    param.width = HEAP_WINDOW_WIDTH;
    param.height = HEAP_WINDOW_HEIGHT;
    strcpy(param.name, heapWindowTitles[nId]);
    window = func_A92704(&param, parentWindow);
    row = heapWindowRowTemplate;
    row.pModuleName = D_0069DAB8;
    row.heap.pHeap = InstanceOfRgHeapById(nId);
    func_A923D8(window, &row);
}

extern char heapWindowAllocTitle[];
void func_A9262C(int window, void (*pCallback)(int));

void heapWindowOpen(int parentWindow)
{
    const char *pName = heapWindowAllocTitle;
    struct WindowCreateParam param;
    int window;

    window = func_A917D4(parentWindow, pName);
    if (window >= 0) {
        func_A91668(window);
    } else {
        param.x = HEAP_ALLOC_WINDOW_X;
        param.y = HEAP_ALLOC_WINDOW_Y;
        param.width = HEAP_ALLOC_WINDOW_WIDTH;
        param.height = HEAP_ALLOC_WINDOW_HEIGHT;
        strcpy(param.name, pName);
        window = func_A92704(&param, parentWindow);
        func_A9262C(window, heapWindowRefresh);
        heapWindowAddAllocRows(window);
    }
}

extern char heapOverviewTitle[8];
extern char heapWindowTitleHeap0[];
extern char heapWindowTitleHeap1[];
extern char heapWindowTitleHeap2[];
extern char heapWindowTitleHeap3[];
extern char heapWindowTitleSystem[];
extern char heapWindowTitleMatrix[];
extern char D_003ED280[];
extern struct HeapWindowRow heapOverviewRowTemplate;
extern char D_0068BF70[];
void func_AB847C(const char *pText, int window);

void heapOverviewWindowOpen(int parentWindow)
{
    struct WindowCreateParam param;
    struct HeapWindowRow row;
    int window;

    window = func_A917D4(parentWindow, heapOverviewTitle);
    if (window >= 0) {
        func_A91668(window);
        return;
    }
    param.x = HEAP_OVERVIEW_WINDOW_X;
    param.y = HEAP_OVERVIEW_WINDOW_Y;
    param.width = HEAP_OVERVIEW_WINDOW_WIDTH;
    param.height = HEAP_OVERVIEW_WINDOW_HEIGHT;
    strcpy(param.name, heapOverviewTitle);
    window = func_A92704(&param, parentWindow);
    func_A9229C(window, D_003ED280);
    row = heapOverviewRowTemplate;
    row.pModuleName = heapWindowTitleHeap0;
    row.heap.heapId = 0;
    func_A923D8(window, &row);
    row = heapOverviewRowTemplate;
    row.pModuleName = heapWindowTitleHeap1;
    row.heap.heapId = 1;
    func_A923D8(window, &row);
    row = heapOverviewRowTemplate;
    row.pModuleName = heapWindowTitleHeap2;
    row.heap.heapId = 2;
    func_A923D8(window, &row);
    row = heapOverviewRowTemplate;
    row.pModuleName = heapWindowTitleHeap3;
    row.heap.heapId = 3;
    func_A923D8(window, &row);
    row = heapOverviewRowTemplate;
    row.pModuleName = heapWindowTitleSystem;
    row.heap.heapId = 4;
    func_A923D8(window, &row);
    row = heapOverviewRowTemplate;
    row.pModuleName = heapWindowTitleMatrix;
    row.heap.heapId = 5;
    func_A923D8(window, &row);
    func_AB847C(D_0068BF70, window);
}

extern unsigned char D_003ED1E0[];

void heapOverviewMenuAttach(int value)
{
    func_A9229C(value, D_003ED1E0);
}

void func_00A82EEC(int, int, int, int, int);

void func_0023E950(int value)
{
    func_00A82EEC(value, 0, 0, 0, 0);
}
