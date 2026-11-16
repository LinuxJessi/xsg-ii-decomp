#include "common.h"
#include "shared.h"

/* The request block of mapProbeInit and mapMoveProbe (get.c's UnduParam): the header word and the attribute word.
   local view: the shared UnduParam of get.h (0x2C bytes, same members) changed the .text bytes of uwamonoStartAreaBlend in a probe; this 0x40-byte view stays. */
typedef struct UnduParam {
    unsigned char unmodeled_00[0x18];
    int header;                         /* +0x18: set from mapFindBlockEntry before the probe */
    unsigned char unmodeled_1C[0x4];
    int attribute;                      /* +0x20: written by the probe */
    unsigned char unmodeled_24[0x40 - 0x24];
} UnduParam;

/* The object a record points at (+0x330): its probe origin and the midpoint tracks of uwamonoUpdateMidpoints. */
typedef struct UwamonoObject {
    unsigned char unmodeled_00[0x10];
    Vector4 position;                   /* +0x10: origin passed to mapMoveProbe */
    unsigned char unmodeled_20[0xCC - 0x20];
    float blend;                        /* +0xCC */
    unsigned char unmodeled_D0[0x190 - 0xD0];
    float midpointA[3][4];              /* +0x190 */
    unsigned char unmodeled_1C0[0x200 - 0x1C0];
    float midpointB[3][4];              /* +0x200 */
    unsigned char unmodeled_230[0x260 - 0x230];
    float midpointC[4];                 /* +0x260 */
    unsigned char unmodeled_270[0x280 - 0x270];
    float midpointD[4];                 /* +0x280 */
} UwamonoObject;

/* One 0x870-byte record of uwamonoRecords: the fields this unit reads and writes. */
typedef struct UwamonoRecord {
    unsigned char unmodeled_00[0xF0];
    float startA[3][4];                 /* +0xF0 */
    float followA[3][4];                /* +0x120 */
    union { float startB[3][4]; };      /* +0x150: alias set 0, so uwamonoStartAreaBlend's cursor stores stay behind these copies */
    float followB[3][4];                /* +0x180 */
    float startD[4];                    /* +0x1B0 */
    float followD[4];                   /* +0x1C0 */
    float blendStart;                   /* +0x1D0 */
    float blendEnd;                     /* +0x1D4 */
    unsigned char unmodeled_1D8[0x1E0 - 0x1D8];
    float startC[4];                    /* +0x1E0 */
    float followC[4];                   /* +0x1F0 */
    unsigned char unmodeled_200[0x222 - 0x200];
    short cursorA[3];                   /* +0x222 */
    short countA[3];                    /* +0x228 */
    short cursorB[3];                   /* +0x22E */
    short countB[3];                    /* +0x234 */
    short cursorD;                      /* +0x23A */
    short countD;                       /* +0x23C */
    short cursorE;                      /* +0x23E */
    short totalE;                       /* +0x240 */
    short cursorC;                      /* +0x242 */
    short countC;                       /* +0x244 */
    unsigned char unmodeled_246[0x248 - 0x246];
    int frameCount;                     /* +0x248 */
    unsigned char unmodeled_24C[0x330 - 0x24C];
    UwamonoObject *object;              /* +0x330 */
    unsigned char unmodeled_334[0x870 - 0x334];
} UwamonoRecord;

extern UwamonoRecord uwamonoRecords[];

/* A midpoint track whose cursor is UWAMONO_TRACK_IDLE is not running; a started track takes UWAMONO_BLEND_STEPS steps. */
#define UWAMONO_TRACK_IDLE (-1)
#define UWAMONO_BLEND_STEPS 5
/* Probe attribute: both bits set means the low byte holds the area index. */
#define UWAMONO_PROBE_AREA_FLAGS 0x500
#define UWAMONO_PROBE_AREA_MASK 0xff
/* gameWork: the midpoint tables are float rows indexed by the frame count (tbl[224 + 12 * count], [416 + ...], [608 + 4 * count], [672 + ...], [736 + count]). */
typedef struct GameWork { unsigned char unmodeled_00[4]; float tbl[1024]; } GameWork;
extern GameWork gameWork;
extern const char D_00685760[];
extern const char D_00685778[];

int mapFindBlockEntry(int, int);
void mapProbeInit(UnduParam *request);
float mapMoveProbe(const Vector4 *position, int mode, UnduParam *request);
void xglFontDebugPrintf(int, int, const char *, ...);
/* local view: the shared Get_MiddlePoint of get.h takes Vector4 pointers; the Vector4 record views change the code of uwamonoUpdateMidpoints. */
void Get_MiddlePoint(float *start, float *follow, int cursor, int count, float *out);

/*
 * Per uwamonoRecords[index] (called from DispMapObject): three Get_MiddlePoint pairs write midpoints to the record's
 * object (the pointer at +0x330) at +0x190 and +0x200; short counters at +0x222..+0x244 pace them, and the float at
 * +0xCC is blended over a frame count.
 */
