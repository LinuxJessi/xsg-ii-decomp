#ifndef INCLUDE_MAIN_RES_GET_PATH_H
#define INCLUDE_MAIN_RES_GET_PATH_H

/* Functions of src/main/res_get_path.c that other files call. */

/* One 0x14-byte entry of the block table xglCdSlotTable, indexed by slot. The table
 * describes memory blocks in address order; CD reads and other allocations (e.g. the pool
 * cfeNodePoolInit takes through GameResBufferAlloc) share it:
 *   buffer: block start; 0 ends the table (every search stops there).
 *   size:   block bytes (GameResBlockAssign splits, GameResBlockFree merges by adding sizes).
 *   owner:  0 when free; a resource id, or GAME_RES_OWNER plus bits 8-23 of the buffer
 *           address for a block allocated without an owner (GameResBlockAlloc with owner 0,
 *           named loads and anonymous allocations alike). Entry 0's owner is the first
 *           index the best-fit search GameResBlockFindBestFit starts at.
 *   tag:    completion callback of a read into the block, 0 when idle.
 *   value:  word handed back to that callback (copied from xglCdSlotValue). */
typedef struct CdSlot {
    void *buffer;
    unsigned int size;
    int owner;
    void *tag;
    void *value;
} CdSlot;

/* Entries of xglCdSlotTable; every search walks indexes 1 .. GAME_RES_BLOCK_COUNT - 1. */
#define GAME_RES_BLOCK_COUNT 0x80

extern CdSlot xglCdSlotTable[];

void resInitPathHeader(int key, int value);
void *GameResBufferAlloc(int size);

#endif /* INCLUDE_MAIN_RES_GET_PATH_H */
