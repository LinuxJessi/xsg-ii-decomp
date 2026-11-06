#include "common.h"
#include "shared.h"
#include "main/tu187.h"
#include "main/tu197.h"

/* A VM object reference (the JNI jobject): a pointer to an object header
 * whose first word is its class. The original type aliased `void *`: the
 * sequence stores of ACT_createChr/ACT_createNPC (near_dir) and the JThread
 * object/script argument slots schedule only under void*'s alias set, not
 * under a char-pointer one. Field slots are reached by byte offset
 * (field->location.offset), so SceneByte views remain for that. */
typedef void *SceneObject;

typedef float Matrix4[4][4];

typedef struct SchedulerState SchedulerState;

typedef float RgVector[4];

typedef struct JntMatrix JntMatrix;

/*
 * JNT_getStaticVal2/JNT_getVal2 index this type by joint slot
 * (`matrix_buffer + index*sizeof(JntMatrix)`, `sll ...,0x6`), so its size is
 * evidenced as 0x40 bytes. The joint tables are read as 4x4 floats by the
 * motion code and as a flat float[16] by JC_face and friends (the w-column
 * stores at [3], [7], [11]). The original accessed them through a union of
 * both views (alias set 0): JC_face/JC_setParent* only reproduce the store
 * order against the quadword copies that way. Unlike xgl_2.h's Matrix there
 * is no quadword member, since that would 16-align the records inside
 * JntWork (shifting every later field).
 */
struct JntMatrix {
    union {
        Matrix4 elements;
        float flat[16];
    };
};

typedef JntMatrix *JntMatrixBuffer;

typedef unsigned int JntMatrixSelect;

typedef struct JntHairIdSource {
    unsigned char unmodeled_00[4];
    unsigned short version;
    unsigned short channel_limit;
    unsigned short primary_id;   /* 0x08 */
    unsigned short secondary_id; /* 0x0A */
    unsigned short additional_primary_count;   /* 0x0C */
    unsigned short additional_secondary_count; /* 0x0E */
    int channel_offset;
} JntHairIdSource;

typedef struct JntChainSlot {
    struct FcvPackAttribute *attribute; /* 0x00 */
    unsigned char unmodeled_04[0x0c];
    Vector4 translation;           /* 0x10 */
    Vector4 scale;                 /* 0x20 */
    Vector4 rotation;              /* 0x30 */
} JntChainSlot;

typedef struct JntProductionOwner JntProductionOwner;

typedef struct JntTransformRecord {
    Vector4 translation;
    Vector4 rotation;
} JntTransformRecord;

typedef struct JntProducer JntProducer;

typedef struct JntInterruptElement JntInterruptElement;

struct JntInterruptElement {
    Vector4 scale;                   /* 0x00 */
    float rotation[3];               /* 0x10 */
    unsigned char unmodeled_1c[4];
    Vector4 translation;             /* 0x20 */
    unsigned short key;
    unsigned short interrupt_channel; /* 0x32, the channel that interrupts this element */
    void *attribute_data;
    unsigned char unmodeled_38[8];
};