void uwamonoUpdateMidpoints(int index)
{
    UwamonoRecord *record = &uwamonoRecords[index];
    UwamonoObject *object = record->object;
    union {
        float value[4];   /* alias set 0: the midpoint loads stay behind the cursor store */
    } out;
    int i;

    for (i = 0; i < 3; i++) {
        if (record->cursorA[i] != UWAMONO_TRACK_IDLE) {
            unsigned short cursor;
            Get_MiddlePoint(record->startA[i], record->followA[i], record->cursorA[i], record->countA[i], out.value);
            cursor = record->cursorA[i];
            record->cursorA[i] = (short)(cursor + 1);
            object->midpointA[i][0] = out.value[0];
            object->midpointA[i][1] = out.value[1];
            object->midpointA[i][2] = out.value[2];
            object->midpointA[i][3] = out.value[3];
            if ((short)cursor >= record->countA[i]) {
                record->cursorA[i] = UWAMONO_TRACK_IDLE;
                record->countA[i] = 0;
            }
        }
        if (record->cursorB[i] != UWAMONO_TRACK_IDLE) {
            unsigned short cursor;
            Get_MiddlePoint(record->startB[i], record->followB[i], record->cursorB[i], record->countB[i], out.value);
            cursor = record->cursorB[i];
            record->cursorB[i] = (short)(cursor + 1);
            object->midpointB[i][0] = out.value[0];
            object->midpointB[i][1] = out.value[1];
            object->midpointB[i][2] = out.value[2];
            object->midpointB[i][3] = out.value[3];
            if ((short)cursor >= record->countB[i]) {
                record->cursorB[i] = UWAMONO_TRACK_IDLE;
                record->countB[i] = 0;
            }
        }
    }
    if (record->cursorC != UWAMONO_TRACK_IDLE) {
        unsigned short cursor;
        Get_MiddlePoint(record->startC, record->followC, record->cursorC, record->countC, out.value);
        cursor = record->cursorC;
        record->cursorC = (short)(cursor + 1);
        object->midpointC[0] = out.value[0];
        object->midpointC[1] = out.value[1];
        object->midpointC[2] = out.value[2];
        object->midpointC[3] = out.value[3];
        if ((short)cursor >= record->countC) {
            record->cursorC = UWAMONO_TRACK_IDLE;
            record->countC = 0;
        }
    }
    if (record->cursorD != UWAMONO_TRACK_IDLE) {
        unsigned short cursor;
        Get_MiddlePoint(record->startD, record->followD, record->cursorD, record->countD, out.value);
        cursor = record->cursorD;
        record->cursorD = (short)(cursor + 1);
        object->midpointD[0] = out.value[0];
        object->midpointD[1] = out.value[1];
        object->midpointD[2] = out.value[2];
        object->midpointD[3] = record->followD[3];
        if ((short)cursor >= record->countD) {
            record->cursorD = UWAMONO_TRACK_IDLE;
            record->countD = 0;
        }
    }
    if (record->cursorE != UWAMONO_TRACK_IDLE) {
        short cursor;
        cursor = record->cursorE;
        object->blend = record->blendStart + (record->blendEnd - record->blendStart) * cursor / record->totalE;
        record->cursorE = record->cursorE + 1;
        if (cursor >= record->totalE) {
            record->cursorE = UWAMONO_TRACK_IDLE;
            record->totalE = 0;
        }
    }
}

/*
 * Per uwamonoRecords[index] (called from DispMapObject before uwamonoUpdateMidpoints): copies the object's midpoints back into
 * the record (+0xF0, +0x1B0, +0x1E0, +0xCC) and stores the frame count at +0x248. A debug line goes out when the flags
 * word returned through the stack has 0x500 set.
 */
void uwamonoStartAreaBlend(int index)
{
    UwamonoRecord *record = &uwamonoRecords[index];
    UwamonoObject *object = record->object;
    UnduParam request;
    int attribute;
    int count;
    int i;

    mapProbeInit(&request);
    request.header = mapFindBlockEntry(2560, -1);
    mapMoveProbe(&object->position, 0, &request);
    attribute = request.attribute;
    if ((attribute & UWAMONO_PROBE_AREA_FLAGS) == UWAMONO_PROBE_AREA_FLAGS) {
        xglFontDebugPrintf(192, 20, D_00685760, attribute);
        attribute = request.attribute;
        count = (attribute & UWAMONO_PROBE_AREA_MASK) + 1;
    } else {
        count = 0;
    }
    xglFontDebugPrintf(192, 30, D_00685778, record->frameCount, count, attribute);
    if (record->frameCount != count) {
        for (i = 0; i < 3; i++) {
            record->startA[i][0] = object->midpointA[i][0];
            record->startA[i][1] = object->midpointA[i][1];
            record->startA[i][2] = object->midpointA[i][2];
            record->startA[i][3] = object->midpointA[i][3];
            record->followA[i][0] = gameWork.tbl[224 + 12 * count + 4 * i + 0];
            record->followA[i][1] = gameWork.tbl[224 + 12 * count + 4 * i + 1];
            record->followA[i][2] = gameWork.tbl[224 + 12 * count + 4 * i + 2];
            record->followA[i][3] = gameWork.tbl[224 + 12 * count + 4 * i + 3];
            record->startB[i][0] = object->midpointB[i][0];
            record->startB[i][1] = object->midpointB[i][1];
            record->startB[i][2] = object->midpointB[i][2];
            record->startB[i][3] = object->midpointB[i][3];
            record->followB[i][0] = gameWork.tbl[416 + 12 * count + 4 * i + 0];
            record->followB[i][1] = gameWork.tbl[416 + 12 * count + 4 * i + 1];
            record->followB[i][2] = gameWork.tbl[416 + 12 * count + 4 * i + 2];
            record->followB[i][3] = gameWork.tbl[416 + 12 * count + 4 * i + 3];
            record->cursorA[i] = record->cursorB[i] = 0;
            record->countB[i] = UWAMONO_BLEND_STEPS;
            record->countA[i] = UWAMONO_BLEND_STEPS;
        }
        record->startD[0] = object->midpointD[0];
        record->startD[1] = object->midpointD[1];
        record->startD[2] = object->midpointD[2];
        record->startD[3] = object->midpointD[3];
        record->followD[0] = gameWork.tbl[672 + 4 * count + 0];
        record->followD[1] = gameWork.tbl[672 + 4 * count + 1];
        record->followD[2] = gameWork.tbl[672 + 4 * count + 2];
        record->followD[3] = gameWork.tbl[672 + 4 * count + 3];
        record->blendStart = object->blend;
        record->blendEnd = gameWork.tbl[736 + count];
        record->startC[0] = object->midpointC[0];
        record->startC[1] = object->midpointC[1];
        record->startC[2] = object->midpointC[2];
        record->startC[3] = object->midpointC[3];
        record->followC[0] = gameWork.tbl[608 + 4 * count + 0];
        record->followC[1] = gameWork.tbl[608 + 4 * count + 1];
        record->followC[2] = gameWork.tbl[608 + 4 * count + 2];
        record->followC[3] = gameWork.tbl[608 + 4 * count + 3];
        record->cursorC = 0;
        record->cursorE = 0;
        record->cursorD = 0;
        record->countC = UWAMONO_BLEND_STEPS;
        record->totalE = UWAMONO_BLEND_STEPS;
        record->countD = UWAMONO_BLEND_STEPS;
        record->frameCount = count;
    }
}

