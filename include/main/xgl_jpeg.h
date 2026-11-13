#ifndef INCLUDE_MAIN_XGL_JPEG_H
#define INCLUDE_MAIN_XGL_JPEG_H

/* The request of src/main/xgl_jpeg.c's decoder (xglJpegDecode, StartOfScan). */

typedef struct JpegImageDimensions {
    short width;
    short height;
} JpegImageDimensions;

/* The 32-byte decode request. With destination 0 the image is uploaded to
 * the GS frame buffer at frameBufferBase / frameBufferWidth, origin
 * originX / originY; otherwise it is written to destination. */
typedef struct JpegDecodeControl {
    u8 *source;
    u8 *destination;
    int consumedSize; /* +0x08: bytes of source parsed up to the RST0/EOI marker */
    short frameBufferBase;
    short frameBufferWidth;
    u16 originX;
    u16 originY;
    JpegImageDimensions imageLimits; /* +0x14 */
    u8 luminancePercent;
    u8 chrominancePercent;
    u8 scaleMode; /* +0x1a */
    u8 pixelFormat;
    u8 redPercent;
    u8 greenPercent;
    u8 bluePercent;
    u8 unmodeled_1f;
} JpegDecodeControl;

int xglJpegDecode(JpegDecodeControl *request);

#endif /* INCLUDE_MAIN_XGL_JPEG_H */