struct JntWork {
    JntProducer *producer;         /* +0x000 */
    unsigned short *cursor;        /* +0x004: FCV2 packed-curve state */
    unsigned char unmodeled_008[0x10 - 0x08];
    unsigned int requested_attribute_key; /* 0x10 */
    unsigned char unmodeled_014[0x28 - 0x14];
    struct JntHairIdSource *hair_id_source;
    /*
     * JNT_getStaticVal2/JNT_getVal2 read this as the default matrix slot for
     * their CUR_MATRIX_Get call and unconditionally increment it once per
     * call (`lw/sw ...,44($.)`).
     */
    int current_index; /* 0x2C */
    struct JntStaticChannel *channels; /* +0x030 */
    void *model;
    /*
     * General flags word: JNT_setFlags writes it and JNT_getFlags reads it
     * (`sw $4,56($1)` / `lw $2,56($2)` off the 0x70000000 base). Callers
     * ACT_resetMatrix, ACT_updateMotionSub, ACT_modelDrawSub, ACT_setMotion
     * and ACT_setMotion2 read and write it around the joint update.
     */
    unsigned int flags;            /* +0x038 */
    unsigned short channel_index;  /* +0x03c */
    unsigned char unmodeled_03e[2];
    Vector4 root_translation;      /* +0x040 */
    Vector4 root_rotation;         /* +0x050 */
    Vector4 root_scale;            /* +0x060 */
    Vector4 channel_translation; /* 0x070 */
    Vector4 channel_rotation;    /* 0x080 */
    Vector4 channel_scale;       /* 0x090 */
    /* 0x0A0: 0x40-byte chain records; logical capacity is unrecovered. */
    JntChainSlot chain_slots[16];
    unsigned char unmodeled_4a0[0x4bc - 0x4a0];
    JntMatrixSelect matrix_select;
    unsigned short matrix_index;   /* +0x4c0 */
    short chain_base_index;        /* 0x4C2 */
    unsigned char unmodeled_4c4[0x4d0 - 0x4c4];
    Matrix4 transform_basis; /* 0x4D0: basis and chain origin */
    unsigned char unmodeled_510[0x720 - 0x510];
    JntMatrix root_matrix;
    JntMatrix interpolation_root;
    /*
     * JNT_getStaticVal2/JNT_getVal2 read this as the matrix table their
     * CUR_MATRIX_Set/Get calls index by joint slot (`lw ...,1952($.)`);
     * JNT_setMatrix/JNT_setMatrix2 write it through JNT_MATRIX_BUFFER_OFFSET
     * above (same 0x7A0 slot, kept as a raw offset there for the codegen
     * reason given on that macro).
     */
    JntMatrix *matrix_buffer; /* 0x7A0 */
    JntMatrixBuffer interp_matrix; /* 0x7A4, JNT_setInterpMatrix */
    JntTransformRecord (*static_value_records)[];    /* 0x7A8, JNT_setCurve arg0; read by JNT_getStaticVal */
    void *value_records;           /* 0x7AC, JNT_setCurve arg1; read by JNT_getVal */
    Matrix4 root_correction;       /* 0x7B0 */
    unsigned int attribute_key;
    unsigned int attribute_difference; /* 0x7F4 */
    unsigned char unmodeled_7f8[4];
    float current_frame;           /* +0x7fc */
    float interpolation; /* 0x800, JNT_setInterpMatrix */
    float chain_length;            /* 0x804 */
    struct JntChannelTag *current_value; /* +0x808: an attribute's tag words */
    char *attribute_name;          /* +0x80c */
    /*
     * JNT_animSetFlags writes the animation flags word at 0x810 and
     * JNT_setClipR writes the clipping radius right after it at 0x814;
     * ACT_updateMotionSub supplies both (JNT_animSetFlags also from
     * ACT_modelDrawSub).
     */
    unsigned int animation_flags;  /* +0x810 */
    float clip_radius; /* 0x814 */
    JntInterruptElement *elements;
    int channel_start_index;       /* +0x81c */
    JntProductionOwner *owner;     /* +0x820 */
    unsigned char unmodeled_824[0x0c];
    JntInterruptElement default_element; /* +0x830: default element storage */
};

typedef struct JntWork JntWork;

typedef struct JntStaticChannel JntStaticChannel;

typedef void *(*JntProductionConsumer)(JntWork *work,
                                      JntStaticChannel *channels,
                                      int argument);

/*
 * The joint producer embedded in Actor at +0x790 (Actor.jointProducer, the
 * only producer JNT_initProducer is given). Its tail words are the actor's
 * per-model caches that JNT_initProducer resets: ACT_getAccessories caches
 * JNT_getAccessories in accessory_joints, ACT_getMoveElement caches
 * JNT_getMoveElement in move_element_id (-1 = not looked up yet).
 */
