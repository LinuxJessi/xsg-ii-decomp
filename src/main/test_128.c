#include "common.h"
#include "shared.h"

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
 * evidenced by "module '%s' line %d\n". magic holds the 15-character
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

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023CF98);

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

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023D140);

INCLUDE_ASM("asm/main/nonmatchings/test_128", _IsCollectMagicString);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023D1B0);

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

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023D288);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023D310);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023D340);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023D3D8);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023D688);

INCLUDE_ASM("asm/main/nonmatchings/test_128", RgHeapIsInSelf);

INCLUDE_ASM("asm/main/nonmatchings/test_128", RgHeapIsInvalidMemory);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023D988);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023DA90);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023DB40);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023DBA8);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023DC28);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023DE40);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023E058);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023E080);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023E248);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023E2F8);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023E930);

INCLUDE_ASM("asm/main/nonmatchings/test_128", func_0023E950);
