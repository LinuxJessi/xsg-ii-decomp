#include "common.h"
#include "main/tu026.h"
#include "main/tu092.h"
#include "main/tu097.h"
#include "main/tu101.h"
#include "main/xgl_cd.h"

int xglCdGetFileSize();
void *xglCdBufferReserve();
extern unsigned char D_0068B200[];
extern unsigned char D_0069D8A0[];

/* Allocates a buffer sized for the named CD file. The size comes from the
 * directory lookup xglCdGetFileSize; a zero size asserts "size" in ../ysegf.euc.c. */
void *ysegfAllocFileBuffer(const char *name) {
    int fileSize;

    fileSize = xglCdGetFileSize(name);
    if (fileSize == 0) {
        assertFailed(D_0069D8A0, D_0068B200, 0x123);
    }
    return xglCdBufferReserve(fileSize);
}

void *ysegfAllocFileBuffer(const char *name);

/* Reads a CD file into *buffer, allocating the buffer first when allocate is set. */
void ysegfReadFile(void **buffer, const char *name, int allocate, int mode) {
    if (allocate != 0) {
        *buffer = ysegfAllocFileBuffer(name);
    }
    xglCdReadFile(name, *buffer, mode, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/ysegf", ysegfLoadTreasure);

extern signed char ysegfSelectedIndex;
extern signed char D_0069E289;

/* Clears the two signed cursor bytes of the item page. */
void ysegfResetSelection(void)
{
    ysegfSelectedIndex = 0;
    D_0069E289 = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/ysegf", ysegfInit);

extern int ysegfDoorImage;
extern int ysegfItemImage;
extern int ysegfDoorAlpha;
extern int ysegfItemAlpha;

/* Releases the scene buffers and clears the four image pointers. */
void ysegfReleaseBuffers(void) {
    xglCdBufferRewind();
    ysegfItemImage = 0;
    ysegfItemAlpha = 0;
    ysegfDoorImage = 0;
    ysegfDoorAlpha = 0;
}


/* Draws a pair of sprites at (x, y); the second pair is drawn only when flag is set. */
void ysegfDrawFlagIcon(int x, int y, int flag) {
    packetSubmitRequest(0x253, x, y, 0);
    packetSubmitRequest(0x256, x, y, 1);
    if (flag != 0) {
        packetSubmitRequest(0x266, x + 0x17, y + 0x18, 0);
        packetSubmitRequest(0x267, x + 0x10, y + 0x10, 1);
    }
}


/* Draws a window frame: four corner sprites and two edge strips (sprite 0x236). */
void ysegfDrawWindow(int x, int y, int width, int height)
{
    int right;
    int bottom;

    packetSubmitRequest(0x237, x, y, 0);
    right = (x + width) - 0x10;
    bottom = (y + height) - 0x10;
    packetSubmitRequest(0x238, right, y, 0);
    packetSubmitRequest(0x234, x, bottom, 0);
    packetSubmitRequest(0x235, right, bottom, 0);
    packetSubmitRequestEx(0x236, x, y + 0x10, width, height - 0x20, 0x100);
    packetSubmitRequestEx(0x236, x + 0x10, y, width - 0x20, height, 0x100);
}

/* Draws a panel frame of eight pieces (sprites 0x239-0x240) around x, y, width, height. */
INCLUDE_ASM("asm/main/nonmatchings/ysegf", ysegfDrawPanel);

/* Same frame as ysegfDrawPanel with the sprite set 0x272-0x279. */
INCLUDE_ASM("asm/main/nonmatchings/ysegf", func_002327E8);

/* Draws the two footer sprites 0x270 and 0x271 at (0, 0x170). */
void ysegfDrawFooter(void) {
    packetSubmitRequest(0x271, 0, 0x170, 1);
    packetSubmitRequest(0x270, 0, 0x170, 1);
}

/* Draws the treasure detail page for the record at index (stride 0xA9 in ysegfTreasureTable). */
INCLUDE_ASM("asm/main/nonmatchings/ysegf", ysegfDrawTreasureDetail);

/* Draws the treasure list: eighteen rows, each a detail page, then flushes the font. */
INCLUDE_ASM("asm/main/nonmatchings/ysegf", ysegfDrawTreasureList);

INCLUDE_ASM("asm/main/nonmatchings/ysegf", func_00232F40);

/* Draws the item explanation page and loads the item image for the selected entry. */
INCLUDE_ASM("asm/main/nonmatchings/ysegf", ysegfDrawItemPage);

/* Scene loop of the 201_menu screen: init, poll pad, run frames until the exit flag is set. */
INCLUDE_ASM("asm/main/nonmatchings/ysegf", ysegfRunMenu);

extern unsigned char ysegfEntryPos[];

/* Resets the eighteen entry positions to 0xC8. */
void ysegfResetEntryPos(void)
{
    int offset;
    int *slot;
    int index;

    offset = 0x44;
    slot = (int *) (ysegfEntryPos + offset);
    for (index = 0; index < 18; index++) {
        *slot = 0xC8;
        slot--;
    }
}

/* Draws the header row of eleven sprites (0x241-0x24B). */
void ysegfDrawListHeader(void) {
    packetSubmitRequest(0x241, 0x3B, 0xD, 1);
    packetSubmitRequest(0x242, 0x5E, 0xD, 1);
    packetSubmitRequest(0x243, 0x81, 0xD, 1);
    packetSubmitRequest(0x244, 0xA4, 0xD, 1);
    packetSubmitRequest(0x245, 0xC7, 0xD, 1);
    packetSubmitRequest(0x246, 0xEA, 0xD, 1);
    packetSubmitRequest(0x247, 0x10D, 0xD, 1);
    packetSubmitRequest(0x248, 0x14F, 0xD, 1);
    packetSubmitRequest(0x249, 0x172, 0xD, 1);
    packetSubmitRequest(0x24A, 0x195, 0xD, 1);
    packetSubmitRequest(0x24B, 0x1B8, 0xD, 1);
}

void msgRangeDrawRevealed(void *data, int count, int id);
void ysegfDrawFooter(void);
void ysegfDrawTreasureList();
void ysegfDrawListHeader(void);
extern unsigned char D_006ABCC8[];

/* List page enter/draw: header, treasure list, footer, then the list object. */
void ysegfDrawList(void) {
    fontObjResetAndBuild();
    ysegfDrawListHeader();
    ysegfDrawTreasureList();
    ysegfDrawFooter();
    msgRangeDrawRevealed(D_006ABCC8, 0x18, 0x175);
}

/* Update of the detail page: lerps the entry positions, L1/R1 steps the selection. */
INCLUDE_ASM("asm/main/nonmatchings/ysegf", ysegfUpdatePage);

void func_00233A98(void)
{
}

void fadeStartIn(int value);

void ysegfStartFadeIn(void) {
    fadeStartIn(0xA);
}

void ysegfDrawTreasureDetail();
void ysegfDrawItemPage();

/* Detail page draw: treasure detail, item explanation, footer, list object. */
void ysegfDrawDetail(void) {
    fontObjResetAndBuild();
    ysegfDrawTreasureDetail(0xC, 0xF, ysegfSelectedIndex, ysegfSelectedIndex, 0x80);
    ysegfDrawItemPage();
    ysegfDrawFooter();
    msgRangeDrawRevealed(D_006ABCC8, 0x18, 0x175);
}

/* Returns the nearest entry below the given index whose flag (0x1C1 + i) is set, wrapping. */
INCLUDE_ASM("asm/main/nonmatchings/ysegf", ysegfFindPrevEntry);

/* Returns the nearest entry above the given index whose flag (0x1C1 + i) is set, wrapping. */
INCLUDE_ASM("asm/main/nonmatchings/ysegf", ysegfFindNextEntry);

/* List page update: pad cancel leaves the page, up/down move the selection. */
INCLUDE_ASM("asm/main/nonmatchings/ysegf", ysegfUpdateList);
