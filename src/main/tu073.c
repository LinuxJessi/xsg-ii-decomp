#include "common.h"
#include "shared.h"
#include "main/xenoActor.h"
#include "main/tu086.h"

int uwamonoFindIndexByShortKey(short);
void func_A9CC70(int, int);
/* The running native call's argument count: takes the call id (enepc_001F60B8
 * only schedules `daddu $18,$4` into the jal delay slot with $4 live at the
 * call; declared without the argument it copies id before the array init). */
int func_A9F0A8(int call);
int func_A9F0C0(int, int);
int func_A9F1C0(int call, int index);

void XenoActorSetFirstWord();

/* The XenoActor of a map object (offsets as in include/main/tu051.h); only
 * the fields used below. The resource pointer at +0x600 is written by
 * enepc_001F5FE0. */
struct XenoActor {
  unsigned char unmodeled_0000[0x10];
  Vector4 position; /* +0x10 */
  unsigned char unmodeled_0020[0x60 - 0x20];
  /* A one-member union: Resigne reads it with alias set 0, after the short
   * store before it. */
  union {
    Vector4 direction; /* +0x60: y is the moving angle (include/main/tu051.h) */
  };
  unsigned char unmodeled_0070[0xBC - 0x70];
  int drawFlags; /* +0xBC: 0x4 hides the actor (include/main/tu051.h) */
  unsigned char unmodeled_00c0[0x5E4 - 0xC0];
  int selector; /* +0x5E4: the last XenoActorSetSelector value */
  unsigned char unmodeled_05e8[0x600 - 0x5E8];
  void *resource; /* +0x600 */
};

/* The per-unit state/motion block pointed to by uwamonoSlotTable[].state,
 * D_003400F4[].data and uwamonoRecords[].state (same 0x870-byte stride).
 * Only the fields written by the functions below are modeled. */
struct UwamonoState {
  unsigned char unmodeled_0000[0xC];
  signed char action; /* +0x0C: 5 while resigned, see Resigne */
  unsigned char unmodeled_000d[0x20 - 0xD];
  /* A one-member union: enepc_001F6A60 stores it with alias set 0, the
   * state pointer is reloaded after each store. */
  union {
    Vector4 home; /* +0x20: the record position, written by enepc_001F6A60 */
  };
  float speed; /* +0x30 */
  unsigned char unmodeled_0034[0x58 - 0x34];
  short actionTimer; /* +0x58 */
  unsigned char unmodeled_005a[0x5F - 0x5A];
  signed char mode; /* +0x5F, written by enepc_001F6A60 */
  unsigned char unmodeled_0060[0x90 - 0x60];
  Vector4 anchor; /* +0x90 */
  unsigned char unmodeled_00a0[0x4010 - 0xA0];
  int array4010[3]; /* +0x4010, written by enepc_001F6728 */
  unsigned char unmodeled_401c[0x4042 - 0x401C];
  short array4042[8]; /* +0x4042, written by setGroup */
  unsigned char unmodeled_4052[0x4054 - 0x4052];
  int group[4]; /* +0x4054, written by enepc_001F68D0 */
  unsigned char unmodeled_4064[0x4074 - 0x4064];
  int resourceId; /* +0x4074 */
};

/* One 0x870-byte slot of uwamonoSlotTable; only the actor and state pointers
 * at +0x00 and +0x04 are modeled here (same fields as src/main/tu085.c). */
struct UwamonoSlot {
  struct XenoActor *actor; /* +0x00 */
  struct UwamonoState *state; /* +0x04 */
  unsigned char unmodeled_08[0x870 - 8];
};

extern struct UwamonoSlot uwamonoSlotTable[];
int GameResLoadID(int id, void **buffer, unsigned int *size, void (*callback)(int index, void *buffer, unsigned int size, int value));

/* One 0x870-byte record of uwamonoRecords (include/main/tu051.h). The actor
 * and state pointers at +0x330/+0x334 are the same fields uwamonoSlotTable[]
 * names. */