struct JntProducer {
    int count;              /* 0x00 */
    int consumer_ids[8];
    JntProductionConsumer consumers[8]; /* 0x24, called by JNT_startProduction */
    int consumer_arguments[8];         /* 0x44 */
    int pending_output;     /* 0x64: work->flags after a production; ACT_updateMotionSub tests 0x10/0x40 */
    struct ActorAccessoryJointTable *accessory_joints; /* 0x68 */
    int move_element_id;    /* 0x6C */
    /* 0x70: entries in deferred_limits; JNT_startProduction clears it and
     * JNT_getVal2/JNT_getVal_HOGE2 append the shape binding descriptors that
     * follow a channel's rotation-limit header, which ACT_modelDrawSub walks. */
    int deferred_count;
    struct ActorShapeBinding *deferred_limits[8]; /* 0x74 */
};

typedef union JntRotationPayload {
    float values[3];
    int bits[3];
} JntRotationPayload;

struct JntStaticChannel {
    unsigned short type;           /* 0x00 */
    unsigned short interpolation_flags; /* 0x02 */
    int rotation_limits_offset;
    unsigned char unmodeled_08[6];
    unsigned short matrix_index;   /* 0x0E */
    Vector4 translation;           /* 0x10: quadword translation payload */
    JntRotationPayload rotate;     /* 0x20 */
    unsigned char unmodeled_2C[0x0C];
    float translation_direction;  /* 0x38: sign selects the chain direction */
    unsigned char unmodeled_3c[4];
};

/*
 * main/tu149's light editor (DB_lightWrite, updateLight) indexes these
 * vectors as float[4] rows (component offsets fold into the row's index, not
 * the load displacement), so each vector also has its RgVector row.
 */
typedef struct StudioLightEntry {
    union { Vector4 color; RgVector color_row; };
    union { Vector4 direction; RgVector direction_row; };
} StudioLightEntry;

/* The light block of a studio record, also the light record of an Actor
 * (+0x510). Whole-block copies (game.c GameStateSave/RestoreCameraLight) are
 * ld/sd, while the Studio records holding it are only 8-aligned (xgl_studio
 * .bss): the block is 8-aligned, which the doubleword view of its first vector
 * gives. */
typedef struct StudioLight {
    union {
        struct {
            union { Vector4 ambient_color; RgVector ambient_color_row; u64 ambient_color_words[2]; };
            StudioLightEntry lights[3];
        };
        Vector4 parameters[7];          /* the seven quadwords xglLightCalcMatrix loads: ambient, then three colour/direction pairs */
    };
    float normal_matrix[4][4];          /* +0x70: map_1.c's MapDrawLight view */
    float color_modifier_matrix[4][4];  /* +0xb0: map_1.c's MapDrawLight view */
} StudioLight;

/* Enemy parameter record at Actor +0xc0. Member offsets are relative to +0xc0
 * (actor offset = +0xc0 + member offset); the record ends at +0x228, the end of
 * the largest view (EnemyParamsInitView in enemy_system_init.h). Embedded by the
 * TU-local Actor views of the same bytes: ActorEnemyWorkView (enemy_1.c),
 * ActorEnemyDetectView (set_motion.c, hearing), EncountActor (set_motion.c) and
 * ActorPauseView (pause_menu_page_enemy.h, parameters). Views that keep their own
 * declaration: EnemyParamsInitView (ACT_createEnemy, move_kind stored as a
 * halfword) and EnemyParamsActionView (enemy_3.c, idle/reaction motion ids).
 * A member whose readers disagree on width or signedness is not a union here:
 * GCC 2.96 schedules union-member accesses differently (measured on enemy_1.c),
 * so the TU keeps its own EnemyParams*View declaration instead. */
