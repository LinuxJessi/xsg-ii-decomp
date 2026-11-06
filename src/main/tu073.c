#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu073", motionFace);

INCLUDE_ASM("asm/main/nonmatchings/tu073", enepc_001F5F20);

INCLUDE_ASM("asm/main/nonmatchings/tu073", enepc_001F5FE0);

INCLUDE_ASM("asm/main/nonmatchings/tu073", enepc_001F60B8);

int uwamonoFindIndexByShortKey(short);
void func_00205FD8(int, int, int);
void func_A9CC70(int, int);
int func_A9F0A8(void);
int func_A9F0C0(int, int);

/*
 * Returns 0 after func_A9CC70 when func_A9F0A8 does not report 2. Otherwise
 * passes func_A9F0C0's values for the id to func_00205FD8 and returns 1.
 */
int enepc_001F6420(int id)
{
  int firstArg;
  int convertedArg;

  if (func_A9F0A8() != 2) {
    func_A9CC70(id, 2);
    return 0;
  }
  /* script argument 1 of this id, converted by uwamonoFindIndexByShortKey */
  firstArg = func_A9F0C0(id, 1);
  convertedArg = uwamonoFindIndexByShortKey(firstArg);
  func_00205FD8(convertedArg, 1, func_A9F0C0(id, 2));
  return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu073", Resigne);

INCLUDE_ASM("asm/main/nonmatchings/tu073", setGroup);

INCLUDE_ASM("asm/main/nonmatchings/tu073", enepc_001F6680);

INCLUDE_ASM("asm/main/nonmatchings/tu073", enepc_001F6728);

INCLUDE_ASM("asm/main/nonmatchings/tu073", enepc_001F6800);

INCLUDE_ASM("asm/main/nonmatchings/tu073", enepc_001F68D0);

INCLUDE_ASM("asm/main/nonmatchings/tu073", enepc_001F6A60);

INCLUDE_ASM("asm/main/nonmatchings/tu073", enepc_001F6BB8);

INCLUDE_ASM("asm/main/nonmatchings/tu073", enepc_001F6C38);