/* Returns 0. No caller inside this unit. */
int func_001DF088(void) {
    return 0;
}

/* .rodata strings of this unit (recovered from the original bytes). */
const char D_00685760[24] = "$col00F0F0;\243\311\243\304\306\247 %8x";

const char D_00685778[40] = "$col00F0F0;\270\367ID %2d/%2d:%8x";

const char D_006857A0[16] = "ziggyH01";

const char D_006857B0[16] = "chaosH02";

const char D_006857C0[16] = "jin_sw04";

const char D_006857D0[16] = "jin_sw03";

const char D_006857E0[16] = "jin_sw02";

const char D_006857F0[16] = "zig_sw01";

const char D_00685800[16] = "jr_gun01b_2";

const char D_00685810[16] = "jr_gun01b";

const char D_00685820[16] = "jr_gun01";

const char D_00685830[16] = "jin_sw01";

const char D_00685840[16] = "jin_saya";

const char D_00685850[16] = "chaosH01";

const char D_00685860[16] = "chaosH00";

const char D_00685870[16] = "kos_sw03";

const char D_00685880[16] = "kosmos03";

const char D_00685890[16] = "kosmos02";

const char D_006858A0[16] = "kosmos01";

const char D_006858B0[16] = "kosmos00";

const char D_006858C0[16] = "kosmosH00";

const char D_006858D0[16] = "sh_box01";

const char D_006858E0[16] = "shionH03";

const char D_006858F0[16] = "shionH00";

const char D_00685900[16] = "model\\char\\pc\\";

const char D_00685910[16] = "gaignunH06";

const char D_00685920[16] = "gaignunH05";

const char D_00685930[16] = "gaignunH03";

const char D_00685940[16] = "gaignunH01";

const char D_00685950[16] = "marglisH03";

const char D_00685960[16] = "albeltH07";

const char D_00685970[16] = "albeltH06";

const char D_00685980[16] = "mathewsH00";

const char D_00685990[16] = "hammerH00";

const char D_006859A0[16] = "tonnyH00";

const char D_006859B0[16] = "nephilim01";

const char D_006859C0[16] = "sakuraH00";

const char D_006859D0[16] = "gaignun03";

const char D_006859E0[16] = "gaignunH04";

const char D_006859F0[16] = "gaignunH02";

const char D_00685A00[16] = "gaignunH00";

const char D_00685A10[16] = "marglisH02";

const char D_00685A20[16] = "marglisH01";

const char D_00685A30[16] = "marglisH00";

const char D_00685A40[16] = "albeltH05";

const char D_00685A50[16] = "albeltH04";

const char D_00685A60[16] = "albeltH03";

const char D_00685A70[16] = "albeltH01";

const char D_00685A80[16] = "albeltH00";

const char D_00685A90[16] = "virhelm00";

const char D_00685AA0[16] = "cananH00";

const char D_00685AB0[16] = "mathews00";

const char D_00685AC0[16] = "hammer00";

const char D_00685AD0[16] = "miyuki00";

const char D_00685AE0[16] = "nephilim00";

const char D_00685AF0[16] = "virgil00";

const char D_00685B00[16] = "febronia00";

const char D_00685B10[16] = "citren00";

const char D_00685B20[16] = "voyager01";

const char D_00685B30[16] = "voyager00";

const char D_00685B40[16] = "yuriev00";

const char D_00685B50[16] = "sellers00";

const char D_00685B60[16] = "sakura00";

const char D_00685B70[16] = "shelley00";

const char D_00685B80[16] = "gaignun02";

const char D_00685B90[16] = "gaignun01";

const char D_00685BA0[16] = "gaignun00";

const char D_00685BB0[16] = "pelegri00";

const char D_00685BC0[16] = "marglis01";

const char D_00685BD0[16] = "marglis00";

const char D_00685BE0[16] = "helmer01";

const char D_00685BF0[16] = "helmer00";

const char D_00685C00[16] = "albelt03";

const char D_00685C10[16] = "albelt02";

const char D_00685C20[16] = "albelt01";

const char D_00685C30[16] = "albelt00";

const char D_00685C40[16] = "allenH01";

const char D_00685C50[16] = "model\\char\\npc\\";

const char D_00685C60[16] = "cf_bos010";

const char D_00685C70[16] = "cf_uso010";

const char D_00685C80[16] = "cf_uso009";

const char D_00685C90[16] = "cf_uso008";

const char D_00685CA0[16] = "cf_uso007";

const char D_00685CB0[16] = "cf_uso004";

const char D_00685CC0[16] = "cf_uso003";

const char D_00685CD0[16] = "vectech00";

const char D_00685CE0[16] = "hakase02";

const char D_00685CF0[16] = "master00";

const char D_00685D00[16] = "pelesol00";

const char D_00685D10[16] = "cleanch00";

const char D_00685D20[16] = "breadch00";

const char D_00685D30[16] = "inngirl00";

const char D_00685D40[16] = "hakase00";

const char D_00685D50[16] = "hakase01";

const char D_00685D60[16] = "brealm01";

const char D_00685D70[16] = "brealm00";

const char D_00685D80[16] = "gfgass300";

const char D_00685D90[16] = "gfgass200";

const char D_00685DA0[16] = "gfgass100";

const char D_00685DB0[16] = "cf_uso002";

const char D_00685DC0[16] = "cf_uso001";

const char D_00685DD0[16] = "urtvmH00";

const char D_00685DE0[24] = "model\\char\\cf_npc\\";

const char D_00685DF8[16] = "ballet00";

const char D_00685E08[16] = "kitten00";

const char D_00685E18[16] = "kingcat00";

const char D_00685E28[16] = "singer00";

const char D_00685E38[16] = "doctor00";