typedef struct EnemyParams {
    float route_step_length;                /* +0x00 */
    float chase_distance;                   /* +0x04 */
    float move_speed;                       /* +0x08: pause menu label "movement speed" */
    float chase_speed;                      /* +0x0c */
    float sight_angle;                      /* +0x10 */
    float sight_distance;                   /* +0x14 */
    short stiff_duration;                   /* +0x18: pause menu stiffness row */
    short pause_interval;                   /* +0x1a */
    short threat_duration;                  /* +0x1c: pause menu threat row */
    short threat_interval;                  /* +0x1e */
    short turn_duration;                    /* +0x20 */
    short gravity_mode;                     /* +0x22 */
    float locator_distance;                 /* +0x24 */
    short wall_attribute;                   /* +0x28 */
    unsigned char move_kind;                /* +0x2a: Enemy_Init loads one byte */
    unsigned char unmodeled_2b;
    int preset_index;                       /* +0x2c: -1 when no AdrsEnemy preset */
    float *route_values;                    /* +0x30 */
    int route_value_count;                  /* +0x34 */
    Vector4 *homing_values;                 /* +0x38 */
    int homing_value_count;                 /* +0x3c */
    float body_radius;                      /* +0x40 */
    int ex_motion_resource;                 /* +0x44: -1 means no ACT_initExMotion */
    float value_108;                        /* +0x48 */
    int event_type;                         /* +0x4c */
    short monster_set_numbers[8];           /* +0x50: Check_Encount picks one, -1 is empty */
    int value_120;                          /* +0x60 */
    int locator_flags;                      /* +0x64 */
    float hearing_distance;                 /* +0x68: Enemy_FindByEar radius */
    short encounter_event_flags;            /* +0x6c: low byte copied to cfEvent */
    short light_level;                      /* +0x6e */
    short default_motion_table[7];          /* +0x70: Get_DefaultMotion */
    unsigned char unmodeled_7e[0x182];
    int value_2c0;                          /* +0x200 */
    int value_2c4;                          /* +0x204 */
    unsigned char unmodeled_208[0x18];
    short linked_actors[4];                 /* +0x220: Enemy_After_Battle */
} EnemyParams;

typedef struct ActorAnimSlot {
    unsigned int flags;
    float frame;
    float speed;
    float reverseLimit;
    float forwardLimit;
    unsigned short currentDataId;
    unsigned short nextDataId;
    unsigned short channelMask; /* +0x18 */
    unsigned short bufferIndex; /* +0x1a */
    unsigned char unmodeled_01c[0x22 - 0x1c];
    unsigned short animQueuedFlags;
} ActorAnimSlot;

typedef struct ActorAnimationState {
    ActorAnimSlot slot;
    float blendDuration; /* +0x24 */
    float blendEndFrame; /* +0x28 */
    void *animData;
    void *animUserData;
    union {
        void *animPack;                    /* +0x34: act_2.c clears it */
        struct FpkFcvHeader *fcvHeader;    /* +0x34: the FPK pack hair_test.c counts */
    };
    unsigned char unmodeled_38[0x48 - 0x38]; /* +0x38 */
    float blendWork[4]; /* +0x48 */
    unsigned char unmodeled_58[0x60 - 0x58]; /* +0x58 */
    unsigned short previousDataId; /* +0x60 */
} ActorAnimationState;

typedef struct ActorAccessoryJointTable {
    unsigned char unmodeled_00[0x20];
    int jointId[0x29];
} ActorAccessoryJointTable;

typedef struct ActorDrawEnv {
    float fogDistance[4];
    float fogColor[4];
    float lightPosition[3][4];
    float lightColor[3][4];
    unsigned int flags;
    unsigned int renderLevel;
    int pixelAlpha;
    int partAlphaCount;
    /*
     * Per-part pixel alpha, [n][0] alpha and [n][1] part id: a plain short
     * array, as Java_xeno_Chr_pixelAlphaParts__II's stores show (a struct
     * element moves the field offset out of the store displacements).
     */
    short partAlpha[4][2];
    float transparency;                                /* +0xa0: Chr peers' first filter float */
    float filter_param[3];                             /* +0xa4: the other three filter floats */
    float cameraCullDistance;
    unsigned char unmodeled_b4[4];
    unsigned long long alpha;
} ActorDrawEnv;

typedef struct ActorAttachment {
    short type;
    short joint;
    void **matrix;
} ActorAttachment;

typedef struct ActorModel {
    unsigned char unmodeled_00[0x54];
    void *matrixResource;
} ActorModel;

typedef struct ActorModelState {
    ActorModel model;
    Matrix4 *jointMatrices;
} ActorModelState;

