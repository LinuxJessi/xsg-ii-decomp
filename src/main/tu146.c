#include "common.h"
#include "main/tu146.h"

extern void *memset(void *destination, int value, unsigned int count);

/*
 * A node of the chain walked by cfeListTail, linked through the pointer
 * at +0x20; the other fields are not used by this allocation.
 */
typedef struct CfeListNode {
    unsigned char unmodeled_00[0x20];
    struct CfeListNode *next;
} CfeListNode;

/*
 * One 0x30-byte node of the pool that cfeNodePool points to (6144 nodes). Its
 * first word is a flags word: bit 0 marks a node in use. cfeNodeAlloc
 * allocates the first free node, cfeNodeRelease (cfeNodeRelease) frees one and
 * cfeNodePoolCountInUse counts the nodes in use. The list links at +0x1C and +0x20
 * are used by the list helpers in tu144.
 */
typedef struct CfePoolNode {
    int flags;
    unsigned char unmodeled_04[0x2C];
} CfePoolNode;

#define CFE_POOL_NODE_COUNT 6144

extern CfePoolNode *cfeNodePool;

/* Returns the last node of the chain that starts at node, or 0 for a null node. */
CfeListNode *cfeListTail(CfeListNode *node)
{
    CfeListNode *last = 0;
    CfeListNode *next;
    CfeListNode *current;
    CfeListNode *following;

    if (node != 0) {
        next = node->next;
        last = node;
        if (next != 0) {
            do {
                current = next;
                following = current->next;
                next = following;
            } while (following != 0);
            last = current;
        }
    }
    return last;
}

/* Points the pool at a table of CFE_POOL_NODE_COUNT nodes (allocated by tu141 init). */
void cfeNodePoolSet(CfePoolNode *records) {
    cfeNodePool = records;
}

/* Clears every node of the pool, freeing all of them. */
void cfeNodePoolClear(void) {
    memset(cfeNodePool, 0, CFE_POOL_NODE_COUNT * sizeof(CfePoolNode));
}

INCLUDE_ASM("asm/main/nonmatchings/tu146", cfeNodeAlloc);

/* Frees a pool node: clears its in-use bit (cfeNodeRelease). */
void cfeNodeRelease(CfePoolNode *record) {
    record->flags &= ~1;
}

/* Resets the pool: every node becomes free. */
void cfeNodePoolReset(void) {
    cfeNodePoolClear();
}

/* Number of pool nodes in use (bit 0 set). */
int cfeNodePoolCountInUse(void) {
    CfePoolNode *record;
    int inUseCount;
    int index;
    int flags;

    inUseCount = 0;
    record = cfeNodePool;
    for (index = 0; index < CFE_POOL_NODE_COUNT; index++) {
        flags = record->flags;
        record++;
        inUseCount += flags & 1;
    }
    return inUseCount;
}
