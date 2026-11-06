#include "common.h"
#include "main/tu096.h"
#include "main/tu097.h"
#include "main/tu101.h"
#include "main/tu102.h"
#include "main/tu104.h"

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawBorder);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawPanelFrame);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021D8B8);


/* Panel frame: border and corner pieces 0x2F-0x35 with the fill in mode 0x106; the tint is set first. */
void menuDrawPanelFrameAlt(int x, int y, int width, int height, int color) {
    int bottomY;
    int innerY;
    int innerX;
    int innerWidth;
    int innerHeight;

    bottomY = (y + height) - 0x10;
    innerY = y + 0x10;
    innerX = x + 0x10;
    innerWidth = width - 0x20;
    fontObjSetColor(color);
    packetSubmitRequest(0x2F, x, bottomY, 6);
    innerHeight = height - 0x20;
    packetSubmitRequestEx(0x30, innerX, bottomY, innerWidth, 0x10, 6);
    packetSubmitRequest(0x31, (x + width) - 0x10, bottomY, 6);
    packetSubmitRequestEx(0x32, x, innerY, 0x10, innerHeight, 6);
    packetSubmitRequestEx(0x33, innerX, innerY, innerWidth, innerHeight, 0x106);
    packetSubmitRequestEx(0x32, (x + width) - 0x10, innerY, 0x10, innerHeight, 6);
    packetSubmitRequest(0x34, x, y, 6);
    packetSubmitRequestEx(0x30, innerX, y, innerWidth, 0x10, 6);
    packetSubmitRequest(0x35, (x + width) - 0x10, y, 6);
    fontObjResetColor();
}

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021DB98);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawPanel);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021DDB8);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021DE60);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021E338);

void menuDrawOutlineFrame(int x, int y, int width, int height) {
    int rightX;
    int bottomY;
    int innerY;
    int innerHeight;
    int innerX;
    int innerWidth;

    rightX = (x + width) - 0x10;
    bottomY = (y + height) - 0x10;
    innerY = y + 0x10;
    innerHeight = height - 0x20;
    innerX = x + 0x10;
    packetSubmitRequest(0x63, x, bottomY, 0);
    packetSubmitRequestEx(0x64, x, innerY, 0x10, innerHeight, 0);
    innerWidth = width - 0x20;
    packetSubmitRequest(0x65, x, y, 0);
    packetSubmitRequestEx(0x66, innerX, y, innerWidth, 0x10, 0);
    packetSubmitRequest(0x67, rightX, y, 0);
    packetSubmitRequest(0x5D, x, bottomY, 1);
    packetSubmitRequestEx(0x5E, innerX, bottomY, innerWidth, 0x10, 1);
    packetSubmitRequest(0x5F, rightX, bottomY, 1);
    packetSubmitRequestEx(0x60, rightX, innerY, 0x10, innerHeight, 1);
    packetSubmitRequest(0x61, rightX, y, 1);
}

/* Split frame: outer pieces 0x4D-0x59, the 0x62 mark at split and 0x50 bars on both sides of it. */
void menuDrawSplitFrame(int x, int y, int width, int height, int split) {
    int bottom;
    int top_inner;
    int split_x;
    int right;
    int left_inner;

    bottom = (y + height) - 0x10;
    top_inner = y + 0x11;
    split_x = x + split;
    right = (x + width) - 0x10;
    left_inner = x + 0x10;
    packetSubmitRequest(0x55, x, bottom, 0);
    packetSubmitRequestEx(0x56, left_inner, bottom, width - 0x20, 0x10, 0);
    packetSubmitRequest(0x57, right, bottom, 0);
    packetSubmitRequestEx(0x58, right, y + 0x21, 0x10, height - 0x31, 0);
    packetSubmitRequest(0x59, right, top_inner, 0);
    packetSubmitRequest(0x4D, x, bottom, 1);
    packetSubmitRequestEx(0x4E, x, y + 0x10, 0x10, height - 0x20, 1);
    packetSubmitRequest(0x4F, x, y, 1);
    packetSubmitRequest(0x51, right, top_inner, 1);
    packetSubmitRequest(0x62, split_x, y, 1);
    packetSubmitRequestEx(0x50, left_inner, y, split - 0x10, 0x10, 1);
    packetSubmitRequestEx(0x50, split_x + 0x20, top_inner, (width - split) - 0x30, 0x10, 1);
}

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021E7B0);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021EE68);