/* The Java argument copy area of a Chr peer (Actor +0xc0..+0x4bf, ACTOR_ARGS):
 * a struct member, not a union, so its fields keep their own alias sets.
 * Comments give the Actor offset; enemy_2.c's Enemy_Command_Action and
 * Start_Enemy_Command keep &actor->args in a base register. */
typedef struct ActorJavaArgs {
    unsigned char unmodeled_00[0x18];
    u16 idle_motion_id;                  /* +0xd8: Enemy_ActionReady reads it as the idle motion */
    short action_frame;                  /* +0xda: Enemy_Command_Action clears it */
    u16 reaction_motion_id;              /* +0xdc: Enemy_ActionReady reads it as the reaction motion */
    short action_duration;               /* +0xde */
    unsigned char unmodeled_20[0x2c - 0x20];
    int command_code;                    /* +0xec: Enemy_Command_Code stores it before Enemy_Init */
    unsigned char unmodeled_30[0x4c - 0x30];
    int methodArg;                       /* +0x10c: DrillHitActor's enemy-broken method argument */
    unsigned char unmodeled_50[0x60 - 0x50];
    int command_count;                   /* +0x120: queued enemy commands */
    u32 locator_flags;                   /* +0x124 */
    unsigned char unmodeled_68[0x80 - 0x68];
    unsigned int command_type[16];       /* +0x140 */
    int command_parameter[5][16];        /* +0x180: five parameter rows */
    unsigned char unmodeled_200[0x400 - 0x200];
} ActorJavaArgs;

/*
 * What the menu's model actors (MenuModelUnitActorSet, ACT_create(-1)) keep
 * in the +0xc0 block instead of Java arguments: the transform the menu
 * controls move, the draw progress and the draw-type hook. The original
 * copies these vectors as two doublewords and keeps the
 * tskMenuModelTaskMain (1, 1, 1, 1) initializer constant only 8-aligned in
 * .rodata (+0x138), so the copy type is the 8-aligned DoublewordAlignedVector4.
 */
struct Actor;

typedef struct ActorMenuControl {
    AlignedVector4 offset;                             /* +0xc0 */
    AlignedVector4 rotation;                           /* +0xd0 */
    AlignedVector4 scale;                              /* +0xe0 */
    int enabled;                                       /* +0xf0 */
    unsigned char unmodeled_34[0xC];
    AlignedVector4 position;                           /* +0x100 */
    unsigned char unmodeled_50[0x10];
    float transparency;                                /* +0x120: menu draw progress */
    void (*drawTypeFunction)(struct Actor *actor);     /* +0x124 */
    int initialMotionId;                               /* +0x128: selected before opening the unit actor */
    unsigned char unmodeled_6C[4];
} ActorMenuControl;

