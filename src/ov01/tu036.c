#include "common.h"
#include "shared.h"

typedef struct ObjectTask ObjectTask;
typedef struct XglTaskPrefix XglTaskPrefix;
typedef struct XglTaskScheduler XglTaskScheduler;

/*
 * Only the link/callback prefix is evidenced by this allocation.  The
 * complete task records are allocated with a 0x80-byte stride, and callers
 * write task-specific payload beyond this 0x10-byte prefix.  This declaration
 * must not be read as the complete task layout or allocation size.
 */
struct XglTaskPrefix {
    XglTaskScheduler *scheduler;
    XglTaskPrefix *next;
    XglTaskPrefix *previous;
    int (*callback)(XglTaskPrefix *task);
};

struct XglTaskScheduler {
    XglTaskPrefix *free_tasks;
    XglTaskPrefix *active_head;
    XglTaskPrefix *active_tail;
    XglTaskPrefix *next_to_visit;
};

struct ObjectTask {
    XglTaskPrefix task; /* the scheduler header xglTaskEntryNext maintains. */
    void *work; /* objRemove passes this slot to objWorkFree. */
};

extern XglTaskPrefix *xglTaskEntryNext(XglTaskScheduler *scheduler,
                                int (*callback)(XglTaskPrefix *task),
                                XglTaskPrefix *entry);

/*
 * The battle actor object a unit's actor/weapon slots point to. src/main/
 * chr.h, src/main/near_dir.h, src/main/set_motion.h and src/ov01/unit_cmd.h
 * all restate this same 0x70-byte head verbatim (a TU-local header cannot be
 * included from another TU); this TU adds only the words its own code
 * touches: the float at +0xC0 (transWepIn/transWepOut set it to 0.0/1.0, the
 * same "transparency" member src/ov01/unit_cmd.h restates at that offset)
 * and the flags word at +0x6F0 (hairWindOn ORs in HAIR_WIND_BIT there for
 * every other unit's actor and hairWindOff clears it again).
 */
typedef struct Actor {
    u32 flags;
    void (*update)(struct Actor *actor);
    void (*draw)(struct Actor *actor);
    u32 quadword_alignment_gap;
    Vector4 position;
    Vector4 previous_position;
    Vector4 velocity;
    Vector4 acceleration;
    Vector4 rotation;
    Vector4 scale;
    unsigned char unmodeled_70[0xC0 - 0x70];
    float transparency; /* +0xC0 */
    unsigned char unmodeled_c4[0x6F0 - 0xC4];
    u32 windFlags; /* +0x6F0 */
} Actor;

/*
 * The unit record scenarioMtd, transWepIn/transWepOut and spWepDispOn
 * (0x00a0e550) all share: spWepDispOn receives the
 * same pointer scenarioMtd receives, dereferences it to reach this record,
 * then indexes its +0x1C table by (weapon-slot & 3) and reads +0x14, exactly
 * as transWepIn/transWepOut do on their own argument and scenarioMtd does on
 * *unitHandle. Bytes between the evidenced members are untouched here.
 */
typedef struct BattleUnit BattleUnit;

struct BattleUnit {
    unsigned char unmodeled_00[0x10];
    Actor *work;       /* +0x10 */
    Actor *actor;      /* +0x14 */
    unsigned char unmodeled_18[0x1C - 0x18];
    Actor *weapon[4];  /* +0x1C */
};

BattleUnit **tgtUnitAdrGet(void);
BattleUnit *tgtUnitGet(void);

/*
 * Private battle state, in original BSS address order. Keeping these local
 * definitions together lets the guarded overlay linker map each exact object
 * to its original storage span.
 */
extern ObjectTask *pTaskEnCur;

/*
 * objEntry2 and objStdInit are defined by ov01/tu001 obj.c (src/ov01/obj.h
 * is that TU-local header and is not includable here);
 * src/ov01/entry_first_init.c already redeclares them the same way.
 */
extern ObjectTask *objEntry2(void *argument, void (*callback)(ObjectTask *task));

extern void objRemove(ObjectTask *task);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A97D68);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A97F08);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A97F40);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A97FE0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98180);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A981B8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98258);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98408);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98440);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A984E0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98690);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A986C8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98768);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98928);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98960);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98A00);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98BB0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98BE8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98C88);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98CF8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98DA8);

void objCurRemove(void)
{
    if (pTaskEnCur != 0) {
        objRemove(pTaskEnCur);
        pTaskEnCur = 0;
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A98F70);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99A98);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99CA0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", tgtUnitAdrGet);

BattleUnit *tgtUnitGet(void)
{
    return *tgtUnitAdrGet();
}

void tgtUnitSet(BattleUnit *unit)
{
    *tgtUnitAdrGet() = unit;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99D98);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99DA8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99DB8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99DC8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99DD8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99DE8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99DF8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99E08);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99E18);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99E58);

INCLUDE_ASM("asm/nonmatchings/ov01/tu036", func_00A99E68);
