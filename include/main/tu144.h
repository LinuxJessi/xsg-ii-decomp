#ifndef INCLUDE_MAIN_TU144_H
#define INCLUDE_MAIN_TU144_H

/* The key lists of src/main/tu144.c and the functions other files call. */

/*
 * A node of a doubly linked list whose entries carry a sort key at +0x04 (the
 * list helpers compare it). `prev` (+0x1C) and `next` (+0x20) are the
 * neighbours the list helpers store; the bytes between are not modeled.
 */
typedef struct CfeListNode {
    int flags;                       /* 0x00 */
    int key;                         /* 0x04 */
    int value;                       /* 0x08: 8.8 fixed point */
    unsigned char unmodeled_0c[0x10];
    struct CfeListNode *prev;        /* 0x1C */
    struct CfeListNode *next;        /* 0x20 */
} CfeListNode;

/* An 8.8 fixed-point key value as a float. */
#define CFE_FIXED_TO_FLOAT(value) ((float) ((value) / 256) + (float) ((value) % 256) * (1.0f / 256))

CfeListNode *func_0024F698(CfeListNode **list, int key, float value);
CfeListNode *func_0024F730(CfeListNode **list, int key, float value);
CfeListNode *func_0024F7C0(CfeListNode **list, int key, float value);
CfeListNode *func_0024F860(CfeListNode **list, int key, float value);

#endif /* INCLUDE_MAIN_TU144_H */
