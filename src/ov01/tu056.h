#ifndef OV01_TU057_H
#define OV01_TU057_H

/* Object behind menuTgtStatusPanel; its word at +0x94 is set to 1 and cleared to 0 by the two toggles. */
struct Ov01Tu057State {
    unsigned char unmodeled_00[0x94];
    int enabled;
};

/* Object reached through Ov01Tu057Context.link; its halfword at +0x80 is passed to a draw call. */
struct Ov01Tu057Link {
    unsigned char unmodeled_00[0x80];
    short value;
};

/* Context argument of the draw routine; its pointer at +0x144 leads to the link object. */
struct Ov01Tu057Context {
    unsigned char unmodeled_00[0x144];
    struct Ov01Tu057Link *link;
};

#endif