#include "shared.h"
#include "tu104.h"

void func_0020CD50(void *);
void fontTrackSetBoth(void *, int, int);
void fontObjDraw(void *);
void fontObjSetColorRgba(int, int, int, int);
void textDrawDigits(int, int, int, int);
void func_0021CC08(int, int, int, int);
void func_0021F658(int, int, int, int, int);
extern unsigned short menuIconKinds[];
extern int menuCursorPos[];
extern int menuCursorSize[];
extern Tu104Request packetRequest;

/* Icon at (x, y): the backing piece 0x157, then the kind stored in menuIconKinds[index]. */
void menuDrawIndexedIcon(int x, int y, int index)
{
    unsigned short iconKind = menuIconKinds[index];

    packetSubmitRequest(0x157, x, y, 0x10);
    packetSubmitRequest(iconKind, x, y, 0x11);
}

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021EFC8);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021F0A0);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021F110);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021F190);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021F218);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021F2F0);

/* Cursor box: position from menuCursorPos, size from menuCursorSize, kind 0x82 with mode 0x105. */
void menuDrawCursor(void)
{
    func_0020CD50(menuCursorPos);
    func_0020CD50(menuCursorSize);
    func_0020CD50(menuCursorPos);
    func_0020CD50(menuCursorSize);
    fontObjSetColorRgba(0x24, 0, 0x48, 0x80);
    packetSubmitRequestEx(0x82, menuCursorPos[0], menuCursorPos[1], menuCursorSize[0], menuCursorSize[1], 0x105);
    fontObjResetColor();
}

/* Select arrows 0x164 and 0x165 beside a row; their bob offset comes from xglTickGet. */
void menuDrawSelectArrows(int x, int width, int y)
{
    int offset = (xglTickGet() & 0xF) >> 1;
    int row = y + 0x3C;

    packetSubmitRequest(0x164, x - offset - 8, row, 1);
    packetSubmitRequest(0x165, x + width + offset - 8, row, 1);
}

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021F530);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021F658);

/* List window: a 0xE0 x 0x11F panel; with two or more entries it adds the 0x1C mark and the rows (func_0021F658). */
void menuDrawListWindow(int x, int y, int layer, int count)
{
    func_0021CC08(x, y, 0xE0, 0x11F);
    if (count >= 2) {
        packetSubmitRequest(0x1C, x + 0x94, y + 0x10C, 0x40);
        textDrawDigits(x + 0xC0, y + 0x10C, layer + 1, 2);
        func_0021F658(x + 0xC, y + 0x101, layer, count, 8);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_0021F8A8);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawCharInfo);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawItemList);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawItemListDetail);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuListContains);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00220210);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00220550);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00220840);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawCharStats);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawEquipPanel);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawItemCategory);

void menuDrawKindIcon(int x, int y, int index)
{
    fontObjSetColor(0x3F80000080000000LL);
    packetSubmitRequest(0x44, x, y, 4);
    fontObjResetColor();
    packetSubmitRequest(index + 0x47, x, y, 1);
}

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawAttackCommand);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawSpecialAttack);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawCommandInfo);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawPartnerInfo);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00222860);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawCopilotInfo);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawClassInfo);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawLicenseBoard);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawConfig);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawControllerConfig);

INCLUDE_ASM("asm/main/nonmatchings/tu104", menuDrawPlayTime);

void menuResetRecord(Tu104Record *record)
{
    if (record != 0) {
        record->status = -1;
        fontTrackSetBoth(record->first_block, 0, 0);
        fontTrackSetBoth(record->second_block, 0, 0);
        record->pending = 0;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_002245B8);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00224608);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_002246A0);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00224740);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00224810);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00224928);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00224A28);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00224B40);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00224C40);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00224CF8);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00224E20);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00224F20);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00225048);

void menuDrawAnimatedBand(int first, int second)
{
    packetRequest.first = first;
    packetRequest.second = second;
    packetRequest.length = 0x134;
    packetRequest.phase = (xglTickGet() & 7) * 0x10;
    packetRequest.kind = 0x21;
    packetRequest.reset_value = 0;
    fontObjDraw(&packetRequest);
}

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_002251A0);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_002253A8);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00225460);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00225FE0);

INCLUDE_ASM("asm/main/nonmatchings/tu104", func_00226140);