const char D_00685E48[16] = "woman401";

const char D_00685E58[16] = "woman400";

const char D_00685E68[16] = "woman301";

const char D_00685E78[16] = "woman300";

const char D_00685E88[16] = "woman201";

const char D_00685E98[16] = "woman200";

const char D_00685EA8[16] = "woman101";

const char D_00685EB8[16] = "woman100";

const char D_00685EC8[24] = "model\\char\\citizen\\";

const char D_00685EE0[16] = "pcr05_sr01";

const char D_00685EF0[16] = "pcr05_br01";

const char D_00685F00[16] = "pcr05_rf01";

const char D_00685F10[16] = "pcr02_rp02";

const char D_00685F20[16] = "pcr02_rp01";

const char D_00685F30[16] = "pcr05_mg01";

const char D_00685F40[16] = "pcr05_sh01";

const char D_00685F50[24] = "model\\robo\\pc_robo\\";

const char D_00685F68[16] = "npcr10_sh01";

const char D_00685F78[16] = "npcr10_rf01";

const char D_00685F88[16] = "npcr17_sw01";

const char D_00685F98[16] = "npcr06_sw01";

const char D_00685FA8[16] = "npcr06_mg01";

const char D_00685FB8[16] = "npcr06_rf01";

const char D_00685FC8[16] = "npcr13_sw01";

const char D_00685FD8[16] = "npcr12_mg01";

const char D_00685FE8[16] = "npcr12_rf01";

const char D_00685FF8[24] = "model\\robo\\npc_robo\\";

const char D_00686010[16] = "gattai81";

const char D_00686020[16] = "gattai80";

const char D_00686030[16] = "gattai79";

const char D_00686040[16] = "gattai78";

const char D_00686050[16] = "gattai77";

const char D_00686060[16] = "gattai76";

const char D_00686070[16] = "gattai75";

const char D_00686080[16] = "gattai74";

const char D_00686090[16] = "gattai73";

const char D_006860A0[16] = "gattai72";

const char D_006860B0[16] = "gattai71";

const char D_006860C0[16] = "gattai70";

const char D_006860D0[16] = "gattai69";

const char D_006860E0[16] = "gattai68";

const char D_006860F0[16] = "gattai67";

const char D_00686100[16] = "gattai66";

const char D_00686110[16] = "gattai65";

const char D_00686120[16] = "gattai64";

const char D_00686130[16] = "gattai63";

const char D_00686140[16] = "gattai62";

const char D_00686150[16] = "gattai61";

const char D_00686160[16] = "gattai60";

const char D_00686170[16] = "gattai59";

const char D_00686180[16] = "gattai58";

const char D_00686190[16] = "gattai57";

const char D_006861A0[16] = "gattai56";

const char D_006861B0[16] = "gattai55";

const char D_006861C0[16] = "gattai54";

const char D_006861D0[16] = "gattai53";

const char D_006861E0[16] = "gattai52";

const char D_006861F0[16] = "gattai51";

const char D_00686200[16] = "gattai50";

const char D_00686210[16] = "gattai49";

const char D_00686220[16] = "gattai48";

const char D_00686230[16] = "gattai47";

const char D_00686240[16] = "gattai46";

const char D_00686250[16] = "gattai45";

const char D_00686260[16] = "gattai44";

const char D_00686270[16] = "gattai43";

const char D_00686280[16] = "gattai42";

const char D_00686290[16] = "gattai41";

const char D_006862A0[16] = "gattai40";

const char D_006862B0[16] = "gattai39";

const char D_006862C0[16] = "gattai38";

const char D_006862D0[16] = "gattai37";

const char D_006862E0[16] = "gattai36";

const char D_006862F0[16] = "gattai35";

const char D_00686300[16] = "gattai34";

const char D_00686310[16] = "gattai33";

const char D_00686320[16] = "gattai32";

const char D_00686330[16] = "gattai31";

const char D_00686340[16] = "gattai30";

const char D_00686350[16] = "gattai29";

const char D_00686360[16] = "gattai28";

const char D_00686370[16] = "gattai27";

const char D_00686380[16] = "gattai26";

const char D_00686390[16] = "gattai25";

const char D_006863A0[16] = "gattai24";

const char D_006863B0[16] = "gattai23";

const char D_006863C0[16] = "gattai22";

const char D_006863D0[16] = "gattai21";

const char D_006863E0[16] = "gattai20";

const char D_006863F0[16] = "gattai19";

const char D_00686400[16] = "gattai18";

const char D_00686410[16] = "gattai17";

const char D_00686420[16] = "gattai16";

const char D_00686430[16] = "gattai15";

const char D_00686440[16] = "gattai14";

const char D_00686450[16] = "gattai13";

const char D_00686460[16] = "gattai12";

const char D_00686470[16] = "gattai11";

const char D_00686480[16] = "gattai10";

const char D_00686490[16] = "gattai09";

const char D_006864A0[16] = "gattai08";

const char D_006864B0[16] = "gattai07";

const char D_006864C0[16] = "gattai06";

const char D_006864D0[16] = "gattai05";

const char D_006864E0[16] = "gattai04";

const char D_006864F0[16] = "gattai03";

const char D_00686500[16] = "gattai02";

const char D_00686510[16] = "gattai01";

const char D_00686520[24] = "model\\robo\\gattai_robo\\";

const char D_00686538[16] = "uso001_sg01";

const char D_00686548[16] = "uso001_rf01";

const char D_00686558[16] = "model\\enemy\\";

const char D_00686568[16] = "E2_OMG21";

const char D_00686578[16] = "E2_KML11";

const char D_00686588[16] = "E2_MOM04";

const char D_00686598[16] = "E2_EVS09";

const char D_006865A8[16] = "E2_EVS08";

const char D_006865B8[16] = "E2_EVS07";

const char D_006865C8[16] = "E2_EVS06";

const char D_006865D8[16] = "E2_EVS05";

const char D_006865E8[16] = "E2_EVS04";

const char D_006865F8[16] = "E2_EVS03b";

