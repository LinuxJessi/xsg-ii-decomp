#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu202", gsVertexX);

INCLUDE_ASM("asm/main/nonmatchings/tu202", gsVertexY);

/* Converts a coordinate to 12.4 fixed point with half-unit rounding by sign (gsRoundCoord). */
int gsRoundCoord(int coordinate)
{
  int zero;
  int magnitude;
  zero = 0;
  magnitude = coordinate;
  if (coordinate < zero)
  {
    magnitude = -magnitude;
  }
  if (zero < coordinate)
  {
    return (coordinate * 0x10) + 8;
  }
  return (-(magnitude * 0x10)) - 8;
}
