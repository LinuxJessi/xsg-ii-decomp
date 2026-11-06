#include "common.h"
#include "main/tu026.h"

INCLUDE_ASM("asm/main/nonmatchings/TiDrawJoint", func_002956D8);

INCLUDE_ASM("asm/main/nonmatchings/TiDrawJoint", func_00295780);

#include "shared.h"

void xglVectorMulMat(Vector4 *destination, float matrix[4][4], const Vector4 *vector);

/* Transforms count points by matrix into destination; each result gets w = 1. */
void tiDrawJointTransformPoints(unsigned int count, float matrix[4][4], const Vector4 *source,
                   Vector4 *destination)
{
    unsigned int index;

    index = 0;
    if (count != 0) {
        do {
            xglVectorMulMat(destination, matrix, source);
            destination->w = 1.0f;
            index += 1;
            source += 1;
            destination += 1;
        } while (index < count);
    }
}

INCLUDE_ASM("asm/main/nonmatchings/TiDrawJoint", tiDrawJointPackPoints);

INCLUDE_ASM("asm/main/nonmatchings/TiDrawJoint", tiDrawJointDrawPoints);

void func_00295780(unsigned int, const Vector4 *, Vector4 *);
void tiDrawJointPackPoints(unsigned int, const Vector4 *, int, int);
extern unsigned char D_0068F750[];
extern unsigned char D_0068F788[];
extern Vector4 tiDrawJointMeshPoints[];
extern Vector4 tiDrawJointMeshTransformed[];
extern Vector4 D_00989C50[];

/* Draws the 16-point mesh tiDrawJointMeshPoints through current_matrix (asserted as pCurrMat).
 * mode reaches the packet builder in its colour position (asserted as pCol there); its
 * declared int type is kept from the original until the caller's type is settled. */
void tiDrawJointDrawMesh(float current_matrix[4][4], int mode)
{
    if (current_matrix == 0) {
        assertFailed(D_0068F788, D_0068F750, 210);
    }
    tiDrawJointTransformPoints(16, current_matrix, tiDrawJointMeshPoints, tiDrawJointMeshTransformed);
    func_00295780(16, tiDrawJointMeshTransformed, D_00989C50);
    tiDrawJointPackPoints(16, D_00989C50, mode, 0);
}

INCLUDE_ASM("asm/main/nonmatchings/TiDrawJoint", func_00295CF8);

INCLUDE_ASM("asm/main/nonmatchings/TiDrawJoint", func_00295EE8);
