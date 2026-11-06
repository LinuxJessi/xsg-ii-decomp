#include "common.h"
#include "shared.h"
#include "main/test_128.h"

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
 * +0x0C heads the allocated chain. +0x10 is stored as 0x100 by the heap setup
 * and has no reader in this TU.
 */
struct RgHeap {
    void *top;
    int size;
    struct RgHeapBlock *freeHead;
    struct RgHeapBlock *allocHead;
    int reserved10;
};

int printf(const char *format, ...);
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
    } while (index < 15);
}

INCLUDE_ASM("asm/main/nonmatchings/test_128", _IsCollectMagicString);

extern char D_0068BAF8[];
extern char D_0068BB10[];

struct RgHeapBlock *_SetHeapHead(struct RgHeapBlock *pBlock, u32 size)
{
    if (size < 65) {
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
INCLUDE_ASM("asm/main/nonmatchings/test_128", InstanceOfRgHeapById);

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
    pHeap->reserved10 = 0x100;
}

INCLUDE_ASM("asm/main/nonmatchings/test_128", RgHeapAllocById);

INCLUDE_ASM("asm/main/nonmatchings/test_128", RgHeapFree);

INCLUDE_ASM("asm/main/nonmatchings/test_128", RgHeapIsInSelf);

INCLUDE_ASM("asm/main/nonmatchings/test_128", RgHeapIsInvalidMemory);

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
            printf(D_0068BE98, pBlock, pBlock->size - 0x40, pBlock->size, pBlock->next,
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
int RgHeapIsInSelf(struct RgHeap *pHeap, void *pPtr);

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
void func_002AA5D8(char *pDest, const char *pSrc, u32 count);

void RgHeapDumpById(u32 heapId)
{
    char sourceFile[0x80];
    struct RgHeap *pHeap;

    if (heapId < 6) {
        pHeap = InstanceOfRgHeapById(heapId);
        func_002AA5D8(sourceFile, D_0068BF40, heapId);
        RgHeapDump_sub(pHeap, sourceFile, D_0068BAC0, 611);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023DBA8);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023DC28);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023DE40);

void func_0023DE40(int);
void func_A91E04(void);

/* Window callback registered by heapWindowOpen: calls func_A91E04 on the window, then rebuilds its heap
 * contents with func_0023DE40, the same call heapWindowOpen makes once when it opens the window. */
void heapWindowRefresh(int window)
{
    func_A91E04();
    func_0023DE40(window);
}

INCLUDE_ASM("asm/main/nonmatchings/test_128", heapWindowCreate);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023E248);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023E2F8);

extern unsigned char D_003ED1E0[];
void func_A9229C(int, void *);

void func_0023E930(int value)
{
    func_A9229C(value, D_003ED1E0);
}

void func_00A82EEC(int, int, int, int, int);

void func_0023E950(int value)
{
    func_00A82EEC(value, 0, 0, 0, 0);
}