typedef struct Actor {
    u32 flags;                                         /* +0x00 */
    void (*update)(struct Actor *actor);               /* +0x04 */
    void (*draw)(struct Actor *actor);                 /* +0x08 */
    u32 quadword_alignment_gap;                        /* +0x0c */
    AlignedVector4 position;                           /* +0x10 */
    Vector4 previous_position;                         /* +0x20 */
    Vector4 velocity;                                  /* +0x30 */
    Vector4 acceleration;                              /* +0x40 */
    AlignedVector4 rotation;                           /* +0x50 */
    AlignedVector4 scale;                              /* +0x60 */
    AlignedVector4 globalPosition;                     /* +0x70 */
    u8 number;                                         /* +0x80 */
    u8 signal;                                         /* +0x81 */
    u8 kind;                                           /* +0x82 */
    u8 humanKind;                                      /* +0x83 */
    short state84;                                     /* +0x84 */
    short inUseId;                                     /* +0x86 */
    short frame_counter;                              /* +0x88 */
    unsigned char unmodeled_8a[0x90 - 0x8a];
    u8 shadow_kind;                                    /* +0x90 */
    u8 shadowSize;                                     /* +0x91 */
    unsigned char unmodeled_92[0xa0 - 0x92];
    Vector4 shadow_offset;                             /* +0xa0: set_motion's Set_Shadow reads it */
    short last_step_distance;                          /* +0xb0: Sound_FootStep keeps the previous step here */
    unsigned char unmodeled_b2[0xc0 - 0xb2];
    /* The +0xc0 block is kind-specific: Chr peers keep their Java argument
     * copy here, the menu's model actors keep their transform control, and
     * enemies keep their parameter record. */
    union {
        ActorMenuControl menu;                         /* +0xc0 */
        ActorJavaArgs args;                            /* +0xc0 */
        EnemyParams enemy;                             /* +0xc0 */
    };
    SceneObject java_object;                           /* +0x4c0 */
    unsigned char unmodeled_4c4[0x4c8 - 0x4c4];
    /* +0x4c8 is the UnduParam record UnduParamInit fills; its attr_mask word
     * at +0x4d0 is status_flags, so the record is a plain byte array here. */
    unsigned char undulation[8];                       /* +0x4c8 */
    u16 status_flags;                                  /* +0x4d0 */
    unsigned char unmodeled_4d2[0x4dc - 0x4d2];
    float translate_y;                                 /* +0x4dc */
    struct LayoutHeader *data_header;                  /* +0x4e0 */
    unsigned char unmodeled_4e4[0x4e8 - 0x4e4];
    union {
        u64 attribute_flags;                           /* +0x4e8 */
        u32 collision_bits;                            /* +0x4e8: karakama_test.c reads the low word */
    };
    unsigned char unmodeled_4f0[0x510 - 0x4f0];
    StudioLight light;                                 /* +0x510 */
    union {
        Vector4 cleared600[2];                         /* +0x600 */
        u32 look_clear_600[8];                         /* +0x600: the same cleared block as u32s */
    };
    float look_point[3];                               /* +0x620 */
    float look_eye_speed;                              /* +0x62c */
    unsigned char unmodeled_630[0x640 - 0x630];
    union {
        float lookState[11];                           /* +0x640 */
        struct {
            u32 look_clear_640[8];                     /* +0x640 */
            float look_eye_angle[2];                   /* +0x660: lookState[8..9] */
            unsigned char unmodeled_668[4];
        };
    };
    float look_speed;                                  /* +0x66c */
    float shadowClipScale;                             /* +0x670 */
    short look_mode;                                   /* +0x674 */
    u16 windowId;                                      /* +0x676 */
    int look_eye_control;                              /* +0x678 */
    void *look_target;                                 /* +0x67c */
    u16 mapShadowPartIds[8];                           /* +0x680 */
    int mapShadowPartCount;                            /* +0x690 */
    signed char stick_x;                               /* +0x694 */
    signed char stick_y;                               /* +0x695 */
    short pauseCounter;                                /* +0x696 */
    int attachmentCount;                               /* +0x698 */
    int attachmentIndex;                               /* +0x69c */
    ActorAttachment attachments[8];                    /* +0x6a0 */
    signed char attachmentUsed[8];                     /* +0x6e0 */
    unsigned char unmodeled_6e8[0x6f0 - 0x6e8];
    ActorAnimationState animation;                     /* +0x6f0 */
    int transitionFrames;                              /* +0x754 */
    int holdFrames;                                    /* +0x758 */
    unsigned char unmodeled_75c[4];
    Vector4 rootTranslation;                           /* +0x760 */
    unsigned char unmodeled_770[0x790 - 0x770];
    JntProducer jointProducer;                         /* +0x790..+0x823 */
    void *jointMatrices;                               /* +0x824: ACT_updateMPack reads the same word as its linked actor */
    void *previousJointMatrices;                       /* +0x828 */
    void *staticValueRecords;                          /* +0x82c */
    void *valueRecords;                                /* +0x830 */
    unsigned char unmodeled_834[0x840 - 0x834];
    ActorModelState model[1];                          /* +0x840 */
    unsigned char unmodeled_89c[0x8d0 - 0x89c];
    void *modelResource;                               /* +0x8d0 */
    void *matrixResource;                              /* +0x8d4 */
    void *move;                                        /* +0x8d8 */
    void *animPackTables[8];                           /* +0x8dc */
    struct Actor *parent;                              /* +0x8fc */
    int childCount;                                    /* +0x900 */
    struct Actor *children[7];                         /* +0x904 */
    /* children[7..14] (Chr peers keep fifteen, and ACT_setParent does not cap
     * childCount at seven) would be the words that open drawEnv. A union here
     * was measured to reorder the scheduling of drawEnv's stores (chr.c), so the
     * record keeps drawEnv plain and the Chr loops index children as the original
     * does. */
    ActorDrawEnv drawEnv;                              /* +0x920 */
    float sortOffset;                                  /* +0x9e0 */
    float target_angle;                                /* +0x9e4 */
    float body_radius;                                 /* +0x9e8: 0.35f from ACT_init; collision radius and interaction range */
    short look_at_timer;                               /* +0x9ec */
    short look_at_target;                              /* +0x9ee */
    short flash_timer;                                 /* +0x9f0 */
    unsigned char unmodeled_9f2[0x9f4 - 0x9f2];
    /* script.c reads the same words as the JNI method names of the talk and
     * touch scripts (null when the actor has none). */
    union {
        int talk_message;                              /* +0x9f4 */
        const char *talk_method_name;
    };
    union {
        int touch_message;                             /* +0x9f8 */
        const char *touch_method_name;
    };
    unsigned char unmodeled_9fc[0xa00 - 0x9fc];
    AlignedVector4 saved_position;                     /* +0xa00: enemy_1.c's ACT_updateEnemy */
    SchedulerState *effects[8];                       /* +0xa10 */
    short effect_timer;                                /* +0xa30 */
    short effect_slot;                                 /* +0xa32 */
    unsigned char unmodeled_a34[0xa40 - 0xa34];
    u8 moved_hair_mode;                                /* +0xa40 */
    unsigned char unmodeled_a41[3];
    float moved_hair_delay;                            /* +0xa44 */
    unsigned char unmodeled_a48[8];
    Vector4 moved_hair_sway;                           /* +0xa50 */
    union {
        int statea60;                                  /* +0xa60 */
        union Matrix *previous_joint_matrices;         /* +0xa60: EXM reads it as the owner's previous motion matrices */
    };
    unsigned char unmodeled_a64[0xa70 - 0xa64];
} Actor;

