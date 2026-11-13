#include "common.h"

/* Swaps the two height-buffer pointers the arguments address (the water sim swaps its buffers this way). */
void tiWatSimSwapWords(float **first, float **second) {
    float *firstValue;

    firstValue = *first;
    *first = *second;
    *second = firstValue;
}

#include "shared.h"

typedef struct TiWatSimItem {
    u8 unmodeled_00[0x08];
    int cols;
    int rows;
    u8 unmodeled_10[0x40];
    float cell_spacing;
    float wave_speed;
    u8 unmodeled_58[0x10];
    float *height_buffer_a;
    float *height_buffer_b;
    u8 unmodeled_70[0x08];
    u32 mask_offset;
    u32 normals_offset;
    u8 unmodeled_80[0x04];
    unsigned char *block;
} TiWatSimItem;

/* One water step: buffer A drives buffer B over the interior cells, masked cells are scaled, the
 * buffers are swapped, and the normals of the interior are rebuilt from buffer B. */
void tiWatSimAdvance(TiWatSimItem *item)
{
    float courant;
    float courant_squared;
    float self_weight;
    unsigned int x;
    unsigned int y;
    unsigned int row;
    float next_height;
    float nx;
    float nz;
    float dy;
    float length_squared;
    float inverse;
    float *mask = (float *) (item->block + item->mask_offset);
    DoublewordVector4 *normals;

    /* courant is the wave speed scaled to the grid step; the centre cell keeps 2 - 4 * courant^2 of its height. */
    courant = item->wave_speed * 0.111f;
    courant_squared = courant * courant;
    self_weight = 2.0f - courant_squared * 4.0f;
    normals = (DoublewordVector4 *) (item->block + item->normals_offset);
    for (y = 1; y < item->rows - 1; y++) {
        row = y * item->cols;
        for (x = 1; x < item->cols - 1; x++) {
            next_height = courant_squared * (item->height_buffer_a[row - item->cols + x] + item->height_buffer_a[row + item->cols + x]
                    + item->height_buffer_a[row + x - 1] + item->height_buffer_a[row + x + 1])
                + self_weight * item->height_buffer_a[row + x] - item->height_buffer_b[row + x];
            item->height_buffer_b[row + x] = next_height;
            if (mask[row + x] != 0.0f) {
                item->height_buffer_b[row + x] = next_height * mask[row + x];
            } else {
                item->height_buffer_b[row + x] = 0.0f;
            }
            nx = item->height_buffer_b[row + x - 1] - item->height_buffer_b[row + x + 1];
            normals[row + x].vector.x = nx;
            nz = item->height_buffer_b[row - item->cols + x] - item->height_buffer_b[row + item->cols + x];
            normals[row + x].vector.z = nz;
            dy = item->cell_spacing * 2.25f;
            length_squared = nx * nx + dy * dy + nz * nz;
            inverse = 1.0f / length_squared;
            normals[row + x].vector.z = nz * inverse;
            normals[row + x].vector.x = nx * inverse;
            dy = dy * inverse;
            normals[row + x].vector.y = dy;
        }
    }
    tiWatSimSwapWords(&item->height_buffer_a, &item->height_buffer_b);
}

/* Runs one water simulation step (tiWatSimAdvance). */
void tiWatSimUpdate(TiWatSimItem *item)
{
    tiWatSimAdvance(item);
}