const char D_00686608[16] = "E2_EVS03";

const char D_00686618[16] = "E2_EVS02b";

const char D_00686628[16] = "E2_EVS02";

const char D_00686638[16] = "E2_EVS01";

const char D_00686648[16] = "E2_EVE91";

const char D_00686658[16] = "E2_EVE90";

const char D_00686668[16] = "E2_EVE89";

const char D_00686678[16] = "E2_EVE88";

const char D_00686688[16] = "E2_EVE87";

const char D_00686698[16] = "E2_EVE86";

const char D_006866A8[16] = "E2_EVE85";

const char D_006866B8[16] = "E2_EVE84";

const char D_006866C8[16] = "E2_EVE83";

const char D_006866D8[16] = "E2_EVE82";

const char D_006866E8[16] = "E2_EVE81";

const char D_006866F8[16] = "E2_EVE80";

const char D_00686708[16] = "E2_EVE79";

const char D_00686718[16] = "E2_EVE78";

const char D_00686728[16] = "E2_EVE77";

const char D_00686738[16] = "E2_EVE76";

const char D_00686748[16] = "E2_EVE75";

const char D_00686758[16] = "E2_EVE74";

const char D_00686768[16] = "E2_EVE73";

const char D_00686778[16] = "E2_EVE72";

const char D_00686788[16] = "E2_EVE71";

const char D_00686798[16] = "E2_EVE70";

const char D_006867A8[16] = "E2_EVE69";

const char D_006867B8[16] = "E2_EVE68";

const char D_006867C8[16] = "E2_EVE67";

const char D_006867D8[16] = "E2_EVE66";

const char D_006867E8[16] = "E2_EVE65";

const char D_006867F8[16] = "E2_EVE64";

const char D_00686808[16] = "E2_EVE63";

const char D_00686818[16] = "E2_EVE62";

const char D_00686828[16] = "E2_EVE61";

const char D_00686838[16] = "E2_EVE60";

const char D_00686848[16] = "E2_EVE59";

const char D_00686858[16] = "E2_EVE58";

const char D_00686868[16] = "E2_EVE57";

const char D_00686878[16] = "E2_EVE56";

const char D_00686888[16] = "E2_EVE55";

const char D_00686898[16] = "E2_EVE54";

const char D_006868A8[16] = "E2_EVE53";

const char D_006868B8[16] = "E2_EVE52";

const char D_006868C8[16] = "E2_EVE51";

const char D_006868D8[16] = "E2_EVE50";

const char D_006868E8[16] = "E2_EVE49";

const char D_006868F8[16] = "E2_EVE48";

const char D_00686908[16] = "E2_EVE47";

const char D_00686918[16] = "E2_EVE46";

const char D_00686928[16] = "E2_EVE45";

const char D_00686938[16] = "E2_EVE44";

const char D_00686948[16] = "E2_EVE43";

const char D_00686958[16] = "E2_EVE42";

const char D_00686968[16] = "E2_EVE41";

const char D_00686978[16] = "E2_EVE40";

const char D_00686988[16] = "E2_EVE39";

const char D_00686998[16] = "E2_EVE38";

const char D_006869A8[16] = "E2_EVE37";

const char D_006869B8[16] = "E2_EVE36";

const char D_006869C8[16] = "E2_EVE35";

const char D_006869D8[16] = "E2_EVE34";

const char D_006869E8[16] = "E2_EVE33";

const char D_006869F8[16] = "E2_EVE32";

const char D_00686A08[16] = "E2_EVE31";

const char D_00686A18[16] = "E2_EVE30";

const char D_00686A28[16] = "E2_EVE29";

const char D_00686A38[16] = "E2_EVE28";

const char D_00686A48[16] = "E2_EVE27";

const char D_00686A58[16] = "E2_EVE26";

const char D_00686A68[16] = "E2_EVE25";

const char D_00686A78[16] = "E2_EVE24";

const char D_00686A88[16] = "E2_EVE23";

const char D_00686A98[16] = "E2_EVE22";

const char D_00686AA8[16] = "E2_EVE21";

const char D_00686AB8[16] = "E2_EVE20";

const char D_00686AC8[16] = "E2_EVE19";

const char D_00686AD8[16] = "E2_EVE18";

const char D_00686AE8[16] = "E2_EVE17";

const char D_00686AF8[16] = "E2_EVE16";

const char D_00686B08[16] = "E2_EVE15";

const char D_00686B18[16] = "E2_EVE14";

const char D_00686B28[16] = "E2_EVE13";

const char D_00686B38[16] = "E2_EVE12";

const char D_00686B48[16] = "E2_EVE11";

const char D_00686B58[16] = "E2_EVE10";

const char D_00686B68[16] = "E2_EVE09";

const char D_00686B78[16] = "E2_EVE08";

const char D_00686B88[16] = "E2_EVE07";

const char D_00686B98[16] = "E2_EVE06";

const char D_00686BA8[16] = "E2_EVE05";

const char D_00686BB8[16] = "E2_EVE04";

const char D_00686BC8[16] = "E2_EVE03";

const char D_00686BD8[16] = "E2_EVE02";

const char D_00686BE8[16] = "E2_EVE01";

const char D_00686BF8[16] = "E2_EXD01";

const char D_00686C08[16] = "E2_EXC11";

const char D_00686C18[16] = "E2_EXC10";

const char D_00686C28[16] = "E2_EXC09";

const char D_00686C38[16] = "E2_EXC08";

const char D_00686C48[16] = "E2_EXC07";

const char D_00686C58[16] = "E2_EXC06";

const char D_00686C68[16] = "E2_EXC05";

const char D_00686C78[16] = "E2_EXC04";

const char D_00686C88[16] = "E2_EXC03";

const char D_00686C98[16] = "E2_EXC02";

const char D_00686CA8[16] = "E2_EXC01";

const char D_00686CB8[16] = "E2_EXB07";

const char D_00686CC8[16] = "E2_EXB06";

const char D_00686CD8[16] = "E2_EXB05";

const char D_00686CE8[16] = "E2_EXB04";

