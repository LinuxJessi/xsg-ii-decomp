/*
 * OV01 original TU 55: 0x00aa5248..0x00aa5390 (3 functions)
 */
#include "common.h"
#include "ov01/tu005.h"

typedef struct Ov01Object {
    unsigned char unmodeled_00[0x04];
    void (*callback_04)(void);
    void (*callback_08)(void);
    unsigned char unmodeled_0c[0x04];
    float x;
    float y;
    int state;
    unsigned char unmodeled_1c[0x74];
    int counter;
    int busy;
} Ov01Object;

void *objSlotAlloc(void);
void func_00AA52D0(void);
void func_00AA52D8(void);
extern void *D_00AE0EC0;

void func_00AA5248(void)
{
    Ov01Object *object;

    object = objSlotAlloc();
    if (object != 0) {
        object->callback_04 = func_00AA52D0;
        object->callback_08 = func_00AA52D8;
        D_00AE0EC0 = object;
        ov01MatrixBlockInit(&object->x);
        object->x = 348.0f;
        object->y = 386.0f;
        do {
            object->busy = 0;
            object->state = (object->counter = 0);
        } while (0);
    }
}

void func_00AA52D0(void) {

}

INCLUDE_ASM("asm/nonmatchings/ov01/tu055", func_00AA52D8);
