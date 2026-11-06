#include "common.h"
#include "shared.h"
#include "main/tu075.h"

void func_A9CDB0(void *, void *);
extern unsigned char uwamonoNativeTable[];
extern unsigned char D_0069D310[];

/* Registers the uwamono native table uwamonoNativeTable with the module name D_0069D310. */
void uwamonoRegisterNatives(void) {
    func_A9CDB0(D_0069D310, uwamonoNativeTable);
}

int unduCheckAngle(f32, f32, f32);
extern f32 D_0069AF84;
extern f32 D_0069AF88;
extern f32 D_0069AF8C;
extern f32 D_0069AF90;

/*
 * Tests angle against four bounds loaded in the order D_0069AF84,
 * D_0069AF88, D_0069AF8C, D_0069AF90. Each test passes an adjacent pair of
 * bounds. Returns the index of the first test that holds, or -1 when none.
 * The four bounds are -3pi/4, 3pi/4, pi/4 and -pi/4, so the result is one of
 * four 90-degree compass sectors (printed as "Compass = %1d" by uwamonoLadderMove).
 */
int uwamonoCompassSector(f32 angle)
{
    f32 sectorEdge0;
    f32 sectorEdge1;
    f32 sectorEdge2;
    f32 sectorEdge3;

    sectorEdge0 = D_0069AF84;
    sectorEdge1 = D_0069AF88;
    if (unduCheckAngle(angle, sectorEdge1, sectorEdge0) != 0) {
        return 0;
    }
    sectorEdge2 = D_0069AF8C;
    if (unduCheckAngle(angle, sectorEdge2, sectorEdge1) != 0) {
        return 1;
    }
    sectorEdge3 = D_0069AF90;
    if (unduCheckAngle(angle, sectorEdge3, sectorEdge2) != 0) {
        return 2;
    }
    return (unduCheckAngle(angle, sectorEdge0, sectorEdge3) != 0) ? 3 : -1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu075", uwamonoLadderApproach);

INCLUDE_ASM("asm/main/nonmatchings/tu075", uwamonoParabolaMove);

INCLUDE_ASM("asm/main/nonmatchings/tu075", func_001F8608);

INCLUDE_ASM("asm/main/nonmatchings/tu075", uwamonoLadderMove);

INCLUDE_ASM("asm/main/nonmatchings/tu075", uwamonoUpdateLadderState);
