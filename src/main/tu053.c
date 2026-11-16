#include "common.h"

/* Returns 0. Same 8-byte body as func_001DF088 in main/tu052. No caller found in the binary. */
int func_001DF090(void) {
    return 0;
}

/*
 * Footstep material keys (proposed footMaterialKeys): 4-byte entries holding three letters, the fourth byte zero.
 * The list ends at an entry whose first byte is zero. Read by footMaterialFind and footMaterialKey.
 */
extern int footMaterialKeys[16];

/*
 * Returns the key entry at `index` (proposed footMaterialKey). enefoot and seFootBanksLoad copy its three bytes into
 * their sound command.
 */
void *footMaterialKey(int index) {
    return &footMaterialKeys[index];
}

/*
 * Searches the key list for the three bytes of `name` (proposed footMaterialFind) and returns the matching index.
 * It returns 0 when the list ends without a match, which is also the index of "kon", the default material.
 * foot calls it once per token.
 */
int footMaterialFind(const char *name) {
    const char *entry = (const char *)footMaterialKeys;
    int index = 0;

    while (entry[0] != 0) {
        if (entry[0] == name[0] && entry[1] == name[1] && entry[2] == name[2]) {
            return index;
        }
        entry += 4;
        index++;
    }
    return 0;
}

/* Footstep material keys: 16 entries of 4 bytes (three letters and a zero), terminated by an empty entry. */
int footMaterialKeys[16] = {
    0x006E6F6B, 0x00696172, 0x00616C67, 0x006F6C66,
    0x006F7269, 0x006F7473, 0x00726163, 0x006F6F77,
    0x006F6C63, 0x00746177, 0x00727574, 0x00726165,
    0x006F6E73, 0x00616E73, 0x00000000, 0x00000000,
};

/* Words read by res_get_path (the size argument of seBankLoad per kind; the last one is zero). Guessed type. */
int D_002BB5B8[22] = {
    0x00019000, 0x00005000, 0x0007D000, 0x000BE000, 0x000BE000, 0x00000000,
    0x00000000, 0x00154000, 0x00176000, 0x0017A400, 0x0017E800, 0x00182C00,
    0x00187000, 0x0018D000, 0x00193000, 0x00199000, 0x0019F000, 0x001A5000,
    0x001AB000, 0x001B1000, 0x001B7000, 0x00000000,
};

extern const char D_0069CFC0[];
extern const char D_0069CFB8[];
extern const char D_0069CFB0[];
extern const char D_0069CFA8[];
extern const char D_0069CFA0[];
extern const char D_0069CF98[];
extern const char D_0069CF90[];
extern const char D_0069CF88[];
extern const char D_0069CF80[];
extern const char D_0069CF78[];
extern const char D_0069CF58[];
extern const char D_0069CF50[];
extern const char D_0069CF48[];
extern const char D_0069CF40[];
extern const char D_0069CF38[];
extern const char D_0069CF30[];
extern const char D_0069CF28[];
extern const char D_0069CF20[];
extern const char D_0069CF18[];
extern const char D_0069CF10[];
extern const char D_0069CF08[];
extern const char D_0069CF00[];
extern const char D_0069CEF8[];

/* Command table of resScriptRun: the name stored in the scaffold strings of main/tu054 (its ';' prefix is not
 * part of the name) and the handler. The handler prototypes are guessed (void). */
typedef struct ResCommand {
    const char *name;
    void (*handler)(void);
} ResCommand;

void resCmdMap(void);
void resCmdAct(void);
void resCmdMot(void);
void resCmdMobj(void);
void resCmdEffect(void);
void resCmdMapse(void);
void resCmdRuntimeBgm(void);
void resCmdBgmVol(void);
void resCmdFoot(void);
void resCmdEnefoot(void);
void resCmdDisk(void);
void resCmdSave(void);
void resCmdMpeg2(void);

ResCommand resCommandTable[14] = {
    { D_0069CF58, resCmdMap },
    { D_0069CF50, resCmdAct },
    { D_0069CF48, resCmdMot },
    { D_0069CF40, resCmdMobj },
    { D_0069CF38, resCmdEffect },
    { D_0069CF30, resCmdMapse },
    { D_0069CF28, resCmdRuntimeBgm },
    { D_0069CF20, resCmdBgmVol },
    { D_0069CF18, resCmdFoot },
    { D_0069CF10, resCmdEnefoot },
    { D_0069CF08, resCmdDisk },
    { D_0069CF00, resCmdSave },
    { D_0069CEF8, resCmdMpeg2 },
    { 0, 0 }
};

const char *D_002BB680[10] = {
    D_0069CFC0, D_0069CFB8, D_0069CFB0, D_0069CFA8, D_0069CFA0,
    D_0069CF98, D_0069CF90, D_0069CF88, D_0069CF80, D_0069CF78
};

/* Guessed type: a table of 4-byte words (pairs of 16-bit values, 0xFFFFFFFF separators); its reader is in other TUs. */
unsigned int D_002BB6A8[40] = {
    0x10002u, 0x10005u, 0x10006u, 0x10004u, 0xFFFFFFFFu, 0x1000Au, 0x1000Bu, 0xFFFFFFFFu,
    0x10010u, 0x10011u, 0x10012u, 0x10013u, 0xFFFFFFFFu, 0x10016u, 0x10017u, 0x10018u,
    0x10019u, 0xFFFFFFFFu, 0x1001Cu, 0x1001Du, 0x1001Eu, 0xFFFFFFFFu, 0x10023u, 0x10024u,
    0xFFFFFFFFu, 0x10026u, 0x10027u, 0x10028u, 0xFFFFFFFFu, 0x14002u, 0x14008u, 0x14009u,
    0x1400Au, 0xFFFFFFFFu, 0x14003u, 0xFFFFFFFFu, 0x14005u, 0x1400Bu, 0xFFFFFFFFu, 0xFFFFFFFFu
};

char D_002BB748[16] = "fs_xxx_xxx";
char D_002BB758[16] = "break_xxx";

/* Guessed type: byte table read by resFootMaterialIndex (other TU). */
unsigned char D_002BB768[56] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 10, 10, 10, 10, 10,
    0, 0, 0, 0, 0, 11, 11, 11,
    0, 0, 0, 0, 0, 0, 0, 0,
    12, 0, 0, 0, 0, 0, 0, 0,
    0, 13, 0, 0, 0, 0, 0, 0
};

int xglCdSlotTable[] = { 0 };
int D_002BB7A4[] = { 0 };
unsigned char resSelectedSlot[2552] = { 0 };