typedef struct UwamonoRecord {
  unsigned char unmodeled_000[2];
  unsigned char kind; /* +0x002: 1 and 4 carry an actor (src/main/tu067.c) */
  unsigned char unmodeled_003[0x10 - 3];
  float position[4]; /* +0x010: read as an array by enepc_001F6A60 */
  unsigned char unmodeled_020[0x40 - 0x20];
  AlignedVector4 anchor; /* +0x040: quadword-aligned, makes the record 16-aligned */
  unsigned char unmodeled_050[0x330 - 0x50];
  struct XenoActor *actor; /* +0x330 */
  struct UwamonoState *state; /* +0x334 */
  unsigned char unmodeled_338[0x870 - 0x338];
} UwamonoRecord;

extern UwamonoRecord uwamonoRecords[];

void XenoActorSetSelector(struct XenoActor *actor, int selector);

/*
 * Returns 0 after func_A9CC70 when func_A9F0A8 does not report 3 or more.
 * Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; if found and the record is of kind 1 or 4 and
 * its actor's selector differs from argument 2, sets the actor's channel
 * value to 4.0f and its selector to argument 2. Argument 3 is read but
 * unused. Returns 1.
 */
int motionFace(int id)
{
  int lookupIndex;
  int selector;
  struct XenoActor *actor;

  if (func_A9F0A8(id) < 3) {
    func_A9CC70(id, 2);
    return 0;
  }
  lookupIndex = uwamonoFindIndexByShortKey((short) func_A9F0C0(id, 1));
  if (lookupIndex == -1) {
    return 1;
  }
  selector = func_A9F0C0(id, 2);
  func_A9F0C0(id, 3);
  if (uwamonoRecords[lookupIndex].kind != 1 && uwamonoRecords[lookupIndex].kind != 4) {
    return 1;
  }
  actor = uwamonoRecords[lookupIndex].actor;
  if (actor->selector != selector) {
    XenoActorSetChannelValueB(actor, 4.0f);
    XenoActorSetSelector(actor, selector);
  }
  return 1;
}

/*
 * Returns 0 after func_A9CC70 when func_A9F0A8 does not report 2 or more.
 * Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; when found and the slot has an actor, consumes
 * func_A9F0C0's second argument and calls XenoActorSetFirstWord on the
 * actor. Returns 1 on success, 0 otherwise.
 */
int enepc_001F5F20(int id)
{
  int lookupKey;
  int lookupIndex;
  if (func_A9F0A8(id) < 2) {
    func_A9CC70(id, 2);
    return 0;
  }
  lookupKey = func_A9F0C0(id, 1);
  lookupIndex = uwamonoFindIndexByShortKey((short) lookupKey);
  func_A9F0C0(id, 2);
  if (lookupIndex == -1) {
    return 0;
  }
  if (uwamonoSlotTable[lookupIndex].actor == 0) {
    return 0;
  }
  XenoActorSetFirstWord(uwamonoSlotTable[lookupIndex].actor, func_A9F0C0(id, 2));
  return 1;
}

/*
 * Returns 0 after func_A9CC70 when func_A9F0A8 does not report 2. Otherwise
 * looks up func_A9F0C0's first argument with uwamonoFindIndexByShortKey; if
 * found, loads the resource for func_A9F0C0's second argument, stores the
 * id to the slot's state and the loaded resource to the slot's actor.
 * Returns 1.
 */
int enepc_001F5FE0(int id)
{
  void *resource;
  void *loadedResource;
  int lookupKey;
  int lookupIndex;
  int resourceId;
  struct UwamonoSlot *slot;

  resource = 0;
  if (func_A9F0A8(id) != 2) {
    func_A9CC70(id, 2);
    return 0;
  }
  lookupKey = func_A9F0C0(id, 1);
  lookupIndex = uwamonoFindIndexByShortKey((short) lookupKey);
  if (lookupIndex == -1) {
    return 1;
  }
  resourceId = func_A9F0C0(id, 2);
  GameResLoadID(resourceId, &resource, 0, 0);
  loadedResource = resource;
  slot = &uwamonoSlotTable[lookupIndex];
  slot->actor->resource = loadedResource;
  slot->state->resourceId = resourceId;
  return 1;
}

