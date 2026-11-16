#include "common.h"
#include "shared.h"
#include "main/tu168.h"
#include "vu0_macro.h"

/* A key sequence: keys holds rows of stride halfwords, the first of each row being the frame
 * the row starts at (see the SDV_KEY_ values below). */
typedef struct SdvSequence {
    short *keys;            /* +0x00 */
    short elapsed;          /* +0x04 frames since the sequence started */
    signed char position;   /* +0x06 current row */
    signed char stride;     /* +0x07 halfwords per row */
} SdvSequence;

#define SDV_KEY_END 0x400           /* frame value that ends the table */
#define SDV_KEY_WAIT_HIT 0x1000     /* the row waits for the hit signal */
#define SDV_KEY_FRAME_MASK 0xFFF    /* frame of a hit-wait row */

/* Actor select bits of a key row: which actor of the effect request the row applies to. */
#define SDV_KEY_SOURCE 0x40         /* the request's source actor */
#define SDV_KEY_TARGET 0x80         /* the request's target actor */
#define SDV_KEY_LINK_1 0x400        /* the request's linked actors (links[0..2]) */
#define SDV_KEY_LINK_2 0x800
#define SDV_KEY_LINK_3 0x1000

/* The actor (xenoActor.c); only the move-state and shadow fields sdvExecShadowSeq sets. func_00289028
 * (XenoActorUpdateMoveState) animates state +0x160, angle +0x150 and velocity +0x154. */
struct XenoActor {
    unsigned char unmodeled_000[0x150];
    float angle;            /* +0x150 */
    float velocity;         /* +0x154 */
    float shadowValue;      /* +0x158 */
    int shadowCount;        /* +0x15C */
    int state;              /* +0x160 */
    unsigned char unmodeled_164[4];
    float shadowValue3;     /* +0x168 */
};

/* The effect request copied into sefRequestBuffer (0x40 bytes). */
typedef struct SefRequest {
    struct XenoActor *source;   /* +0x00 */
    unsigned char unmodeled_04[4];
    struct XenoActor *target;   /* +0x08 */
    struct XenoActor *links[3]; /* +0x0C the actors SDV_KEY_LINK_1..3 select */
    unsigned char unmodeled_18[0x24 - 0x18];
    unsigned char waitHit;  /* +0x24 nonzero: hit-wait rows report their row */
} SefRequest;

/* Hit signal of the current effect step; sefExecStep clears it. */
extern short D_0069DB82;

/* Returns the current row when the sequence reaches it (and advances to the next row), else -1
 * (sdvExecSeqTbl's key test in X1). */
int sdvCheckSeqKey(SdvSequence *sequence)
{
    int result = -1;
    short key = sequence->keys[sequence->position * sequence->stride];
    SefRequest *request = (SefRequest *)sefGetRequestBuffer();
    int hitFrame = key & SDV_KEY_FRAME_MASK;

    if ((key & SDV_KEY_WAIT_HIT) && D_0069DB82 != 0 && sequence->elapsed >= hitFrame) {
        if (request->waitHit == 0) {
            sequence->position++;
        } else {
            result = sequence->position++;
        }
    } else if (sequence->elapsed >= key) {
        result = sequence->position++;
    }
    return result;
}

/* Counts one frame (up to 0x3FF) and runs the key test; SDV_KEY_END when the sequence has ended
 * (sdvExecSeqTbl in X1, which counts after the test). */
int sdvExecSeqTbl(SdvSequence *sequence)
{
    if (sequence->keys[sequence->position * sequence->stride] == SDV_KEY_END) {
        return SDV_KEY_END;
    }
    sequence->elapsed++;
    if (sequence->elapsed >= 0x400) {
        sequence->elapsed = 0x3FF;
    }
    return sdvCheckSeqKey(sequence);
}

/* The same key test on a row position kept by the caller and an elapsed frame count passed in;
 * SDV_KEY_END when the sequence has ended (sdvProgressKey in X1). */