/* Actor origin, raised origin, or an accessory coordinate selector. */
typedef enum MEfActorPoint {
    MEF_POINT_ACTOR = -1,
    MEF_POINT_ACTOR_RAISED = -2,
    MEF_POINT_ACCESSORY_COUNT = 0x28
} MEfActorPoint;

extern Matrix4 *MSeGetActorMatrix(Matrix4 *destination, Actor *actor, u32 coordinate);

typedef struct Actor Actor;

int ACT_jointGetAccessories(Actor *actor, unsigned int accessoryId);
extern Matrix4 *MMathRotateMatrixY(Matrix4 *out, Matrix4 *matrix, float angle);
extern const char D_00A512F0[40];
extern const char D_00A51318[40];
extern const char D_00A51340[48];
Matrix4 *MSeGetActorMatrix(Matrix4 *destination, Actor *actor, u32 coordinate);

INCLUDE_ASM("asm/nonmatchings/ov01/tu077", MSeGetActorMatrix);

/*
 * MEfGetActorCoord: the translation row of MEfGetActorMatrix, with w = 1.
 */
Vector4 *MEfGetActorCoord(Vector4 *destination, Actor *actor, u32 coordinate)
{
    Matrix4 matrix;

    MSeGetActorMatrix(&matrix, actor, coordinate);
    __asm__ __volatile__(
        "lqc2 $vf1,48(%1)\n\t"
        "vmove.w $vf1w,$vf0w\n\t"
        "sqc2 $vf1,0(%0)\n\t"
        :
        : "r"(destination), "r"(matrix)
        : "memory"
    );
    return destination;
}

float MMathMakeRandomPI(void);

