#include "common.h"
#include "main/sm_initilize.h"

INCLUDE_ASM("asm/main/nonmatchings/sm_initilize", smInitMemInfo);

#include "shared.h"

/*
 * Memory cells of a pool. A cell is 0x40 bytes: its 16-byte header sits in the
 * first unit and the user data follows it (allocation pointer = cell + 0x40).
 * tag: 0x1234 for a free cell, 0x5678 for an allocated one.
 * unit_count: data units that follow the header of this cell.
 * prev/next: neighbouring cells in address order.
 */
typedef struct SmCell {
  int tag;
  int unit_count;
  struct SmCell *prev;
  struct SmCell *next;
} SmCell;

/*
 * A pool, as the global object is laid out: first_cell at 0x00, total_units at
 * 0x04. Bytes 0x08..0x17 hold allocation counters (allocated units, total bytes,
 * allocated bytes, largest free block), filled by smInitMemInfo and not modelled here.
 */
typedef struct SmPool {
  SmCell *first_cell;
  int total_units;
} SmPool;

void smInitMemInfo(SmPool *pool, SmCell *first_cell, u32 unit_count);
extern SmPool smPool;
extern SmPool smPoolSmall;

void smInitilize(SmCell *cell, u32 size)
{
  u32 unit_count;

  unit_count = size >> 6;
  cell->tag = 0x1234;
  cell->unit_count = (int)(unit_count - 1);
  cell->prev = 0;
  cell->next = 0;
  smInitMemInfo(&smPool, cell, unit_count);
}

void smInitilizeSmall(SmCell *cell, u32 size)
{
  u32 unit_count;

  unit_count = size >> 6;
  cell->tag = 0x1234;
  cell->unit_count = (int)(unit_count - 1);
  cell->prev = 0;
  cell->next = 0;
  smInitMemInfo(&smPoolSmall, cell, unit_count);
}

INCLUDE_ASM("asm/main/nonmatchings/sm_initilize", smAllocFromPool);

/* Best-fit search over a pool; mode 0 allocates the cell found, nonzero only returns it. */
void smAllocFromPool(u32 size, SmCell *first_cell, int total_units, int find_only);

void smAlloc(int size)
{
  smAllocFromPool((size + 0x7FF) & ~0x7FF, smPool.first_cell, smPool.total_units, 0);
}

void smFindFree(int size)
{
  smAllocFromPool((size + 0x7FF) & ~0x7FF, smPool.first_cell, smPool.total_units, 1);
}

void smAllocSmall(int size)
{
  smAllocFromPool((size + 0x3F) & ~0x3F, smPoolSmall.first_cell, smPoolSmall.total_units, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/sm_initilize", smFree);

/* Original prototype is smFree(void *allocation); tu178 declares it that way. */
void smFree(void *allocation);

void smFreeHandle(void *handle)
{
  smFree(handle);
}

/* Walks the cell chain from first_cell, stopping at the first cell that is neither free nor allocated. */
void smWalkCells(SmCell *cell)
{
  int tag;

  if (cell != 0)
  {
    for (;;)
    {
      tag = cell->tag;
      if (tag == 0x1234 || tag == 0x5678)
      {
        cell = cell->next;
        if (cell != 0)
        {
          continue;
        }
      }
      break;
    }
  }
}

void smPrintInfo(void)
{
  smWalkCells(smPool.first_cell);
}

void smPrintInfoSmall(void)
{
  smWalkCells(smPoolSmall.first_cell);
}
