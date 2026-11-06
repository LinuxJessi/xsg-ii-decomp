#include "common.h"
#include "main/tu207.h"

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00274808);

/* Inclusive range test: lower <= value <= upper. */
int xglIsInRange(int value, int lower, int upper)
{
  return lower <= value && value <= upper;
}

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00274850);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_002748C8);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00275710);

void recordClear(void *record);
extern unsigned char recordBanks[];

/* Clears the ten 0x7C-byte records of bank bankIndex (banks are 0x4D8 bytes apart). */
void recordClearBank(int bankIndex)
{
    unsigned char *record;
    int slot;

    record = recordBanks + bankIndex * 0x4D8;
    for (slot = 0; slot < 10; slot++) {
        recordClear(record);
        record += 0x7C;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_002757F0);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00275830);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00275968);

extern void *memset(void *destination, int value, unsigned int count);

/* Clears one 0x7C-byte record. */
void recordClear(void *record)
{
  memset(record, 0, 0x7C);
}

/* Flag that suspends the per-frame record updates; 1 while suspended. */
extern int recordUpdatePaused;

void recordPauseUpdates(void)
{
  recordUpdatePaused = 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_002759F8);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00275CF0);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00275D30);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00275E08);

int func_00274808(int, int, int);
extern int recordAngle;

/* At stage 2, while updates are not suspended, steps the angle recordAngle by 92 and wraps it into 0..360. */
void recordStepAngle(int stage)
{
    if (recordUpdatePaused == 0 && stage == 2) {
        recordAngle += 0x5C;
        recordAngle = func_00274808(recordAngle, 0, 0x168);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00275EE8);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00275F70);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00276350);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00276400);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00276458);

extern int recordGridWidth;
extern int recordGridHeight;
extern unsigned char recordGridCells[];

/* Clears the 0x9C4-byte cell grid and sets its width and height (cells across and down). */
void recordGridReset(int width, int height)
{
    memset(recordGridCells, 0, 0x9C4);
    recordGridWidth = width;
    recordGridHeight = height;
}

/* Number of cells in the grid: width * height. */
int recordGridCellCount(void)
{
  return recordGridWidth * recordGridHeight;
}

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00276838);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00276D68);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00277100);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00277168);

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_002772A8);

void func_002775E8(void)
{
}

int func_002775F0(void)
{
  return 0x40808080;
}

int recordDefaultColor(void)
{
  return 0x40808080;
}

INCLUDE_ASM("asm/main/nonmatchings/tu207", func_00277610);
