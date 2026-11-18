#ifndef INCLUDE_OV01_TU062_H
#define INCLUDE_OV01_TU062_H

/* Functions of src/ov01/tu062.c that other files call. */
void menuPanelCreate(void);
/* Opens a stat panel of kind. */
void menuStatPanelNew(int kind);

/* menuStatPanelNew kinds func_00A94260 opens for a hit (rows of menuStatRects). */
enum {
    MENU_STAT_PANEL_NO_HIT = 1,         /* hit kind 0 */
    MENU_STAT_PANEL_BLOCKED = 2,        /* hitMode bit 9 */
    MENU_STAT_PANEL_TAKEN = 3,          /* hitMode bit 8 */
    MENU_STAT_PANEL_TAKEN_BONUS = 4,    /* hitMode bit 8 after calcCritDamage's bonus roll */
    MENU_STAT_PANEL_BACK_ATTACK = 6     /* HitRecord.backAttack */
};
/* Opens the stat panel of a step state (1 to 3, ActStepCounter.stepState). */
void func_00AA76F8(int stepState);

struct GrPacket;
struct Ov01Object;

/* Draws the face of the unit with key `key`; small picks the smaller of the two frames. */
void func_00AA7750(struct GrPacket *packet, int x, int y, short key, int small);
/* A sprite of width x height at (x, y) from the texel rectangle at (srcX, srcY) of the same size. */
int menuStatSprite(struct GrPacket *packet, int x, int y, int width, int height, int srcX, int srcY);
/* A sprite of width x height at (x, y) from the texel rectangle (srcX, srcY)-(srcRight, srcBottom), which the
 * gauges keep at full size while the width shrinks. */
void menuStatSprite2(struct GrPacket *packet, int x, int y, int width, int height, int srcX, int srcY, int srcRight,
                     int srcBottom);
void menuNumDraw(struct GrPacket *packet, int x, int y, int value, int digits, int style);
void menuNumDrawTarget(struct GrPacket *packet, int x, int y, int value, int digits, int style,
                       struct Ov01Object *target);
void func_00AA7A48(struct GrPacket *packet, int x, int y, int value, int digits, int style, struct Ov01Object *target);
/* One 18 x 18 icon from the icon atlas, cell (col, row). */
void menuIconPut(struct GrPacket *packet, int x, int y, int col, int row);
void menuStatusIconPut(struct GrPacket *packet, int x, int y);
/* The status icons of a tec record (tecRecordGet). */
void menuStatusIconsDraw(struct GrPacket *packet, int x, int y, int record);

#endif /* INCLUDE_OV01_TU062_H */