Vector4 *MEfMakeRandomVector(Vector4 *destination)
{
    destination->x = MMathMakeRandomPI();
    destination->y = MMathMakeRandomPI();
    destination->z = MMathMakeRandomPI();
    destination->w = 1.0f;
    return destination;
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu077", MEfScatterXY);

INCLUDE_ASM("asm/nonmatchings/ov01/tu077", MEfMake4Vertex);

INCLUDE_ASM("asm/nonmatchings/ov01/tu077", MEfMake4Vertex2);

/*
 * The object the step functions below receive. Offsets follow their loads and
 * stores: the phase-flags, update and advance callbacks at +0x08, +0x10 and
 * +0x14, and the step record at +0x20.
 */
typedef struct MEfStepRecord {
    unsigned char unmodeled_00[0x54];
    short step;                              /* +0x54 (object +0x74) */
    unsigned char unmodeled_56[0x70 - 0x56];
    int counter;                             /* +0x70 (object +0x90) */
    unsigned char unmodeled_74[0xb4 - 0x74];
    unsigned short flags_history[25];        /* +0xb4 (object +0xd4): [0] is the flags halfword, [1..24] its history */
} MEfStepRecord;

typedef struct MEfStepObject {
    unsigned char unmodeled_00[0x08];
    void (*flags_callback)(void);
    unsigned char unmodeled_0c[0x04];
    void (*update_callback)(void);
    void (*advance_callback)(void);
    unsigned char unmodeled_18[0x20 - 0x18];
    MEfStepRecord record;                    /* +0x20 */
} MEfStepObject;

void func_00AABDF0(void);
void func_00AABE50(void);
void MEfWaitStep(MEfStepObject *object);

int MEfCreateWait(MEfStepObject *object)
{
    object->flags_callback = func_00AABDF0;
    object->update_callback = func_00AABE50;
    object->record.counter = 0;
    object->advance_callback = MEfWaitStep;
    return 1;
}

void func_00AABDF0(void)
{
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu077", func_00AABDF8);

INCLUDE_ASM("asm/nonmatchings/ov01/tu077", func_00AABE50);


void MEfWaitStep(MEfStepObject *object)
{
    MEfStepRecord *record;

    record = &object->record;
    record->counter = record->counter + 1;
    if (record->counter >= 0x1F) {
        MSeObjRequestFree(object);
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu077", func_00AABEF0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu077", MEfStartFromActor);

INCLUDE_ASM("asm/nonmatchings/ov01/tu077", MEfSampleHermitePath);

int ov01LastValueGet(void);

void MEfSetPhaseFlags(MEfStepObject *object)
{
    MEfStepRecord *record;
    short step;

    record = &object->record;
    if (ov01LastValueGet() == 0) {
        step = record->step;
        record->flags_history[0] = 0U;
        if (step < 0x1E) {
            record->flags_history[0] = 1U;
            if (step == 0) {
                record->flags_history[0] = 0x11U;
            }
            if (step == 0x1D) {
                record->flags_history[0] |= 0x20;
            }
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu077", func_00AAC510);

void MEfAdvanceStep(MEfStepObject *object)
{
    MEfStepRecord *record;
    unsigned short *slot;
    int remaining;
    int next_step;

    record = &object->record;
    if (ov01LastValueGet() != 0) {
        return;
    }
    remaining = 24;
    slot = &record->flags_history[24];
    do {
        remaining -= 1;
        *slot = slot[-1];
        slot -= 1;
    } while (remaining > 0);
    next_step = (unsigned short) record->step + 1;
    record->step = next_step;
    if ((short) next_step >= 0x37) {
        MSeObjRequestFree(object);
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu077", func_00AACDA0);

INCLUDE_ASM("asm/nonmatchings/ov01/tu077", func_00AACE60);

INCLUDE_ASM("asm/nonmatchings/ov01/tu077", func_00AAD030);

void func_00AAD2E8(MEfStepObject *object)
{
    MEfStepRecord *record;
    short step;

    record = &object->record;
    if (ov01LastValueGet() == 0) {
        step = record->step;
        record->flags_history[0] = 0U;
        if (step < 0x1E) {
            record->flags_history[0] = 1U;
            if (step == 0) {
                record->flags_history[0] = 0x11U;
            }
            if (step == 0x1D) {
                record->flags_history[0] |= 0x20;
            }
        }
    }
}

INCLUDE_ASM("asm/nonmatchings/ov01/tu077", func_00AAD350);