/* Candidate target points and their reach (10 entries; src/main/tu051.c reads
 * them as Vector4 D_0033EE00[]; the float[][4] view gives this function's
 * point-index addressing, the Vector4 one swaps the addu operands). */
extern float D_0033EE00[][4];
extern float D_0033EE90[];

int uwamonoTryClearTarget(int index, signed char threshold);

/*
 * Returns 0 after func_A9CC70 when func_A9F0A8 does not report 4 or more.
 * Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey. With argument 5 present, when one of the 10
 * candidate target points has no reach yet, sets point number argument 5 to
 * arguments 2 to 4 / 100 with reach argument 6 / 100. Without it: arguments
 * 2 to 4 all omitted clear the record's target; arguments 3 and 4 omitted
 * offer the record found for argument 2 as the target, otherwise the point
 * arguments 2 to 4 / 100. Only records of kind 1 or 4 take an offer (0 is
 * returned for the others). Returns 1.
 */
int enepc_001F60B8(int id)
{
  int indices[2] = { -1, -1 };
  Vector4 point;
  int pointIndex;
  float reach;
  int i;

  if (func_A9F0A8(id) < 4) {
    func_A9CC70(id, 2);
    return 0;
  }
  indices[0] = uwamonoFindIndexByShortKey((short) func_A9F0C0(id, 1));
  if (indices[0] == -1) {
    return 1;
  }
  if (func_A9F1C0(id, 5) == 0) {
    pointIndex = func_A9F0C0(id, 5);
    reach = (float) func_A9F0C0(id, 6) / 100.0f;
    for (i = 0; i < 10; i++) {
      if (D_0033EE90[i] == 0.0f) {
        D_0033EE00[pointIndex][0] = (float) func_A9F0C0(id, 2) / 100.0f;
        D_0033EE00[pointIndex][1] = (float) func_A9F0C0(id, 3) / 100.0f;
        D_0033EE00[pointIndex][2] = (float) func_A9F0C0(id, 4) / 100.0f;
        D_0033EE90[pointIndex] = reach;
        break;
      }
    }
    return 1;
  }
  if (func_A9F1C0(id, 2) != 0 && func_A9F1C0(id, 3) != 0 && func_A9F1C0(id, 4) != 0) {
    uwamonoTryClearTarget(indices[0], 3);
    return 1;
  }
  if (func_A9F1C0(id, 3) != 0 && func_A9F1C0(id, 4) != 0) {
    indices[1] = uwamonoFindIndexByShortKey((short) func_A9F0C0(id, 2));
  } else {
    indices[1] = -1;
    point.x = (float) func_A9F0C0(id, 2) / 100.0f;
    point.y = (float) func_A9F0C0(id, 3) / 100.0f;
    point.z = (float) func_A9F0C0(id, 4) / 100.0f;
  }
  if (uwamonoRecords[indices[0]].kind != 1 && uwamonoRecords[indices[0]].kind != 4) {
    return 0;
  }
  if (indices[1] == -1) {
    uwamonoOfferTarget(indices[0], 3, -1, &point);
  } else {
    uwamonoOfferTarget(indices[0], 3, indices[1], 0);
  }
  return 1;
}

void func_00205FD8(int, int, int);

/*
 * Returns 0 after func_A9CC70 when func_A9F0A8 does not report 2. Otherwise
 * passes func_A9F0C0's values for the id to func_00205FD8 and returns 1.
 */
