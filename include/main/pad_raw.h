#ifndef INCLUDE_MAIN_PAD_RAW_H
#define INCLUDE_MAIN_PAD_RAW_H

/*
 * The raw pad words at 0x0063EDB0: one halfword per port (ov01 tu022 dataPadRead returns port 0 to 2
 * of them), also loaded as one doubleword by the menu slot state machine of src/main/tu100.c, which
 * tests the button bits of port 0 (0x5000) and port 2 (0x2000 << 32, 0x8000 << 32) together; the third
 * halfword alone is read by the menu cursors of tu100 (PAD_UP/DOWN/LEFT/RIGHT). Defined in src/main/xgl_dma.c.
 */
typedef union PadRawWords {
    unsigned short port[4];
    unsigned long long all;
} PadRawWords;

extern PadRawWords dataPadRawWords[];

#endif /* INCLUDE_MAIN_PAD_RAW_H */
