#include "common.h"

/*
 * A node of a doubly linked list whose entries carry a sort key at +0x04 (the
 * list helpers below compare it). Only the two link pointers are modeled:
 * `prev` (+0x1C) and `next` (+0x20) are the neighbours the list helpers store,
 * and every other byte is an unmodeled gap.
 */
typedef struct CfeListNode {
    unsigned char unmodeled_00[0x1C];
    struct CfeListNode *prev;        /* 0x1C */
    struct CfeListNode *next;        /* 0x20 */
} CfeListNode;

/* Links `node` between `prev` and `next`; either neighbour may be null. */
void cfeListLink(CfeListNode *prev, CfeListNode *node, CfeListNode *next) {
    if (prev != 0) {
        prev->next = node;
    }
    node->prev = prev;
    node->next = next;
    if (next != 0) {
        next->prev = node;
    }
}

/* Unlinks `node` from between `prev` and `next`; either neighbour may be null. */
void cfeListUnlink(CfeListNode *prev, CfeListNode *node, CfeListNode *next) {
    if (prev != 0) {
        prev->next = next;
    }
    node->prev = 0;
    node->next = 0;
    if (next != 0) {
        next->prev = prev;
    }
}

/* Finds the node with key `key` in the sorted list at *list, or allocates and links one (cfeNodeAlloc);
   a found node has its 0x8 and 0x10 flag bits cleared. */
INCLUDE_ASM("asm/main/nonmatchings/tu144", cfeListFindOrAdd);

/* Sets the 8.8 fixed-point value of a float in node+8 and clears flag 0x2 (via cfeListFindOrAdd). */
INCLUDE_ASM("asm/main/nonmatchings/tu144", func_0024F698);

/* Same as func_0024F698 but sets flag 0x2 (via cfeListFindOrAdd). */
INCLUDE_ASM("asm/main/nonmatchings/tu144", func_0024F730);

INCLUDE_ASM("asm/main/nonmatchings/tu144", func_0024F7C0);

INCLUDE_ASM("asm/main/nonmatchings/tu144", func_0024F860);

INCLUDE_ASM("asm/main/nonmatchings/tu144", func_0024F8F8);

/* Removes the first node whose key is `key` from the list starting at `head`, frees it (cfeNodeRelease) and
   returns the list head (the next node when the head itself was the match). */
INCLUDE_ASM("asm/main/nonmatchings/tu144", cfeListRemoveKey);

void cfeNodeRelease(void *node);

/* Walks the list from `head` along `next`, freeing each node and clearing its links. */
void cfeListFreeAll(CfeListNode *head) {
    CfeListNode *node;
    CfeListNode *next;

    node = head;
    if (node != 0) {
        do {
            next = node->next;
            cfeNodeRelease(node);
            node->prev = 0;
            node->next = 0;
            node = next;
        } while (node != 0);
    }
}
