#include "common.h"
#include "main/TiDrawCollision.h"
#include "main/tu026.h"
#include "main/xgl_2_031.h"

INCLUDE_ASM("asm/main/nonmatchings/TiDrawCollision", tiDrawCollisionBuildRing);

void func_00295780(int count, void *from, void *to);
void tiDrawJointTransformPoints(unsigned int count, float matrix[4][4], void *first, void *second);
void tiDrawJointPackPoints(int count, void *from, void *collision, int flag);
/* 48 Vector4 points: the ring that tiDrawCollisionBuildRing builds at the given scale. */
extern char tiDrawCollisionRing[];
/* First transform pass of the ring (matrix applied). */
extern char tiDrawCollisionTransformed[];
/* Second pass of the ring, the input of the collision fill. */
extern char D_00646B40[];
extern char D_0068F830[];
extern char D_0068F838[];
extern char D_0068F858[];

/* Transforms the 48-point ring by pTransMat in two passes and fills the collision object pCol from the result. */
void tiDrawCollisionFill(void *pTransMat, void *pCol)
{
    if (pCol == 0)
    {
        assertFailed(D_0068F830, D_0068F838, 51);
    }
    if (pTransMat == 0)
    {
        assertFailed(D_0068F858, D_0068F838, 52);
    }
    tiDrawJointTransformPoints(48, pTransMat, tiDrawCollisionRing, tiDrawCollisionTransformed);
    func_00295780(48, tiDrawCollisionTransformed, D_00646B40);
    tiDrawJointPackPoints(48, D_00646B40, pCol, 0);
}

void xglCurMatrixGet(float matrix[4][4]);
void xglCurSetTranslate(void *object);
void tiDrawCollisionBuildRing(float scale);

/* Per-frame collision draw of one object: builds the ring at scale, takes the object transform and fills pCol. */
void tiDrawCollisionUpdate(void *object, void *pCol, float scale)
{
    unsigned int trans_mat[0x10];

    xglMatrixStackUnit();
    tiDrawCollisionBuildRing(scale);
    xglCurSetTranslate(object);
    xglCurMatrixGet(&trans_mat[0]);
    tiDrawCollisionFill(&trans_mat[0], pCol);
}

void func_002996C8(void)
{
}