int sdvProgressKey(short *keys, short *position, int stride, int elapsed)
{
    int result = -1;
    int key = keys[*position * stride];
    SefRequest *request = (SefRequest *)sefGetRequestBuffer();
    int waitsForHit = key & SDV_KEY_WAIT_HIT;
    int hitFrame = key & SDV_KEY_FRAME_MASK;

    if (key == SDV_KEY_END) {
        return SDV_KEY_END;
    }
    if (waitsForHit && D_0069DB82 != 0 && elapsed >= hitFrame) {
        if (request->waitHit == 0) {
            (*position)++;
        } else {
            result = (*position)++;
        }
    } else if (elapsed >= key) {
        result = (*position)++;
    }
    return result;
}

/* Columns of an effect key row: the halfwords after the row's frame. */
enum SdvEffectKeyColumn {
    SDV_EFFECT_KIND = 1,        /* the MSe effect kind */
    SDV_EFFECT_TARGET = 2,      /* SDV_KEY_TARGET plus the part the effect targets */
    SDV_EFFECT_OWNER = 3,       /* SDV_KEY_SOURCE or SDV_KEY_LINK_n plus the part it is attached to */
    SDV_EFFECT_OFFSET_X = 4,    /* offset from that actor, in thousandths */
    SDV_EFFECT_OFFSET_Y = 5,
    SDV_EFFECT_OFFSET_Z = 6
};

/* The 0x40-byte request MSeFire (ov01) copies into a new MSe effect object; kind picks the
 * creator and effectId goes into the object's +0x4. Each actor comes with the part it is attached at. */
typedef struct MSeRequest {
    int kind;                   /* +0x00 */
    struct XenoActor *owner;    /* +0x04 */
    int ownerPart;              /* +0x08 */
    struct XenoActor *link;     /* +0x0C */
    int linkPart;               /* +0x10 */
    unsigned char unmodeled_14[0x20 - 0x14];
    Vector4 offset;             /* +0x20 */
    struct XenoActor *target;   /* +0x30 */
    int targetPart;             /* +0x34 */
    int effectId;               /* +0x38 the effect record's id (sef_164 record +0x6C) */
    unsigned char unmodeled_3c[0x40 - 0x3C];
} MSeRequest;

/* Clears a 0x40-byte request block: memset(buffer, 0, 0x40) (ov01 tu075.c). Every MSeRequest
 * field the select path below leaves unassigned is therefore 0. */
void func_00AAAEC8(void *buffer);
void MSeFire(MSeRequest *request);

/* Steps an effect key sequence and fires the MSe effect of the row it reaches, attached to
 * the actors of the current effect request. */
