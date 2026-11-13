#include "common.h"

/*
 * A node of a doubly linked list whose entries carry a sort key at +0x04 (the
 * list helpers below compare it). Only the two link pointers are modeled:
 * `prev` (+0x1C) and `next` (+0x20) are the neighbours the list helpers store,
 * and every other byte is an unmodeled gap.
 */
typedef struct CfeListNode {
    int flags;                       /* 0x00 */
    int key;                         /* 0x04 */
    int value;                       /* 0x08: 8.8 fixed point */
    unsigned char unmodeled_0c[0x10];
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

CfeListNode *cfeNodeAlloc(void);
void cfeNodeRelease(void *node);
void cfeChainUnwrapAngles(CfeListNode *list);
void func_0024FC90(CfeListNode *list);
CfeListNode *cfeListFindOrAdd(CfeListNode **list, int key);

/* Finds the node with key `key` in the sorted list at *list, or allocates and links one (cfeNodeAlloc);
   a found node has its 0x8 and 0x10 flag bits cleared. */
INCLUDE_ASM("asm/main/nonmatchings/tu144", cfeListFindOrAdd);

/* Sets the 8.8 fixed-point value of the key's node and clears flag 0x2, then updates the list. */
CfeListNode *func_0024F698(CfeListNode **list, int key, float value)
{
    CfeListNode *node = cfeListFindOrAdd(list, key);
    int whole;

    node->flags &= ~2;
    whole = (int) value;
    node->value = (whole << 8) + (int) ((value - (float) whole) * 256.0f);
    func_0024FC90(*list);
    return node;
}

/* Same as func_0024F698 but sets flag 0x2. */
CfeListNode *func_0024F730(CfeListNode **list, int key, float value)
{
    CfeListNode *node = cfeListFindOrAdd(list, key);
    int whole = (int) value;

    node->flags |= 2;
    node->value = (whole << 8) + (int) ((value - (float) whole) * 256.0f);
    func_0024FC90(*list);
    return node;
}

/* Like func_0024F698 for an angle key: also unwraps the angles along the list. */
CfeListNode *func_0024F7C0(CfeListNode **list, int key, float value)
{
    CfeListNode *node = cfeListFindOrAdd(list, key);
    int whole;

    node->flags &= ~2;
    whole = (int) value;
    node->value = (whole << 8) + (int) ((value - (float) whole) * 256.0f);
    cfeChainUnwrapAngles(*list);
    func_0024FC90(*list);
    return node;
}

/* Same as func_0024F7C0 but sets flag 0x2. */
CfeListNode *func_0024F860(CfeListNode **list, int key, float value)
{
    CfeListNode *node = cfeListFindOrAdd(list, key);
    int whole = (int) value;

    node->flags |= 2;
    node->value = (whole << 8) + (int) ((value - (float) whole) * 256.0f);
    cfeChainUnwrapAngles(*list);
    func_0024FC90(*list);
    return node;
}

INCLUDE_ASM("asm/main/nonmatchings/tu144", func_0024F8F8);

/* Removes the first node whose key is `key` from the list starting at `head`, frees it (cfeNodeRelease) and
   returns the list head (the next node when the head itself was the match). */
CfeListNode *cfeListRemoveKey(CfeListNode *head, int key)
{
    CfeListNode *prev = 0;
    CfeListNode *node = head;
    CfeListNode *next;

    if (head == 0) {
        return 0;
    }
    if (head->key == key) {
        next = head->next;
        cfeListUnlink(0, head, next);
        cfeNodeRelease(head);
        return next;
    }
    while (node != 0) {
        next = node->next;
        if (node->key == key) {
            cfeListUnlink(prev, node, next);
            cfeNodeRelease(node);
            return head;
        }
        prev = node;
        node = next;
    }
    return head;
}


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
