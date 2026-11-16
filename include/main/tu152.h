#ifndef INCLUDE_MAIN_TU152_H
#define INCLUDE_MAIN_TU152_H

/* Functions of src/main/tu152.c that other files call. */

/* The identity matrix the rotation functions start from (test_128.c defines it). */
extern const float vu0IdentityMatrix[16];

void vu0SetCurrentRotationY(float value);

#endif /* INCLUDE_MAIN_TU152_H */