void sdvExecEffectSeq(SdvSequence *sequence, int effectId)
{
    MSeRequest fire;
    DoublewordVector4 offset;
    short *keys;
    int row;
    int index;
    SefRequest *request;
    int kind;
    int targetSelect;
    int ownerSelect;

    if (sequence == 0 || sequence->keys == 0) {
        return;
    }
    index = sdvExecSeqTbl(sequence);
    if ((unsigned int)index >= 0x400) {
        return;
    }
    request = (SefRequest *)sefGetRequestBuffer();
    /* The row is read halfword by halfword: through a row pointer gcc keeps the row in keys' register. */
    keys = sequence->keys;
    row = index * sequence->stride;
    kind = keys[row + SDV_EFFECT_KIND];
    targetSelect = keys[row + SDV_EFFECT_TARGET];
    ownerSelect = keys[row + SDV_EFFECT_OWNER];
    offset.vector.x = keys[row + SDV_EFFECT_OFFSET_X];
    offset.vector.y = keys[row + SDV_EFFECT_OFFSET_Y];
    offset.vector.z = keys[row + SDV_EFFECT_OFFSET_Z];
    offset.vector.w = 1.0f;
    func_00AAAEC8(&fire);
    Vu0SefScaleVectorXYZNI(&fire.offset, &offset.vector, 0.001f);
    fire.kind = kind;
    fire.effectId = effectId;
    fire.owner = request->source;
    /* A select is the actor bit plus the part index plus one; 0xA1 (part 0x1F), 0x80 and 0xA2 (part 1)
     * and the bare SDV_KEY_SOURCE (part 1) are remapped. */
    if (targetSelect == 0xA1) {
        fire.target = request->target;
        fire.targetPart = 0x1F;
    } else if (targetSelect == SDV_KEY_TARGET || targetSelect == 0xA2) {
        fire.target = request->target;
        fire.targetPart = 1;
    } else if (targetSelect & SDV_KEY_TARGET) {
        fire.target = request->target;
        fire.targetPart = targetSelect - (SDV_KEY_TARGET + 1);
    }
    if (ownerSelect & SDV_KEY_LINK_1) {
        fire.link = request->links[0];
        fire.linkPart = ownerSelect - (SDV_KEY_LINK_1 + 1);
    } else if (ownerSelect & SDV_KEY_LINK_2) {
        fire.link = request->links[1];
        fire.linkPart = ownerSelect - (SDV_KEY_LINK_2 + 1);
    } else if (ownerSelect & SDV_KEY_LINK_3) {
        fire.link = request->links[2];
        fire.linkPart = ownerSelect - (SDV_KEY_LINK_3 + 1);
    } else if (ownerSelect == SDV_KEY_SOURCE) {
        fire.ownerPart = 1;
    } else if (ownerSelect & SDV_KEY_SOURCE) {
        fire.ownerPart = ownerSelect - (SDV_KEY_SOURCE + 1);
    }
    MSeFire(&fire);
}

/* One row of a shadow key sequence. */
typedef struct SdvShadowKey {
    short frame;            /* +0x00 */
    short flags;            /* +0x02 SDV_KEY_SOURCE / SDV_KEY_TARGET pick the actor */
    signed char shadowCount; /* +0x04 */
    signed char state;      /* +0x05 */
    short velocity;         /* +0x06 tenths */
    short shadowValue;      /* +0x08 hundredths */
    short angle;            /* +0x0A hundredths */
    short shadowValue3;     /* +0x0C hundredths */
} SdvShadowKey;

/* Steps a shadow key sequence and copies the row it reaches into the shadow fields of the
 * request's source or target actor. */
void sdvExecShadowSeq(SdvSequence *sequence)
{
    int index;
    SefRequest *request;
    SdvShadowKey *key;
    struct XenoActor *actor;
    int flags;
    int state;
    int shadowCount;
    int shadowValue3Hundredths;
    int velocityTenths;
    int shadowValueHundredths;
    int angleHundredths;
    float velocity;
    float shadowValue;
    float angle;
    float shadowValue3;

    if (sequence == 0 || sequence->keys == 0) {
        return;
    }
    index = sdvExecSeqTbl(sequence);
    if ((unsigned int)index >= 0x400) {
        return;
    }
    request = (SefRequest *)sefGetRequestBuffer();
    actor = 0;
    key = (SdvShadowKey *)(sequence->keys + index * sequence->stride);
    /* The row is read first and scaled afterwards, shadowValue3 last: scaling each halfword as it
     * is read makes gcc issue the angle product before the shadowValue one. */
    shadowValue3Hundredths = key->shadowValue3;
    velocityTenths = key->velocity;
    shadowValueHundredths = key->shadowValue;
    angleHundredths = key->angle;
    flags = key->flags;
    state = key->state;
    shadowCount = key->shadowCount;
    velocity = velocityTenths * 0.1f;
    shadowValue = shadowValueHundredths * 0.01f;
    angle = angleHundredths * 0.01f;
    shadowValue3 = shadowValue3Hundredths * 0.01f;
    if (flags & SDV_KEY_TARGET) {
        actor = request->target;
    } else if (flags & SDV_KEY_SOURCE) {
        actor = request->source;
    }
    if (actor != 0) {
        actor->angle = angle;
        actor->shadowValue = shadowValue;
        actor->velocity = velocity;
        actor->shadowCount = shadowCount;
        actor->shadowValue3 = shadowValue3;
        actor->state = state;
    }
}
