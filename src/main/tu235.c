#include "common.h"
#include "main/tu243.h"

typedef signed char s8;

typedef int s32;

/*
 * Partial view of the dispatch body (struct DispatchBody in main/tu219.c, the body at slot +0x08):
 * only the words dispatchBodyInitKind2 sets are named here. Its kind is at +0x1C (msgSlotInit stores it).
 */
typedef struct DispatchBody {
    unsigned char unmodeled_00[0x50];
    s32 word_50;
    s32 word_54;
} DispatchBody;

void msgSlotInit();

/*
 * Initialises the body as a kind 2 dispatch body: msgSlotInit(body, 2) clears it and stores the kind
 * (main/tu219 calls func_00284338 for kind 2), then sets word_50 = 0x10 and word_54 = 3.
 */
void dispatchBodyInitKind2(DispatchBody *body) {
    msgSlotInit(body, 2);
    body->word_50 = 0x10;
    body->word_54 = 3;
}

INCLUDE_ASM("asm/main/nonmatchings/tu235", func_00284338);


/* Forwards its two arguments to func_00286ED0 with a fixed third argument of 0x1E. Callee still unnamed. */
s32 func_002843B8(s32 first, s32 second) {
    return func_00286ED0(first, second, 0x1E);
}