int enepc_001F6420(int id)
{
  int firstArg;
  int convertedArg;

  if (func_A9F0A8(id) != 2) {
    func_A9CC70(id, 2);
    return 0;
  }
  /* script argument 1 of this id, converted by uwamonoFindIndexByShortKey */
  firstArg = func_A9F0C0(id, 1);
  convertedArg = uwamonoFindIndexByShortKey(firstArg);
  func_00205FD8(convertedArg, 1, func_A9F0C0(id, 2));
  return 1;
}

/* src/main/get.c defines it with two parameters; this caller passes the
 * actor's moving angle in $f12 as a third. */
void vectorCopyPoint(Vector4 *destination, const Vector4 *source, float angle);
void uwamonoSetAction(int slot_index, int action);
void uwamonoRebuildTrail(int slot_index);

/*
 * Returns 0 after func_A9CC70 when func_A9F0A8 does not report 1. Otherwise
 * looks up func_A9F0C0's first argument with uwamonoFindIndexByShortKey; if
 * found and the slot's state is in action 5, clears the action timer, copies
 * the actor position to the state's anchor and enters action 2 with a rebuilt
 * trail. Returns 1.
 */
int Resigne(int id)
{
  int lookupIndex;
  struct UwamonoSlot *slot;
  struct UwamonoState *state;
  struct XenoActor *actor;

  if (func_A9F0A8(id) != 1) {
    func_A9CC70(id, 2);
    return 0;
  }
  lookupIndex = uwamonoFindIndexByShortKey((short) func_A9F0C0(id, 1));
  if (lookupIndex == -1) {
    return 1;
  }
  slot = &uwamonoSlotTable[lookupIndex];
  state = slot->state;
  actor = slot->actor;
  if (state->action == 5) {
    state->actionTimer = 0;
    vectorCopyPoint(&state->anchor, &actor->position, actor->direction.y);
    uwamonoSetAction(lookupIndex, 2);
    uwamonoRebuildTrail(lookupIndex);
  }
  return 1;
}

/*
 * Returns 0 after func_A9CC70 when func_A9F0A8 does not report 2 or more.
 * Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; if found, for each of the 8 entries of the
 * slot's state array4042[], stores -1 when func_A9F1C0's argument i+2 is
 * nonzero, otherwise stores func_A9F0C0's argument i+1. Returns 1.
 */
int setGroup(int id)
{
  int lookupKey;
  int lookupIndex;
  int i;
  struct UwamonoSlot *slot;

  if (func_A9F0A8(id) < 2) {
    func_A9CC70(id, 2);
    return 0;
  }
  lookupKey = func_A9F0C0(id, 1);
  lookupIndex = uwamonoFindIndexByShortKey((short) lookupKey);
  if (lookupIndex == -1) {
    return 1;
  }
  slot = &uwamonoSlotTable[lookupIndex];
  for (i = 0; i < 8; i++) {
    if (func_A9F1C0(id, i + 2) != 0) {
      slot->state->array4042[i] = -1;
    } else {
      slot->state->array4042[i] = (short) func_A9F0C0(id, i + 1);
    }
  }
  return 1;
}

/* 0x870-byte record of D_003400F4 starting with a pointer to its data block
 * (same layout as src/main/tu086.c). The field at +0x4064 is touched by
 * enepc_001F6680, purpose unmodeled; walk[3] at +0x4078 is the same field as
 * UwamonoMotionTable (include/main/tu051.h). */
struct UwamonoDataBlock {
  unsigned char unmodeled_0000[0x4064];
  short shortValue; /* +0x4064 */
  unsigned char unmodeled_4066[0x4078 - 0x4066];
  int walk[3]; /* +0x4078 */
};

struct UwamonoDataRecord {
  struct UwamonoDataBlock *data;
  unsigned char unmodeled_04[0x870 - 4];
};

extern struct UwamonoDataRecord D_003400F4[];

