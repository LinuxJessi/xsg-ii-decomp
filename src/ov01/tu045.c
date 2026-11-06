/*
 * OV01 original TU 45: 0x00a9ee28..0x00a9f0a8 (8 functions)
 *
 * Each function is the handler of one SB bytecode opcode in the dispatch table
 * sbOpHandlers (entries 0x26..0x2D; the dispatcher is sbStep in tu044).
 * An opcode takes no operand. It pushes a constant on the VM stack
 * (sbStackPush) and moves the stream position past the opcode. The position
 * is the first word of the VM record, which the dispatcher fetches from; the
 * opcode advances it by its length (sbOpLength; one 16-bit unit here).
 */
#include "common.h"

void sbStackPush(int *record, int value);
int sbOpLength(int opcode, int flags);

/* Opcode 0x26: pushes 0. */
int sbPushConst0(int *record, int opcode) {
    unsigned short opcode16;

    opcode16 = (unsigned short) opcode;
    sbStackPush(record, 0);
    *record += sbOpLength(opcode16, 0);
    return 0;
}

/* Opcode 0x27: pushes 0, the same body as opcode 0x26. */
int func_00A9EE78(int *record, int opcode) {
    unsigned short opcode16;

    opcode16 = (unsigned short) opcode;
    sbStackPush(record, 0);
    *record += sbOpLength(opcode16, 0);
    return 0;
}

/* Opcode 0x28: pushes 1. */
int sbPushConst1(int *record, int opcode) {
    unsigned short opcode16;

    opcode16 = (unsigned short) opcode;
    sbStackPush(record, 1);
    *record += sbOpLength(opcode16, 0);
    return 0;
}

/* Opcode 0x29: pushes 2. */
int sbPushConst2(int *record, int opcode) {
    unsigned short opcode16;

    opcode16 = (unsigned short) opcode;
    sbStackPush(record, 2);
    *record += sbOpLength(opcode16, 0);
    return 0;
}

/* Opcode 0x2A: pushes 3. */
int sbPushConst3(int *record, int opcode) {
    unsigned short opcode16;

    opcode16 = (unsigned short) opcode;
    sbStackPush(record, 3);
    *record += sbOpLength(opcode16, 0);
    return 0;
}

/* Opcode 0x2B: pushes 4. */
int sbPushConst4(int *record, int opcode) {
    unsigned short opcode16;

    opcode16 = (unsigned short) opcode;
    sbStackPush(record, 4);
    *record += sbOpLength(opcode16, 0);
    return 0;
}

/* Opcode 0x2C: pushes 5. */
int sbPushConst5(int *record, int opcode) {
    unsigned short opcode16;

    opcode16 = (unsigned short) opcode;
    sbStackPush(record, 5);
    *record += sbOpLength(opcode16, 0);
    return 0;
}

/* Opcode 0x2D: pushes 6. */
int sbPushConst6(int *record, int opcode) {
    unsigned short opcode16;

    opcode16 = (unsigned short) opcode;
    sbStackPush(record, 6);
    *record += sbOpLength(opcode16, 0);
    return 0;
}