const char D_00686CF8[16] = "E2_EXB03";

const char D_00686D08[16] = "E2_EXB02";

const char D_00686D18[16] = "E2_EXB01";

const char D_00686D28[16] = "E2_EXA07";

const char D_00686D38[16] = "E2_EXA06";

const char D_00686D48[16] = "E2_EXA05";

const char D_00686D58[16] = "E2_EXA04";

const char D_00686D68[16] = "E2_EXA03";

const char D_00686D78[16] = "E2_EXA02";

const char D_00686D88[16] = "E2_EXA01";

const char D_00686D98[16] = "E2_JSY08";

const char D_00686DA8[16] = "E2_JSY07";

const char D_00686DB8[16] = "E2_JSY06";

const char D_00686DC8[16] = "E2_JSY05";

const char D_00686DD8[16] = "E2_JSY04";

const char D_00686DE8[16] = "E2_JSY03";

const char D_00686DF8[16] = "E2_JSY02";

const char D_00686E08[16] = "E2_JSY01";

const char D_00686E18[16] = "E2_OMG20";

const char D_00686E28[16] = "E2_OMG19";

const char D_00686E38[16] = "E2_OMG18";

const char D_00686E48[16] = "E2_OMG17";

const char D_00686E58[16] = "E2_OMG16";

const char D_00686E68[16] = "E2_OMG15";

const char D_00686E78[16] = "E2_OMG14";

const char D_00686E88[16] = "E2_OMG13";

const char D_00686E98[16] = "E2_OMG12";

const char D_00686EA8[16] = "E2_OMG11";

const char D_00686EB8[16] = "E2_OMG10";

const char D_00686EC8[16] = "E2_OMG09";

const char D_00686ED8[16] = "E2_OMG08";

const char D_00686EE8[16] = "E2_OMG07";

const char D_00686EF8[16] = "E2_OMG06";

const char D_00686F08[16] = "E2_OMG05";

const char D_00686F18[16] = "E2_OMG04";

const char D_00686F28[16] = "E2_OMG03";

const char D_00686F38[16] = "E2_OMG02";

const char D_00686F48[16] = "E2_OMG01";

const char D_00686F58[16] = "E2_RAV15";

const char D_00686F68[16] = "E2_RAV14";

const char D_00686F78[16] = "E2_RAV13";

const char D_00686F88[16] = "E2_RAV12";

const char D_00686F98[16] = "E2_RAV11";

const char D_00686FA8[16] = "E2_RAV10";

const char D_00686FB8[16] = "E2_RAV09";

const char D_00686FC8[16] = "E2_RAV08";

const char D_00686FD8[16] = "E2_RAV07";

const char D_00686FE8[16] = "E2_RAV06";

const char D_00686FF8[16] = "E2_RAV05";

const char D_00687008[16] = "E2_RAV04";

const char D_00687018[16] = "E2_RAV03";

const char D_00687028[16] = "E2_RAV02";

const char D_00687038[16] = "E2_RAV01";

const char D_00687048[16] = "E2_SUI12";

const char D_00687058[16] = "E2_SUI11";

const char D_00687068[16] = "E2_SUI10";

const char D_00687078[16] = "E2_SUI09";

const char D_00687088[16] = "E2_SUI08";

const char D_00687098[16] = "E2_SUI07";

const char D_006870A8[16] = "E2_SUI06";

const char D_006870B8[16] = "E2_SUI05";

const char D_006870C8[16] = "E2_SUI04";

const char D_006870D8[16] = "E2_SUI03";

const char D_006870E8[16] = "E2_SUI02";

const char D_006870F8[16] = "E2_SUI01";

const char D_00687108[16] = "E2_KUK19";

const char D_00687118[16] = "E2_KUK18";

const char D_00687128[16] = "E2_KUK17";

const char D_00687138[16] = "E2_KUK11";

const char D_00687148[16] = "E2_KUK10";

const char D_00687158[16] = "E2_KUK09";

const char D_00687168[16] = "E2_KUK08";

const char D_00687178[16] = "E2_KUK07";

const char D_00687188[16] = "E2_KUK06";

const char D_00687198[16] = "E2_KUK05";

const char D_006871A8[16] = "E2_KUK04";

const char D_006871B8[16] = "E2_KUK03";

const char D_006871C8[16] = "E2_KUK02";

const char D_006871D8[16] = "E2_KUK01";

const char D_006871E8[16] = "E2_DYU05";

const char D_006871F8[16] = "E2_DYU04";

const char D_00687208[16] = "E2_DYU03";

const char D_00687218[16] = "E2_DYU02";

const char D_00687228[16] = "E2_DYU01";

const char D_00687238[16] = "E2_OLM18";

const char D_00687248[16] = "E2_OLM17";

const char D_00687258[16] = "E2_OLM16";

const char D_00687268[16] = "E2_OLM15";

const char D_00687278[16] = "E2_OLM14";

const char D_00687288[16] = "E2_OLM13";

const char D_00687298[16] = "E2_OLM12";

const char D_006872A8[16] = "E2_OLM11";

const char D_006872B8[16] = "E2_OLM10";

const char D_006872C8[16] = "E2_OLM09";

const char D_006872D8[16] = "E2_OLM08";

const char D_006872E8[16] = "E2_OLM07";

const char D_006872F8[16] = "E2_OLM06";

const char D_00687308[16] = "E2_OLM05";

const char D_00687318[16] = "E2_OLM04";

const char D_00687328[16] = "E2_OLM03";

const char D_00687338[16] = "E2_OLM02";

const char D_00687348[16] = "E2_OLM01";

const char D_00687358[16] = "E2_SPA01";

const char D_00687368[16] = "E2_SYD20";

const char D_00687378[16] = "E2_SYD19";

const char D_00687388[16] = "E2_SYD18";

const char D_00687398[16] = "E2_SYD17";

const char D_006873A8[16] = "E2_SYD16";

const char D_006873B8[16] = "E2_SYD15";

const char D_006873C8[16] = "E2_SYD14";

const char D_006873D8[16] = "E2_SYD13";