/*
 * Returns 0 after func_A9CC70 when func_A9F0A8 does not report 2 or more.
 * Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; if found, stores func_A9F0C0's second argument
 * into the state block's +0x4064 field. Returns 1.
 */
int enepc_001F6680(int id)
{
  int lookupKey;
  int lookupIndex;

  if (func_A9F0A8(id) < 2) {
    func_A9CC70(id, 2);
    return 0;
  }
  lookupKey = func_A9F0C0(id, 1);
  lookupIndex = uwamonoFindIndexByShortKey((short) lookupKey);
  if (lookupIndex == -1) {
    return 1;
  }
  D_003400F4[lookupIndex].data->shortValue = func_A9F0C0(id, 2);
  return 1;
}

/*
 * Returns 0 after func_A9CC70 when func_A9F0A8 does not report 4 or more.
 * Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; if found, stores func_A9F0C0's arguments 2
 * through 4 into the slot's state array4010[0..2]. Returns 1.
 */
int enepc_001F6728(int id)
{
  int lookupKey;
  int lookupIndex;
  int i;
  struct UwamonoSlot *slot;

  if (func_A9F0A8(id) < 4) {
    func_A9CC70(id, 2);
    return 0;
  }
  lookupKey = func_A9F0C0(id, 1);
  lookupIndex = uwamonoFindIndexByShortKey((short) lookupKey);
  if (lookupIndex == -1) {
    return 1;
  }
  slot = &uwamonoSlotTable[lookupIndex];
  for (i = 0; i < 3; i++) {
    slot->state->array4010[i] = func_A9F0C0(id, i + 2);
  }
  return 1;
}

/*
 * Returns 0 after func_A9CC70 when func_A9F0A8 does not report 3 or more.
 * Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; if found, stores func_A9F0C0's third argument
 * into the data block's walk[] array at the index given by the second
 * argument. Returns 1.
 */
int enepc_001F6800(int id)
{
  int lookupKey;
  int lookupIndex;
  int secondArg;
  int thirdArg;

  if (func_A9F0A8(id) < 3) {
    func_A9CC70(id, 2);
    return 0;
  }
  lookupKey = func_A9F0C0(id, 1);
  lookupIndex = uwamonoFindIndexByShortKey((short) lookupKey);
  if (lookupIndex == -1) {
    return 1;
  }
  secondArg = func_A9F0C0(id, 2);
  thirdArg = func_A9F0C0(id, 3);
  D_003400F4[lookupIndex].data->walk[secondArg] = thirdArg;
  return 1;
}

/*
 * Returns 0 after func_A9CC70 when func_A9F0A8 does not report 2 or more.
 * Otherwise resolves script arguments 1 through 4 with
 * uwamonoFindIndexByShortKey (-1 when omitted) and stores the four indices
 * into the group[] of every record found. Returns 1.
 */
int enepc_001F68D0(int id)
{
  int members[4];
  int i;
  int j;
  struct UwamonoState *state;

  if (func_A9F0A8(id) < 2) {
    func_A9CC70(id, 2);
    return 0;
  }
  members[0] = func_A9F1C0(id, 1) != 0 ? -1 : uwamonoFindIndexByShortKey((short) func_A9F0C0(id, 1));
  members[1] = func_A9F1C0(id, 2) != 0 ? -1 : uwamonoFindIndexByShortKey((short) func_A9F0C0(id, 2));
  members[2] = func_A9F1C0(id, 3) != 0 ? -1 : uwamonoFindIndexByShortKey((short) func_A9F0C0(id, 3));
  members[3] = func_A9F1C0(id, 4) != 0 ? -1 : uwamonoFindIndexByShortKey((short) func_A9F0C0(id, 4));
  for (i = 0; i < 4; i++) {
    if (members[i] != -1) {
      state = uwamonoRecords[members[i]].state;
      for (j = 0; j < 4; j++) {
        state->group[j] = members[j];
      }
    }
  }
  return 1;
}

void uwamonoInitState(int slot_index);

