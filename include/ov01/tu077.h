#ifndef INCLUDE_OV01_TU077_H
#define INCLUDE_OV01_TU077_H

/* Functions of src/ov01/tu077.c that the later effect files (tu078-tu089) call. */

struct Actor;

Matrix4 *MSeGetActorMatrix(Matrix4 *destination, struct Actor *actor, u32 coordinate);
Vector4 *MEfMakeRandomVector(Vector4 *destination);
Vector4 *MEfScatterXY(Vector4 *destination, Vector4 *target, Vector4 *source, float spread);
int MEfMake4Vertex(Vector4 *destination, const Vector4 *source, float halfSize);
int MEfMake4Vertex2(Vector4 *destination, const Vector4 *source, float halfSize, float angleBase);

#endif /* INCLUDE_OV01_TU077_H */
