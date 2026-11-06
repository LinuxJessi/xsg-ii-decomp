#include "common.h"
#include "main/res_get_path.h"
#include "main/test_128.h"

/* Source file name passed with the allocation and free calls. */
extern char D_0068D5E8[];
extern int oLibHandle;
int InstanceOfRgHeapById(int count);
int RgHeapAllocById(int count, int flags, char *file, int line);
void resResetSlotTable(void);
void RgHeapFree(int handle, char *file, int line);

/* Object library setup: the handle for the oLib file goes into oLibHandle. */
void oLibInit(void)
{
    int setup_handle;
    int handle;

    setup_handle = InstanceOfRgHeapById(4);
    InitRgHeap(setup_handle, 0x01000000, 0x01200000);
    handle = RgHeapAllocById(4, 0x800010, D_0068D5E8, 118);
    oLibHandle = handle;
    resInitPathHeader(handle, 0x800000);
}

/* Object library teardown: frees the handle in oLibHandle. */
void oLibExit(void)
{
    resResetSlotTable();
    RgHeapFree(oLibHandle, D_0068D5E8, 134);
}

/* Loads a game resource by Id (asserted at line 157) and initialises a new object from its header. */
INCLUDE_ASM("asm/main/nonmatchings/oLib", oLibLoadObject);