/*
 * Returns 0 after func_A9CC70 when func_A9F0A8 does not report 3. Otherwise
 * looks up func_A9F0C0's first argument with uwamonoFindIndexByShortKey; if
 * found, stores argument 2 as the state's mode, the record position as its
 * home, action 1 and argument 3 / 100 as its speed (1000.0f when omitted),
 * then calls uwamonoInitState. Returns 1.
 */
int enepc_001F6A60(int id)
{
  int lookupIndex;
  int mode;
  float speed;

  if (func_A9F0A8(id) != 3) {
    func_A9CC70(id, 2);
    return 0;
  }
  lookupIndex = uwamonoFindIndexByShortKey((short) func_A9F0C0(id, 1));
  if (lookupIndex == -1) {
    return 1;
  }
  mode = func_A9F0C0(id, 2);
  if (func_A9F1C0(id, 3) == 0) {
    speed = (float) func_A9F0C0(id, 3) / 100.0f;
  } else {
    speed = 1000.0f;
  }
  uwamonoRecords[lookupIndex].state->mode = mode;
  uwamonoRecords[lookupIndex].state->home.x = uwamonoRecords[lookupIndex].position[0];
  uwamonoRecords[lookupIndex].state->home.y = uwamonoRecords[lookupIndex].position[1];
  uwamonoRecords[lookupIndex].state->home.z = uwamonoRecords[lookupIndex].position[2];
  uwamonoRecords[lookupIndex].state->speed = speed;
  uwamonoRecords[lookupIndex].state->action = 1;
  uwamonoInitState(lookupIndex);
  return 1;
}

extern void *D_0069D1D8;

/*
 * Returns 0 after func_A9CC70 when func_A9F0A8 does not report 1. Otherwise
 * loads the resource for func_A9F0C0's value for the id, stores it to
 * D_0069D1D8 and returns 1.
 */
int enepc_001F6BB8(int id)
{
  void *resource;
  int resourceId;

  resource = 0;
  if (func_A9F0A8(id) != 1) {
    func_A9CC70(id, 2);
    return 0;
  }
  resourceId = func_A9F0C0(id, 1);
  /* Retries until the resource has loaded. */
  while (GameResLoadID(resourceId, &resource, 0, 0) == -1) {
  }
  D_0069D1D8 = resource;
  return 1;
}

/*
 * Returns 0 after func_A9CC70 when func_A9F0A8 does not report 2 or more.
 * Otherwise looks up func_A9F0C0's first argument with
 * uwamonoFindIndexByShortKey; if found, ORs argument 2 into the draw flags of
 * the actors of kind 1 or 4: of every record whose kind equals argument 3,
 * or of the looked-up record alone when argument 3 is omitted. Returns 1.
 */
int enepc_001F6C38(int id)
{
  int lookupIndex;
  int flags;
  int kind;
  int start;
  int end;
  int i;
  struct XenoActor *actor;

  if (func_A9F0A8(id) < 2) {
    func_A9CC70(id, 2);
    return 0;
  }
  lookupIndex = uwamonoFindIndexByShortKey((short) func_A9F0C0(id, 1));
  if (lookupIndex == -1) {
    return 1;
  }
  flags = func_A9F0C0(id, 2);
  kind = func_A9F1C0(id, 3) != 0 ? -1 : func_A9F0C0(id, 3);
  if (kind == -1) {
    start = lookupIndex;
    end = start + 1;
    if (start == -1) {
      return 1;
    }
  } else {
    start = 0;
    end = 256;
  }
  for (i = start; i < end; i++) {
    if (uwamonoRecords[i].kind == kind || kind == -1) {
      if (uwamonoRecords[i].kind == 4 || uwamonoRecords[i].kind == 1) {
        actor = uwamonoRecords[i].actor;
        if (actor != 0) {
          actor->drawFlags |= flags;
        }
      }
    }
  }
  return 1;
}