const char D_006873E8[16] = "E2_SYD12";

const char D_006873F8[16] = "E2_SYD11";

const char D_00687408[16] = "E2_SYD10";

const char D_00687418[16] = "E2_SYD09";

const char D_00687428[16] = "E2_SYD08";

const char D_00687438[16] = "E2_SYD07";

const char D_00687448[16] = "E2_SYD06";

const char D_00687458[16] = "E2_SYD05";

const char D_00687468[16] = "E2_SYD04";

const char D_00687478[16] = "E2_SYD03";

const char D_00687488[16] = "E2_SYD02";

const char D_00687498[16] = "E2_SYD01";

const char D_006874A8[16] = "E2_ELS08";

const char D_006874B8[16] = "E2_ELS07";

const char D_006874C8[16] = "E2_ELS06";

const char D_006874D8[16] = "E2_ELS05";

const char D_006874E8[16] = "E2_ELS04";

const char D_006874F8[16] = "E2_ELS03";

const char D_00687508[16] = "E2_ELS02";

const char D_00687518[16] = "E2_ELS01";

const char D_00687528[16] = "E2_MOM28";

const char D_00687538[16] = "E2_MOM27";

const char D_00687548[16] = "E2_MOM26";

const char D_00687558[16] = "E2_MOM25";

const char D_00687568[16] = "E2_MOM24";

const char D_00687578[16] = "E2_MOM23";

const char D_00687588[16] = "E2_MOM22";

const char D_00687598[16] = "E2_MOM21";

const char D_006875A8[16] = "E2_MOM19";

const char D_006875B8[16] = "E2_MOM18";

const char D_006875C8[16] = "E2_MOM17";

const char D_006875D8[16] = "E2_MOM16";

const char D_006875E8[16] = "E2_MOM15";

const char D_006875F8[16] = "E2_MOM14";

const char D_00687608[16] = "E2_MOM13";

const char D_00687618[16] = "E2_MOM12";

const char D_00687628[16] = "E2_MOM11";

const char D_00687638[16] = "E2_MOM10";

const char D_00687648[16] = "E2_MOM09";

const char D_00687658[16] = "E2_MOM08";

const char D_00687668[16] = "E2_MOM07";

const char D_00687678[16] = "E2_MOM06";

const char D_00687688[16] = "E2_MOM05";

const char D_00687698[16] = "E2_MOM03";

const char D_006876A8[16] = "E2_MOM02";

const char D_006876B8[16] = "E2_MOM01";

const char D_006876C8[16] = "E2_UMN04";

const char D_006876D8[16] = "E2_UMN03";

const char D_006876E8[16] = "E2_UMN02";

const char D_006876F8[16] = "E2_UMN01";

const char D_00687708[16] = "E2_UZU08";

const char D_00687718[16] = "E2_UZU06";

const char D_00687728[16] = "E2_UZU01";

const char D_00687738[16] = "E2_MIL17";

const char D_00687748[16] = "E2_MIL16";

const char D_00687758[16] = "E2_MIL15";

const char D_00687768[16] = "E2_MIL14";

const char D_00687778[16] = "E2_MIL13";

const char D_00687788[16] = "E2_MIL12";

const char D_00687798[16] = "E2_MIL11";

const char D_006877A8[16] = "E2_MIL10";

const char D_006877B8[16] = "E2_MIL09";

const char D_006877C8[16] = "E2_MIL08";

const char D_006877D8[16] = "E2_MIL07";

const char D_006877E8[16] = "E2_MIL06";

const char D_006877F8[16] = "E2_MIL05";

const char D_00687808[16] = "E2_MIL04";

const char D_00687818[16] = "E2_MIL03";

const char D_00687828[16] = "E2_MIL02";

const char D_00687838[16] = "E2_MIL01";

const char D_00687848[16] = "E2_VEC03";

const char D_00687858[16] = "E2_VEC02";

const char D_00687868[16] = "E2_VEC01";

const char D_00687878[16] = "E2_MID09";

const char D_00687888[16] = "E2_MID08";

const char D_00687898[16] = "E2_MID07";

const char D_006878A8[16] = "E2_MID06";

const char D_006878B8[16] = "E2_MID05";

const char D_006878C8[16] = "E2_MID04";

const char D_006878D8[16] = "E2_MID03";

const char D_006878E8[16] = "E2_MID02";

const char D_006878F8[16] = "E2_MID01";

const char D_00687908[16] = "E2_KML10";

const char D_00687918[16] = "E2_KML09";

const char D_00687928[16] = "E2_KML08";

const char D_00687938[16] = "E2_KML07";

const char D_00687948[16] = "E2_KML06";

const char D_00687958[16] = "E2_KML05";

const char D_00687968[16] = "E2_KML04";

const char D_00687978[16] = "E2_KML03";

const char D_00687988[16] = "E2_KML02";

const char D_00687998[16] = "E2_KML01";

const char D_006879A8[16] = "dummy_0f";

const char D_006879B8[16] = "dummy_0e";

const char D_006879C8[16] = "dummy_0d";

const char D_006879D8[16] = "dummy_0c";

const char D_006879E8[16] = "dummy_0b";

const char D_006879F8[16] = "E2_TST06";

const char D_00687A08[16] = "E2_TST05";

const char D_00687A18[16] = "E2_TST04";

const char D_00687A28[16] = "E2_TST03";

const char D_00687A38[16] = "E2_TST02";

const char D_00687A48[16] = "E2_TST01";

const char D_00687A58[16] = "camera_test";

const char D_00687A68[16] = "kowashi_robo";

const char D_00687A78[16] = "kowashi_chara";

const char D_00687A88[16] = "model\\map\\";

const char D_00687A98[16] = "face_all";

const char D_00687AA8[16] = "motion_ext50";

const char D_00687AB8[16] = "motion_ext49";

const char D_00687AC8[16] = "motion_ext48";

const char D_00687AD8[16] = "motion_ext47";

const char D_00687AE8[16] = "motion_ext46";

const char D_00687AF8[16] = "motion_ext45";

