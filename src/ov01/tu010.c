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

extern void *memset(void *destination, int value, unsigned int count);

extern XglTaskPrefix *xglTaskEntryNext(XglTaskScheduler *scheduler,
                                int (*callback)(XglTaskPrefix *task),
                                XglTaskPrefix *entry);

/*
 * Actor.cmdTarget's type: unitCmdPosAdj saves and restores the whole command
 * target with aligned ld/sd pairs (0x00a04aa4..0x00a04ab8,
 * 0x00a04aec..0x00a04af8), where a plain Vector4 (4-byte aligned) copies
 * through ldl/ldr/sdl/sdr. The u64 pair is the alignment spelling of
 * MCamVector (src/ov01/m_cam.c) and GameVector8 (src/main/game.c) and carries
 * no meaning of its own; the components are read through `vector` and the whole is copied as one 8-byte value (unitCmdPosAdj's savedTarget). Its 8-byte
 * alignment rounds the partial Actor view's sizeof from 0x76C to 0x770;
 * no offset moves and nothing in this TU takes sizeof(Actor).
 */
typedef union UnitVector {
    Vector4 vector;
    u64 pair[2];
} UnitVector;

/*
 * unitCmdDirSet/unitCmdDivSet (0x00a05a60, 0x00a05ba8) also read and blend
 * the x/z components of the Vector4 at +0x90 against a move command's target
 * coordinate; y and w of that Vector4 are untouched by any function this TU
 * claims.
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
    int runFrameCounter;   /* +0x70: unitCmdRun tests it modulo 3 to space
                             * out its footstep decoEffCall. */
    int moveState;         /* +0x74: unitCmdMove's own move-command step;
                             * 0 issues the move (unitCmdDstSet/unitCmdVASet
                             * and, for a jump, unitCmdDirSet) and advances to
                             * 1, which then waits for unitCmdJump/unitCmdWarp
                             * to report arrival before calling
                             * unitCmdMoveNext. */
    int movePhase;          /* +0x78: reset to 0 by unitCmdMove when a move
                             * starts; unitCmdJump/unitCmdWarp (still asm)
                             * advance it through their own per-move-kind
                             * phases. */
    int movePhaseTimer;     /* +0x7C: unitCmdWarp zeroes it every time it
                             * advances movePhase; no claimed function here
                             * reads it. */
    ObjectTask *targetUnit; /* +0x80: unitCmdDstSet's mode-0 path reads the
                             * command target's own CalcUnitParam/table
                             * position through it. */
    unsigned char unmodeled_84[0x90 - 0x84];
    UnitVector cmdTarget; /* +0x90: unitCmdDirSet reads x/z against
                            * motionActor->position to face the pending move
                            * command; unitCmdDivSet blends x/z toward that
                            * position by a supplied ratio. y and w are
                            * untouched by any function this TU claims. */
    unsigned char unmodeled_a0[0xC0 - 0xA0];
    float transparency;   /* +0xC0: unitActdraw draws translucent below 1.0 */
    unsigned char unmodeled_c4[0xD0 - 0xC4];
    float posSnapX;        /* +0xD0: unitCmdJump's arrival check rounds it
                             * and compares against posSnapZ*0.5 rounded. */
    float hopLength;       /* +0xD4: replaces four bytes of filler
                             * (unmodeled_d4) in this existing local Actor
                             * view. unitCmdMoveChaJump2 takes its address
                             * (`addiu $19,$2,0xD4`, 0x00a040dc), stores the
                             * command distance divided by 3 (`swc1 $f13,0($19)`,
                             * 0x00a04114) and reloads it to aim each
                             * intermediate hop (`lwc1 $f0,0($19)`, 0x00a04170). */
    float posSnapZ;        /* +0xD8 */
    unsigned char unmodeled_dc[0xE0 - 0xDC];
    int motionFrame;      /* +0xE0: unitActupdate advances it before
                             ACT_updateMotion */
    unsigned char unmodeled_e4[0xF0 - 0xE4];
    int motion;            /* +0xF0: unitMotGet returns it plus one */
    float moveOriginY;     /* +0xF4: unitCmdRun/unitCmdJump save cmdTarget's
                             * y/z here before a stand/attack phase and
                             * restore them afterward; the paired x value at
                             * this same phase reuses the four bytes of
                             * `motion` above through a cast (see the .c
                             * file), never through a new field here. */
    float moveOriginZ;     /* +0xF8 */
    unsigned char unmodeled_fc[0x6F0 - 0xFC];
    int moveFlags;          /* +0x6F0: unitCmdRun/unitCmdHover/unitCmdFloat/
                              * unitCmdJump test bit 0x1000 of it to detect
                              * that a move command has reached its target. */
    float moveElapsed;      /* +0x6F4: unitCmdJump reads it as a frame count
                              * source for its arrival check. */
    float moveStepRate;      /* +0x6F8: unitCmdFloat/unitCmdJump write a
                              * per-frame interpolation rate here. */
    unsigned char unmodeled_6fc[0x714 - 0x6FC];
    float interpTime;      /* +0x714: unitSetInterpTime's write target */
    unsigned char unmodeled_718[0x760 - 0x718];
    float floatStepX;      /* +0x760: unitCmdFloat adds it to its motion
                             * actor's position.x each step while floating. */
    unsigned char unmodeled_764[0x768 - 0x764];
    float floatStepZ;      /* +0x768 */
} Actor;

/*
 * Partial view of the unit record beyond the published ObjectTask prefix
 * (task+work, ending at +0x14, include/shared.h): unitSetInterpTime and
 * unitMotGet both follow a pointer immediately after that prefix, and
 * unitWpnMotSet reads a 4-entry pointer array starting eight bytes further.
 * Both point at Actor objects: unitWpnMotSet forwards the entry it reads to
 * ACT_animGetData/ACT_setMotion, whose first parameter is already
 * Actor-typed (src/main/act_2.c). No other byte of this wider record is
 * claimed, and the relationship between this pointer and ObjectTask.work is
 * not evidenced here.
 */
typedef struct UnitRecord UnitRecord;

struct UnitRecord {
    ObjectTask task;                   /* +0x00..+0x13: the ObjectTask
                                         * prefix, opaque here */
    Actor *motionActor;                /* +0x14 */
    unsigned char unmodeled_018[0x1c - 0x18]; /* +0x18..+0x1b */
    Actor *wpnActor[4];                /* +0x1c */
    Actor *subActor;                   /* +0x2c: unitRemoveActor disposes it
                                         * first when it is set */
};

/* The same eight unit pointers have full-record and scheduler-prefix views. */
typedef union UnitTableStorage {
    UnitRecord *records[8];
    ObjectTask *tasks[8];
} UnitTableStorage;

extern UnitTableStorage unitTbl;
extern int unitPlNum;
extern int unitEnNum;

void unitInit(void)
{
    memset(&unitTbl, 0, sizeof(unitTbl));
    unitPlNum = 0;
    unitEnNum = 0;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu010", func_00A84128);

INCLUDE_ASM("asm/nonmatchings/ov01/tu010", func_00A84168);

INCLUDE_ASM("asm/nonmatchings/ov01/tu010", func_00A841B8);
