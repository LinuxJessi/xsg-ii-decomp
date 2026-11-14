#include "common.h"
#include "main/tu144.h"


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

/* Node flag bits that finding a node clears (their meaning is not recovered). */
#define CFE_NODE_FLAG_8 0x8
#define CFE_NODE_FLAG_10 0x10

/* Finds the node with key `key` in the sorted list at *list, or allocates one (cfeNodeAlloc) and
 * links it in key order; a found node has its flag bits 0x8 and 0x10 cleared. */
CfeListNode *cfeListFindOrAdd(CfeListNode **list, int key)
{
    CfeListNode **link;
    CfeListNode *prev;
    CfeListNode *node;
    CfeListNode *first;

    if (*list != 0) {
        prev = 0;
        link = list;
        do {
            if (prev == 0 && key < (*link)->key) {
                node = cfeNodeAlloc();
                if (node == 0) {
                    return 0;
                }
                node->key = key;
                cfeListLink(prev, node, *link);
                *list = node;
                return node;
            }
            if ((*link)->key == key) {
                (*link)->flags &= ~CFE_NODE_FLAG_8;
                (*link)->flags &= ~CFE_NODE_FLAG_10;
                return *link;
            }
            if ((*link)->next != 0 && (*link)->key < key && key < (*link)->next->key) {
                node = cfeNodeAlloc();
                if (node == 0) {
                    return 0;
                }
                node->key = key;
                cfeListLink(*link, node, (*link)->next);
                return node;
            }
            prev = *link;
            link = &prev->next;
        } while (*link != 0);
        node = cfeNodeAlloc();
        if (node == 0) {
            return 0;
        }
        node->key = key;
        cfeListLink(prev, node, 0);
        return node;
    }
    first = cfeNodeAlloc();
    *list = first;
    if (first != 0) {
        first->key = key;
    }
    return first;
}

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

/* Copies the flags and the value of `source` to the node of `key` (found or added), then updates the list. */
void func_0024F8F8(CfeListNode **list, CfeListNode *source, int key)
{
    CfeListNode *node = cfeListFindOrAdd(list, key);

    node->flags = source->flags;
    node->value = source->value;
    func_0024FC90(*list);
}

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