const char D_00687B08[16] = "motion_ext44";

const char D_00687B18[16] = "motion_ext43";

const char D_00687B28[16] = "motion_ext42";

const char D_00687B38[16] = "motion_ext41";

const char D_00687B48[16] = "motion_ext40";

const char D_00687B58[16] = "motion_ext39";

const char D_00687B68[16] = "motion_ext38";

const char D_00687B78[16] = "motion_ext37";

const char D_00687B88[16] = "motion_ext36";

const char D_00687B98[16] = "motion_ext35";

const char D_00687BA8[16] = "motion_ext34";

const char D_00687BB8[16] = "motion_ext33";

const char D_00687BC8[16] = "motion_ext32";

const char D_00687BD8[16] = "motion_ext31";

const char D_00687BE8[16] = "motion_ext30";

const char D_00687BF8[16] = "motion_ext29";

const char D_00687C08[16] = "motion_ext28";

const char D_00687C18[16] = "motion_ext27";

const char D_00687C28[16] = "motion_ext26";

const char D_00687C38[16] = "motion_ext25";

const char D_00687C48[16] = "motion_ext24";

const char D_00687C58[16] = "motion_ext23";

const char D_00687C68[16] = "motion_ext22";

const char D_00687C78[16] = "motion_ext21";

const char D_00687C88[16] = "motion_ext20";

const char D_00687C98[16] = "motion_ext19";

const char D_00687CA8[16] = "motion_ext18";

const char D_00687CB8[16] = "motion_ext17";

const char D_00687CC8[16] = "motion_ext16";

const char D_00687CD8[16] = "motion_ext15";

const char D_00687CE8[16] = "motion_ext14";

const char D_00687CF8[16] = "motion_ext13";

const char D_00687D08[16] = "motion_ext12";

const char D_00687D18[16] = "motion_ext11";

const char D_00687D28[16] = "motion_ext10";

const char D_00687D38[16] = "motion_ext09";

const char D_00687D48[16] = "motion_ext08";

const char D_00687D58[16] = "motion_ext07";

const char D_00687D68[16] = "motion_ext06";

const char D_00687D78[16] = "motion_ext05";

const char D_00687D88[16] = "motion_ext04";

const char D_00687D98[16] = "motion_ext03";

const char D_00687DA8[16] = "motion_ext02";

const char D_00687DB8[16] = "motion_ext01";

const char D_00687DC8[16] = "mobj153c";

const char D_00687DD8[16] = "mobj153b";

const char D_00687DE8[16] = "mobj153a";

const char D_00687DF8[16] = "mobj072b";

const char D_00687E08[16] = "mobj039b";

const char D_00687E18[16] = "mobj039a";

const char D_00687E28[16] = "mobj032b";

const char D_00687E38[16] = "mobj032a";

const char D_00687E48[16] = "mobj031f";

const char D_00687E58[16] = "mobj031e";

const char D_00687E68[16] = "mobj031d";

const char D_00687E78[16] = "mobj031c";

const char D_00687E88[16] = "mobj031b";

const char D_00687E98[16] = "mobj031a";

const char D_00687EA8[16] = "mobj030f";

const char D_00687EB8[16] = "mobj030e";

const char D_00687EC8[16] = "mobj030d";

const char D_00687ED8[16] = "mobj030c";

const char D_00687EE8[16] = "mobj030b";

const char D_00687EF8[16] = "mobj030a";

const char D_00687F08[16] = "mobj029f";

const char D_00687F18[16] = "mobj029e";

const char D_00687F28[16] = "mobj029d";

const char D_00687F38[16] = "mobj029c";

const char D_00687F48[16] = "mobj029b";

const char D_00687F58[16] = "mobj029a";

const char D_00687F68[16] = "mobj028c";

const char D_00687F78[16] = "mobj028b";

const char D_00687F88[16] = "mobj028a";

const char D_00687F98[16] = "mobj015b";

const char D_00687FA8[16] = "mobj015a";

const char D_00687FB8[16] = "model\\mobj\\";

const char D_00687FC8[16] = "enemy_020";

const char D_00687FD8[16] = "enemy_019";

const char D_00687FE8[16] = "enemy_018";

const char D_00687FF8[16] = "enemy_017";

const char D_00688008[16] = "enemy_016";

const char D_00688018[16] = "enemy_015";

const char D_00688028[16] = "enemy_014";

const char D_00688038[16] = "enemy_013";

const char D_00688048[16] = "enemy_012";

const char D_00688058[16] = "enemy_011";

const char D_00688068[16] = "enemy_010";

const char D_00688078[16] = "enemy_009";

const char D_00688088[16] = "enemy_008";

const char D_00688098[16] = "enemy_007";

const char D_006880A8[16] = "enemy_006";

const char D_006880B8[16] = "enemy_005";

const char D_006880C8[16] = "enemy_004";

const char D_006880D8[16] = "enemy_003";

const char D_006880E8[16] = "enemy_002";

const char D_006880F8[16] = "enemy_001";

const char D_00688108[16] = "enemy019";

const char D_00688118[16] = "enemy018";

const char D_00688128[16] = "enemy017";

const char D_00688138[16] = "enemy016";

const char D_00688148[16] = "enemy015";

const char D_00688158[16] = "enemy014";

const char D_00688168[16] = "enemy013";

const char D_00688178[16] = "enemy012";

const char D_00688188[16] = "enemy011";

const char D_00688198[16] = "enemy010";

const char D_006881A8[16] = "enemy009";

const char D_006881B8[16] = "enemy008";

const char D_006881C8[16] = "enemy007";

const char D_006881D8[16] = "enemy006";

const char D_006881E8[16] = "enemy005";

const char D_006881F8[16] = "enemy004";

const char D_00688208[16] = "enemy003";

const char D_00688218[16] = "enemy002";

const char D_00688228[16] = "enemy001";

/* 0x2B6420: 1024 zero bytes inside the shared gameWork object (tbl[418..674) of GameWork); this TU's view is the byte run only. */
unsigned char D_002B6420[1024] = { 0 };
