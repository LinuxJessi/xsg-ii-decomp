#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu062", IpuMovieTest);

INCLUDE_ASM("asm/main/nonmatchings/tu062", func_001ED1C0);

INCLUDE_ASM("asm/main/nonmatchings/tu062", func_001ED358);

/* Partial view of a point: the VU0 block reads only its X and Z components. */
typedef struct PointXZ {
    int x;
    int z;
} PointXZ;

float I2F();

/* Returns 1 when the VU0 orientation value is negative, 0 otherwise. */
int func_001ED420(PointXZ *origin, PointXZ *point1, PointXZ *point2, PointXZ *point3)
{
    float origin_vec[4];
    float point1_vec[4];
    float point2_vec[4];
    float point3_vec[4];
    float orientation;

    origin_vec[0] = I2F(origin->x);
    origin_vec[2] = I2F(origin->z);
    point1_vec[0] = I2F(point1->x);
    point1_vec[2] = I2F(point1->z);
    point2_vec[0] = I2F(point2->x);
    point2_vec[2] = I2F(point2->z);
    point3_vec[0] = I2F(point3->x);
    point3_vec[2] = I2F(point3->z);
    __asm__ __volatile__("lqc2 vf10, 0(%1)\n\tlqc2 vf4, 0(%2)\n\tlqc2 vf5, 0(%3)\n\tlqc2 vf6, 0(%4)\n\tvsub.xz vf1, vf4, vf10\n\tvsub.xz vf2, vf5, vf10\n\tvsub.xz vf3, vf6, vf10\n\tvmul.xz vf7, vf1, vf1\n\tvmul.xz vf8, vf2, vf2\n\tvmul.xz vf9, vf3, vf3\n\tvaddz.x vf4, vf7, vf7z\n\tvaddz.x vf5, vf8, vf8z\n\tvaddz.x vf6, vf9, vf9z\n\tvrsqrt Q, vf0w, vf4x\n\tvwaitq\n\tvaddq.x vf4, vf0, Q\n\tvmulz.x vf7, vf1, vf2z\n\tvmulz.x vf8, vf2, vf1z\n\tvmulz.x vf9, vf1, vf3z\n\tvmulz.x vf10, vf3, vf1z\n\tvsub.x vf2, vf7, vf8\n\tvsub.x vf3, vf9, vf10\n\tvrsqrt Q, vf2x, vf5x\n\tvwaitq\n\tvmulq.x vf1, vf4, Q\n\tvrsqrt Q, vf3x, vf6x\n\tvwaitq\n\tvmulq.x vf2, vf4, Q\n\tvmul.x vf3, vf1, vf2\n\tqmfc2.ni %0, vf3" : "=r"(orientation) : "r"(origin_vec), "r"(&point1_vec[0]), "r"(&point2_vec[0]), "r"(&point3_vec[0]) : "memory");
    if (!(!(orientation >= 0.0f))) {
        return 1;
    }
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu062", func_001ED568);
