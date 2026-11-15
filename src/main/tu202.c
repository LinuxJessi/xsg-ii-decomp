#include "common.h"

/* Converts an x coordinate to 12.4 fixed point, offset into the GS display area. */
int gsVertexX(int x)
{
  int zero;
  int magnitude;
  zero = 0;
  magnitude = x;
  if (x < zero)
  {
    magnitude = -magnitude;
  }
  if (zero < x)
  {
    return (x * 0x10) + 0x7008;
  }
  return (-(magnitude * 0x10)) - 0x7008;
}

/* Converts a y coordinate to 12.4 fixed point, offset into the GS display area. */
int gsVertexY(int y)
{
  int zero;
  int magnitude;
  zero = 0;
  magnitude = y;
  if (y < zero)
  {
    magnitude = -magnitude;
  }
  if (zero < y)
  {
    return (y * 0x10) + 0x7208;
  }
  return (-(magnitude * 0x10)) - 0x7208;
}

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
