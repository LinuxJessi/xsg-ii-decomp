#include "common.h"
#include "main/pad.h"

INCLUDE_ASM("asm/main/nonmatchings/tu060", DebugTextPrevLine);
INCLUDE_ASM("asm/main/nonmatchings/tu060", DebugTextNextLine);
INCLUDE_ASM("asm/main/nonmatchings/tu060", DebugTextViewInit);
INCLUDE_ASM("asm/main/nonmatchings/tu060", DebugTextViewUpdate);
INCLUDE_ASM("asm/main/nonmatchings/tu060", func_001E9B28);
INCLUDE_ASM("asm/main/nonmatchings/tu060", FontTestP1);
INCLUDE_ASM("asm/main/nonmatchings/tu060", FontTest);
INCLUDE_ASM("asm/main/nonmatchings/tu060", SoundTest);

/* Empty callback: nothing to do. */
void func_001EAB08(void) {

}

/* Port names: memory card slots 1 and 2, then the HDD. */
extern const char *D_002BD1E8[];

const char *func_001EAB10(int port) {
    return D_002BD1E8[port];
}

int printf(const char *, ...);
extern const char D_00688900[];

/* Logs a callback id; always returns -1. */
int DebugCallbackLog(int callback_id) {
    printf(D_00688900, callback_id);
    return -1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu060", func_001EAB50);

INCLUDE_ASM("asm/main/nonmatchings/tu060", func_001EAC18);

INCLUDE_ASM("asm/main/nonmatchings/tu060", McFileSectionsReport);

int func_001EAB50(int);

/* The pad record of port 1 (PadData): held, pressed and auto-repeated
 * buttons as halfwords at 0x28, 0x2A and 0x2C; the exit test reads the
 * held and pressed words together as one doubleword. */
typedef union McTestPad {
    unsigned long long doublewords[8];
    unsigned short halfwords[32];
} McTestPad;

extern McTestPad PadData;

#define PAD_HELD_AND_PRESSED PadData.doublewords[0x28 / 8]
#define PAD_HELD PadData.halfwords[0x28 / 2]
#define PAD_PRESSED PadData.halfwords[0x2A / 2]
#define PAD_REPEATED PadData.halfwords[0x2C / 2]

/* SELECT held (0x100) with START pressed (0x800 of the next halfword). */
#define PAD_EXIT_COMBO 0x08000100

/* Free EE RAM the tests use as scratch. */
#define DEBUG_BUFFER_BASE 0x01000000
#define MC_TEST_FILE_BASE 0x01002000
#define MC_TEST_FILE_STRIDE 0x50000
#define MC_TEST_FILE_COUNT 11
#define MC_TEST_PATTERN_BASE 0x01100000

/* Requests of the xglMc queue as MemcardTest issues them. */
#define MC_REQ_FREE_SPACE 1
#define MC_REQ_EXIST 2
#define MC_REQ_LIST 3
#define MC_REQ_LOAD 4
#define MC_REQ_SAVE 5
#define MC_REQ_CREATE 6
#define MC_REQ_FORMAT 7
#define MC_REQ_CHECK_OLD 8
#define MC_REQ_UNFORMAT 9

/* Status halfwords returned by xglMcMain for each port. */
#define MC_PORT_READY 1
#define MC_PORT_BUSY 0xFF
#define MC_PORT_HDD 2
#define MC_PORT_COUNT 3
#define MC_SAVE_FILE_COUNT 100
#define MC_MENU_ITEM_COUNT 7
#define MC_HDD_MENU_ITEM_COUNT 2
#define MC_CREATE_PROGRESS_STEPS 20
#define MC_DUMMY_LIMIT_KB 1000
#define MC_EASY_SAVE_DUMMY_SIZE 0x77D800
#define SCE_MC_ICON_SYS_SIZE 964
#define SAVE_FLAGS_COUNT 128

/* MemcardTest state machine: the high nibble selects the screen family. */
enum MemcardTestState {
    MCTEST_STATE_INIT = 0x00,
    MCTEST_STATE_SELECT_PORT = 0x01,
    MCTEST_STATE_MENU_INIT = 0x10,
    MCTEST_STATE_MENU = 0x11,
    MCTEST_STATE_LOAD_LIST = 0x20,
    MCTEST_STATE_LOAD_CHECKING = 0x21,
    MCTEST_STATE_LOAD_NO_FILE = 0x22,
    MCTEST_STATE_LOAD_SELECT = 0x23,
    MCTEST_STATE_LOAD_READING = 0x24,
    MCTEST_STATE_SAVE_QUERY_FREE = 0x30,
    MCTEST_STATE_SAVE_CHECKING_FREE = 0x31,
    MCTEST_STATE_SAVE_SELECT_FILE = 0x32,
    MCTEST_STATE_SAVE_CREATE_CONFIRM = 0x33,
    MCTEST_STATE_SAVE_CREATING = 0x34,
    MCTEST_STATE_SAVE_CHECKING_FILE = 0x35,
    MCTEST_STATE_SAVE_OVERWRITE_CONFIRM = 0x36,
    MCTEST_STATE_SAVE_START = 0x37,
    MCTEST_STATE_SAVE_WRITING = 0x38,
    MCTEST_STATE_FORMAT_INIT = 0x40,
    MCTEST_STATE_FORMAT_CONFIRM = 0x41,
    MCTEST_STATE_FORMATTING = 0x42,
    MCTEST_STATE_FORMAT_DONE = 0x43,
    MCTEST_STATE_UNFORMAT_INIT = 0x50,
    MCTEST_STATE_UNFORMAT_CONFIRM = 0x51,
    MCTEST_STATE_UNFORMATTING = 0x52,
    MCTEST_STATE_UNFORMAT_DONE = 0x53,
    MCTEST_STATE_HDD_MENU_INIT = 0x60,
    MCTEST_STATE_HDD_MENU = 0x61,
    MCTEST_STATE_HDD_LOAD_LIST = 0x70,
    MCTEST_STATE_HDD_LOAD_NO_FILE = 0x71,
    MCTEST_STATE_HDD_LOAD_SELECT = 0x72,
    MCTEST_STATE_HDD_SAVE_INIT = 0x80,
    MCTEST_STATE_HDD_SAVE_SELECT_FILE = 0x81,
    MCTEST_STATE_HDD_SAVE_CREATE_CONFIRM = 0x82,
    MCTEST_STATE_HDD_SAVE_OVERWRITE_CONFIRM = 0x83,
    MCTEST_STATE_HDD_SAVE_WRITE = 0x84,
    MCTEST_STATE_LIST_START = 0x90,
    MCTEST_STATE_LISTING = 0x91,
    MCTEST_STATE_LIST_DONE = 0x92,
    MCTEST_STATE_CHECK_OLD_START = 0xA0,
    MCTEST_STATE_CHECKING_OLD = 0xA1,
    MCTEST_STATE_CHECK_OLD_DONE = 0xA2,
    MCTEST_STATE_DUMMY_QUERY_FREE = 0xB0,
    MCTEST_STATE_DUMMY_CHECKING_FREE = 0xB1,
    MCTEST_STATE_DUMMY_SELECT_SIZE = 0xB2
};

/* JP: メモカテスト (EN: memory card test) */
#define MC_TEXT_TITLE "\x0B\xA5\xE1\xA5\xE2\xA5\xAB\xA5\xC6\xA5\xB9\xA5\xC8"
/* JP: ポート選択 (EN: select a port) */
#define MC_TEXT_SELECT_PORT "\xA5\xDD\xA1\xBC\xA5\xC8\xC1\xAA\xC2\xF2"
/* JP: ロード (EN: load) */
#define MC_TEXT_LOAD "\xA5\xED\xA1\xBC\xA5\xC9"
/* JP: セーブ (EN: save) */
#define MC_TEXT_SAVE "\xA5\xBB\xA1\xBC\xA5\xD6"
/* JP: フォーマット (EN: format) */
#define MC_TEXT_FORMAT "\xA5\xD5\xA5\xA9\xA1\xBC\xA5\xDE\xA5\xC3\xA5\xC8"
/* JP: アンフォーマット (EN: unformat) */
#define MC_TEXT_UNFORMAT "\xA5\xA2\xA5\xF3\xA5\xD5\xA5\xA9\xA1\xBC\xA5\xDE\xA5\xC3\xA5\xC8"
/* JP: 列挙 (EN: list) */
#define MC_TEXT_LIST "\xCE\xF3\xB5\xF3"
/* JP: 旧データチェック (EN: old data check) */
#define MC_TEXT_CHECK_OLD "\xB5\xEC\xA5\xC7\xA1\xBC\xA5\xBF\xA5\xC1\xA5\xA7\xA5\xC3\xA5\xAF"
/* JP: ダミーデータ作成 (EN: create dummy data) */
#define MC_TEXT_DUMMY "\xA5\xC0\xA5\xDF\xA1\xBC\xA5\xC7\xA1\xBC\xA5\xBF\xBA\xEE\xC0\xAE"
/* JP: ロード　ファイル確認中 (EN: load: checking files) */
#define MC_TEXT_LOAD_CHECKING \
    "\xA5\xED\xA1\xBC\xA5\xC9\xA1\xA1\xA5\xD5\xA5\xA1\xA5\xA4\xA5\xEB\xB3\xCE\xC7\xA7\xC3\xE6"
/* JP: ロード　ファイルがありません / Ｏ：戻る (EN: load: no file / O: back) */
#define MC_TEXT_LOAD_NO_FILE \
    "\xA5\xED\xA1\xBC\xA5\xC9\xA1\xA1\xA5\xD5\xA5\xA1\xA5\xA4\xA5\xEB\xA4\xAC\xA4\xA2\xA4\xEA" \
    "\xA4\xDE\xA4\xBB\xA4\xF3\n\n\xA3\xCF\xA1\xA7\xCC\xE1\xA4\xEB"
/* JP: ファイル番号 %2d/99 (EN: file number %2d/99) */
#define MC_TEXT_FILE_OF_99 "\xA5\xD5\xA5\xA1\xA5\xA4\xA5\xEB\xC8\xD6\xB9\xE6 %2d/99"
/* JP: ファイルなし (EN: no file) */
#define MC_TEXT_FILE_ABSENT "\xA5\xD5\xA5\xA1\xA5\xA4\xA5\xEB\xA4\xCA\xA4\xB7"
/* JP: ファイルあり (EN: file present) */
#define MC_TEXT_FILE_PRESENT "\xA5\xD5\xA5\xA1\xA5\xA4\xA5\xEB\xA4\xA2\xA4\xEA"
/* JP: ロード　ファイル%d読み込み中 (EN: load: reading file %d) */
#define MC_TEXT_LOAD_READING \
    "\xA5\xED\xA1\xBC\xA5\xC9\xA1\xA1\xA5\xD5\xA5\xA1\xA5\xA4\xA5\xEB%d\xC6\xC9\xA4\xDF\xB9\xFE" \
    "\xA4\xDF\xC3\xE6"
/* JP: セーブ　空き容量：確認中 (EN: save: checking free space) */
#define MC_TEXT_SAVE_CHECKING_FREE \
    "\xA5\xBB\xA1\xBC\xA5\xD6\xA1\xA1\xB6\xF5\xA4\xAD\xCD\xC6\xCE\xCC\xA1\xA7\xB3\xCE\xC7\xA7\xC3\xE6"
/* JP: セーブ　空き容量：%5dKbyte (EN: save: free space %5d KB) */
#define MC_TEXT_SAVE_FREE \
    "\xA5\xBB\xA1\xBC\xA5\xD6\xA1\xA1\xB6\xF5\xA4\xAD\xCD\xC6\xCE\xCC\xA1\xA7%5dKbyte"
/* JP: ファイル番号 %2d (EN: file number %2d) */
#define MC_TEXT_FILE_NUMBER "\xA5\xD5\xA5\xA1\xA5\xA4\xA5\xEB\xC8\xD6\xB9\xE6 %2d"
/* JP: 新規作成します / Ｏ：はい　　Ｘ：いいえ (EN: create new / O: yes  X: no) */
#define MC_TEXT_CREATE_CONFIRM \
    "\xBF\xB7\xB5\xAC\xBA\xEE\xC0\xAE\xA4\xB7\xA4\xDE\xA4\xB9\n\n\xA3\xCF\xA1\xA7\xA4\xCF\xA4\xA4" \
    "\xA1\xA1\xA1\xA1\xA3\xD8\xA1\xA7\xA4\xA4\xA4\xA4\xA4\xA8"
/* JP: エンセフェロン・シミュレーター内 (EN: inside the Encephalon simulator) */
#define MC_ENTRY_PLACE \
    "\xA5\xA8\xA5\xF3\xA5\xBB\xA5\xD5\xA5\xA7\xA5\xED\xA5\xF3\xA1\xA6\xA5\xB7\xA5\xDF\xA5\xE5" \
    "\xA5\xEC\xA1\xBC\xA5\xBF\xA1\xBC\xC6\xE2"
/* JP: セーブ　新規作成中 (EN: save: creating) */
#define MC_TEXT_SAVE_CREATING \
    "\xA5\xBB\xA1\xBC\xA5\xD6\xA1\xA1\xBF\xB7\xB5\xAC\xBA\xEE\xC0\xAE\xC3\xE6"
/* JP: セーブ　ファイル%2d確認中 (EN: save: checking file %2d) */
#define MC_TEXT_SAVE_CHECKING_FILE \
    "\xA5\xBB\xA1\xBC\xA5\xD6\xA1\xA1\xA5\xD5\xA5\xA1\xA5\xA4\xA5\xEB%2d\xB3\xCE\xC7\xA7\xC3\xE6"
/* JP: セーブ　ファイル%2d (EN: save: file %2d) */
#define MC_TEXT_SAVE_FILE "\xA5\xBB\xA1\xBC\xA5\xD6\xA1\xA1\xA5\xD5\xA5\xA1\xA5\xA4\xA5\xEB%2d"
/* JP: 上書きしてよろしいですか？ / Ｏ：はい　　Ｘ：いいえ (EN: overwrite? / O: yes  X: no) */
#define MC_TEXT_OVERWRITE_CONFIRM \
    "\xBE\xE5\xBD\xF1\xA4\xAD\xA4\xB7\xA4\xC6\xA4\xE8\xA4\xED\xA4\xB7\xA4\xA4\xA4\xC7\xA4\xB9\xA4\xAB" \
    "\xA1\xA9\n\n\xA3\xCF\xA1\xA7\xA4\xCF\xA4\xA4\xA1\xA1\xA1\xA1\xA3\xD8\xA1\xA7\xA4\xA4\xA4\xA4\xA4\xA8"
/* JP: セーブ　ファイル%2d書き込み中 (EN: save: writing file %2d) */
#define MC_TEXT_SAVE_WRITING \
    "\xA5\xBB\xA1\xBC\xA5\xD6\xA1\xA1\xA5\xD5\xA5\xA1\xA5\xA4\xA5\xEB%2d\xBD\xF1\xA4\xAD\xB9\xFE" \
    "\xA4\xDF\xC3\xE6"
/* JP: フォーマットしてよろしいですか？ / Ｏ：はい　　×：いいえ (EN: format? / O: yes  X: no) */
#define MC_TEXT_FORMAT_CONFIRM \
    "\xA5\xD5\xA5\xA9\xA1\xBC\xA5\xDE\xA5\xC3\xA5\xC8\xA4\xB7\xA4\xC6\xA4\xE8\xA4\xED\xA4\xB7\xA4\xA4" \
    "\xA4\xC7\xA4\xB9\xA4\xAB\xA1\xA9\n\n\xA3\xCF\xA1\xA7\xA4\xCF\xA4\xA4\xA1\xA1\xA1\xA1\xA1\xDF\xA1\xA7" \
    "\xA4\xA4\xA4\xA4\xA4\xA8"
/* JP: フォーマット中・・・ (EN: formatting...) */
#define MC_TEXT_FORMATTING \
    "\xA5\xD5\xA5\xA9\xA1\xBC\xA5\xDE\xA5\xC3\xA5\xC8\xC3\xE6\xA1\xA6\xA1\xA6\xA1\xA6"
/* JP: フォーマットが終了しました / Ｏ：戻る (EN: format finished / O: back) */
#define MC_TEXT_FORMAT_DONE \
    "\xA5\xD5\xA5\xA9\xA1\xBC\xA5\xDE\xA5\xC3\xA5\xC8\xA4\xAC\xBD\xAA\xCE\xBB\xA4\xB7\xA4\xDE\xA4\xB7" \
    "\xA4\xBF\n\n\xA3\xCF\xA1\xA7\xCC\xE1\xA4\xEB"
/* JP: アンフォーマットしてよろしいですか？ / Ｏ：はい　　×：いいえ (EN: unformat? / O: yes  X: no) */
#define MC_TEXT_UNFORMAT_CONFIRM \
    "\xA5\xA2\xA5\xF3\xA5\xD5\xA5\xA9\xA1\xBC\xA5\xDE\xA5\xC3\xA5\xC8\xA4\xB7\xA4\xC6\xA4\xE8\xA4\xED" \
    "\xA4\xB7\xA4\xA4\xA4\xC7\xA4\xB9\xA4\xAB\xA1\xA9\n\n\xA3\xCF\xA1\xA7\xA4\xCF\xA4\xA4\xA1\xA1\xA1\xA1" \
    "\xA1\xDF\xA1\xA7\xA4\xA4\xA4\xA4\xA4\xA8"
/* JP: アンフォーマット中・・・ (EN: unformatting...) */
#define MC_TEXT_UNFORMATTING \
    "\xA5\xA2\xA5\xF3\xA5\xD5\xA5\xA9\xA1\xBC\xA5\xDE\xA5\xC3\xA5\xC8\xC3\xE6\xA1\xA6\xA1\xA6\xA1\xA6"
/* JP: アンフォーマットが終了しました / Ｏ：戻る (EN: unformat finished / O: back) */
#define MC_TEXT_UNFORMAT_DONE \
    "\xA5\xA2\xA5\xF3\xA5\xD5\xA5\xA9\xA1\xBC\xA5\xDE\xA5\xC3\xA5\xC8\xA4\xAC\xBD\xAA\xCE\xBB\xA4\xB7" \
    "\xA4\xDE\xA4\xB7\xA4\xBF\n\n\xA3\xCF\xA1\xA7\xCC\xE1\xA4\xEB"
/* JP: 列挙中・・・ (EN: listing...) */
#define MC_TEXT_LISTING "\xCE\xF3\xB5\xF3\xC3\xE6\xA1\xA6\xA1\xA6\xA1\xA6"
/* JP: 列挙終了 / ○：戻る (EN: listing finished / O: back) */
#define MC_TEXT_LIST_DONE "\xCE\xF3\xB5\xF3\xBD\xAA\xCE\xBB\n\n\xA1\xFB\xA1\xA7\xCC\xE1\xA4\xEB"
/* JP: ダミーデータ作成　空き容量：確認中 (EN: dummy data: checking free space) */
#define MC_TEXT_DUMMY_CHECKING_FREE \
    "\xA5\xC0\xA5\xDF\xA1\xBC\xA5\xC7\xA1\xBC\xA5\xBF\xBA\xEE\xC0\xAE\xA1\xA1\xB6\xF5\xA4\xAD\xCD\xC6" \
    "\xCE\xCC\xA1\xA7\xB3\xCE\xC7\xA7\xC3\xE6"
/* JP: ダミーデータ作成　空き容量：%5dKB (EN: dummy data: free space %5d KB) */
#define MC_TEXT_DUMMY_FREE \
    "\xA5\xC0\xA5\xDF\xA1\xBC\xA5\xC7\xA1\xBC\xA5\xBF\xBA\xEE\xC0\xAE\xA1\xA1\xB6\xF5\xA4\xAD\xCD\xC6" \
    "\xCE\xCC\xA1\xA7%5dKB"
/* JP: 残す容量 %3dKB (EN: space to keep %3d KB) */
#define MC_TEXT_DUMMY_KEEP "\xBB\xC4\xA4\xB9\xCD\xC6\xCE\xCC %3dKB"
/* JP: 左右：増減 / □：１００ＫＢ単位 / ○：作成（時間掛かります）
 * EN: left/right: change / square: 100 KB steps / O: create (takes a while) */
#define MC_TEXT_DUMMY_HELP \
    "\xBA\xB8\xB1\xA6\xA1\xA7\xC1\xFD\xB8\xBA\n\n\xA2\xA2\xA1\xA7\xA3\xB1\xA3\xB0\xA3\xB0\xA3\xCB\xA3\xC2" \
    "\xC3\xB1\xB0\xCC\n\n\xA1\xFB\xA1\xA7\xBA\xEE\xC0\xAE\xA1\xCA\xBB\xFE\xB4\xD6\xB3\xDD\xA4\xAB\xA4\xEA" \
    "\xA4\xDE\xA4\xB9\xA1\xCB"
/* JP: ＨＤＤロード　ファイルがありません / Ｏ：戻る (EN: HDD load: no file / O: back) */
#define MC_TEXT_HDD_LOAD_NO_FILE \
    "\xA3\xC8\xA3\xC4\xA3\xC4\xA5\xED\xA1\xBC\xA5\xC9\xA1\xA1\xA5\xD5\xA5\xA1\xA5\xA4\xA5\xEB\xA4\xAC" \
    "\xA4\xA2\xA4\xEA\xA4\xDE\xA4\xBB\xA4\xF3\n\n\xA3\xCF\xA1\xA7\xCC\xE1\xA4\xEB"
/* JP: ＨＤＤロード (EN: HDD load) */
#define MC_TEXT_HDD_LOAD "\xA3\xC8\xA3\xC4\xA3\xC4\xA5\xED\xA1\xBC\xA5\xC9"
/* JP: ＨＤＤセーブ　空き容量：%5dKbyte (EN: HDD save: free space %5d KB) */
#define MC_TEXT_HDD_SAVE_FREE \
    "\xA3\xC8\xA3\xC4\xA3\xC4\xA5\xBB\xA1\xBC\xA5\xD6\xA1\xA1\xB6\xF5\xA4\xAD\xCD\xC6\xCE\xCC\xA1\xA7" \
    "%5dKbyte"

/* One request of the xglMc queue: the file number, its buffer and size, and
 * a callback the list and old-data scans call per entry. */
typedef struct McRequestParam {
    int file;
    void *data;
    int size;
    int (*callback)(int);
} McRequestParam;

typedef struct MemcardTestWork {
    int result;
    int check;
    int file;
} MemcardTestWork;

/* The header of a card data image: the begin and end of the payload. */
struct HddTransferInfo {
    int unmodeled_00;
    int begin;
    int end;
};

struct HddCheckState {
    int status;
    void *transfer;
    int dataSize;
};

struct HddExistRequest {
    int card;
    unsigned char *found;
};

struct HddLoadRequest {
    int card;
    void *data;
    int size;
};

struct HddSaveRequest {
    int card;
    const void *data;
    int size;
};

/* The thumbnail image MemcardTest copies into the save. */
typedef struct McThumbnail {
    unsigned char bytes[0x1000];
} McThumbnail;

/* The save image is the memory from SaveData on: card file 0 holds its
 * 0x50-byte header, the other files the rest from D_0063F890. */
#define SAVE_DATA_SIZE 0x5160
#define SAVE_HEADER_SIZE 0x50
extern unsigned char SaveData[];
extern unsigned char D_0063F890[];
/* The event flags the triangle button randomizes. */
extern unsigned char D_0063F8B4[];
/* The thumbnail saved with the game. */
extern McThumbnail D_0063F9B4;

/* Strings and a table MemcardTest shares with functions that are still
 * assembly: ">" is the menu cursor (X1's menu_cursor, first emitted by
 * SoundTest), "%2x" is shared with func_001EC8E8, and "%1x", "%2x %2x" and
 * the HDD menu table sit in the next scaffold piece of .sdata. */
extern char D_0069D028[];
extern const char D_0069D090[];
extern const char D_0069D0D0[];
extern const char D_0069D0D8[];
/* HDD menu row -> state: MCTEST_STATE_HDD_LOAD_LIST, MCTEST_STATE_HDD_SAVE_INIT
 * (X1 names this static tbl_49). */
extern unsigned char tbl_49[4];

void xglFontDebugPrintf(int x, int y, const char *format, ...);
unsigned int xglMcMain(void);
int xglMcRequest(int port, int command, McRequestParam *param, int *result);
int xglMcEasySave(const char *name, void *data, int size);
int xglMcSetEntryNumbers(void *data, int file, const char *place);
int xglHddCheck(void);
int xglHddMcExist(struct HddExistRequest *request);
int xglHddMcLoad(void *save);
int xglHddMcSave(const struct HddSaveRequest *save);
int xglHddMcCreate_001A5B80(struct HddCheckState *state);
int xglCdReadFile(const char *name, void *buffer, int mode, int flags);
int sceMcSync(int mode, int *command, int *result);
unsigned short xglSRand(void);
void xglSleep(void);
void FlushCache(int operation);
void McFileSectionsReport(const char *name, void *image);

void MemcardTest(void) {
    void *files[MC_TEST_FILE_COUNT];
    unsigned short port_status[2];
    MemcardTestWork work;
    McRequestParam list_request;
    McRequestParam load_request;
    McRequestParam check_request;
    McRequestParam create_request;
    char progress[32];
    McRequestParam save_request;
    McRequestParam enumerate_request;
    McRequestParam check_old_request;
    struct HddExistRequest hdd_list_request;
    struct HddLoadRequest hdd_load_request;
    struct HddExistRequest hdd_check_request;
    struct HddCheckState hdd_create_request;
    struct HddSaveRequest hdd_save_request;
    int sync_command;
    int sync_result;
    int dummy_command;
    int dummy_result;
    unsigned char *found = (unsigned char *)DEBUG_BUFFER_BASE;
    const char *name;
    char *bar;
    unsigned char *flags;
    unsigned char *shown_flags;
    int port;
    int hdd;
    int state;
    int dir;
    int port_step;
    int menu_step;
    int save_file_step;
    int dummy_step;
    int hdd_menu_step;
    int hdd_file_step;
    int done;
    int file;
    int i;

    state = MCTEST_STATE_INIT;
    port = -1;
    hdd = -99;
    xglCdReadFile("yajima\\thumb.jpg", (void *)DEBUG_BUFFER_BASE, 0, 0);
    D_0063F9B4 = *(McThumbnail *)DEBUG_BUFFER_BASE;
    files[0] = (void *)MC_TEST_FILE_BASE;
    for (file = 1; file < MC_TEST_FILE_COUNT; file++) {
        files[file] = (char *)files[file - 1] + MC_TEST_FILE_STRIDE;
    }
    printf("carddata:%08x\n", files[0]);
    McFileSectionsReport("yajima\\mc\\clear.bin", files[0]);
    McFileSectionsReport("yajima\\mc\\chaos.bin", files[1]);
    McFileSectionsReport("yajima\\mc\\jin.bin", files[2]);
    McFileSectionsReport("yajima\\mc\\jr.bin", files[3]);
    McFileSectionsReport("yajima\\mc\\kosmos.bin", files[4]);
    McFileSectionsReport("yajima\\mc\\momo.bin", files[5]);
    McFileSectionsReport("yajima\\mc\\r02.bin", files[6]);
    McFileSectionsReport("yajima\\mc\\r03.bin", files[7]);
    McFileSectionsReport("yajima\\mc\\r05.bin", files[8]);
    McFileSectionsReport("yajima\\mc\\shion.bin", files[9]);
    McFileSectionsReport("yajima\\mc\\ziggy.bin", files[10]);
    printf("sizeof(sceMcIconSys):%d\n", SCE_MC_ICON_SYS_SIZE);
    printf("sizeof(sSaveData):%d\n", SAVE_DATA_SIZE);
    printf("sizeof(carddata):%d\n",
           ((struct HddTransferInfo *)files[0])->end - ((struct HddTransferInfo *)files[0])->begin);

    while ((PAD_HELD_AND_PRESSED & PAD_EXIT_COMBO) != PAD_EXIT_COMBO) {
        xglFontDebugPrintf(0, 0, MC_TEXT_TITLE);
        /* xglMcMain returns both ports' status halfwords as one word. */
        *(unsigned int *)port_status = xglMcMain();
        xglFontDebugPrintf(64, 0, "%4x %4x", port_status[0], port_status[1]);
        if (port >= 0 && port < MC_PORT_HDD) {
            if (port_status[port] != MC_PORT_READY && port_status[port] != MC_PORT_BUSY) {
                state = MCTEST_STATE_INIT;
            }
        }
        xglFontDebugPrintf(0, 16, D_0069D090, state);

        switch (state) {
        case MCTEST_STATE_INIT:
            state = MCTEST_STATE_SELECT_PORT;
            hdd = xglHddCheck();
            /* fall through */
        case MCTEST_STATE_SELECT_PORT:
            xglFontDebugPrintf(0, 32, MC_TEXT_SELECT_PORT);
            name = func_001EAB10(0);
            if (port_status[0] == MC_PORT_READY) {
                xglFontDebugPrintf(16, 48, "%s", name);
            } else {
                /* Text: {color #202020}%s */
                xglFontDebugPrintf(16, 48, "\x0C   %s", name);
            }
            name = func_001EAB10(1);
            if (port_status[1] == MC_PORT_READY) {
                xglFontDebugPrintf(16, 64, "%s", name);
            } else {
                xglFontDebugPrintf(16, 64, "\x0C   %s", name);
            }
            name = func_001EAB10(2);
            if (hdd == 0) {
                xglFontDebugPrintf(16, 80, "%s", name);
            } else {
                xglFontDebugPrintf(16, 80, "\x0C   %s", name);
            }
            if (port_status[0] == MC_PORT_READY || port_status[1] == MC_PORT_READY || hdd == 0) {
                port_step = (PAD_PRESSED & PAD_UP) ? -1 : 0;
                if (PAD_PRESSED & PAD_DOWN) {
                    port_step = 1;
                }
                if (port_step != 0) {
                    work.result += port_step;
                } else {
                    port_step = 1;
                }
                for (;;) {
                    work.result = (work.result + MC_PORT_COUNT) % MC_PORT_COUNT;
                    if ((port_status[0] != MC_PORT_READY && work.result == 0) ||
                        (port_status[1] != MC_PORT_READY && work.result == 1) ||
                        (hdd < 0 && work.result == MC_PORT_HDD)) {
                        work.result += port_step;
                    } else {
                        break;
                    }
                }
            } else {
                work.result = -1;
            }
            if (work.result != -1 && (PAD_PRESSED & PAD_CIRCLE)) {
                if (work.result == MC_PORT_HDD) {
                    port = MC_PORT_HDD;
                    state = MCTEST_STATE_HDD_MENU_INIT;
                } else {
                    port = work.result;
                    state = MCTEST_STATE_MENU_INIT;
                }
            }
            xglFontDebugPrintf(8, work.result * 16 + 48, D_0069D028);
            break;

        case MCTEST_STATE_MENU_INIT:
            work.result = 0;
            state = MCTEST_STATE_MENU;
            /* fall through */
        case MCTEST_STATE_MENU:
            xglFontDebugPrintf(0, 32, "%s", func_001EAB10(port));
            xglFontDebugPrintf(16, 48, MC_TEXT_LOAD);
            xglFontDebugPrintf(16, 64, MC_TEXT_SAVE);
            xglFontDebugPrintf(16, 80, MC_TEXT_FORMAT);
            xglFontDebugPrintf(16, 96, MC_TEXT_UNFORMAT);
            xglFontDebugPrintf(16, 112, MC_TEXT_LIST);
            xglFontDebugPrintf(16, 128, MC_TEXT_CHECK_OLD);
            xglFontDebugPrintf(16, 144, MC_TEXT_DUMMY);
            menu_step = (PAD_PRESSED & PAD_UP) ? -1 : 0;
            if (PAD_PRESSED & PAD_DOWN) {
                menu_step = 1;
            }
            if (menu_step != 0) {
                work.result = (work.result + menu_step + MC_MENU_ITEM_COUNT) % MC_MENU_ITEM_COUNT;
            }
            xglFontDebugPrintf(8, work.result * 16 + 48, D_0069D028);
            if (PAD_PRESSED & PAD_CIRCLE) {
                static unsigned char menu_states[8] = {
                    MCTEST_STATE_LOAD_LIST, MCTEST_STATE_SAVE_QUERY_FREE,
                    MCTEST_STATE_FORMAT_INIT, MCTEST_STATE_UNFORMAT_INIT,
                    MCTEST_STATE_LIST_START, MCTEST_STATE_CHECK_OLD_START,
                    MCTEST_STATE_DUMMY_QUERY_FREE
                };

                if (PAD_HELD & PAD_R1) {
                    state = MCTEST_STATE_INIT;
                    sceMcSync(0, &sync_command, &sync_result);
                    printf("%d %d\n", sync_command, sync_result);
                    xglMcEasySave("/BISLPS-99999Dummy", (void *)DEBUG_BUFFER_BASE, MC_EASY_SAVE_DUMMY_SIZE);
                } else {
                    state = menu_states[work.result];
                }
            }
            if (PAD_PRESSED & PAD_CROSS) {
                state = MCTEST_STATE_INIT;
            }
            break;

        case MCTEST_STATE_LOAD_LIST:
            list_request.file = -1;
            list_request.data = found;
            xglMcRequest(port, MC_REQ_EXIST, &list_request, &work.result);
            state = MCTEST_STATE_LOAD_CHECKING;
            break;

        case MCTEST_STATE_LOAD_CHECKING:
            xglFontDebugPrintf(0, 32, MC_TEXT_LOAD_CHECKING);
            if (port_status[port] != MC_PORT_BUSY) {
                state = MCTEST_STATE_LOAD_NO_FILE;
                if (work.result >= 0) {
                    work.file = 0;
                    state = MCTEST_STATE_LOAD_SELECT;
                }
            }
            break;

        case MCTEST_STATE_LOAD_NO_FILE:
            xglFontDebugPrintf(0, 32, MC_TEXT_LOAD_NO_FILE, work.file);
            if (PAD_PRESSED & PAD_CIRCLE) {
                state = MCTEST_STATE_MENU_INIT;
            }
            break;

        case MCTEST_STATE_LOAD_SELECT:
            xglFontDebugPrintf(0, 32, MC_TEXT_LOAD);
            dir = 0;
            if (PAD_REPEATED & PAD_LEFT) {
                dir = -1;
            }
            if (PAD_REPEATED & PAD_RIGHT) {
                dir = 1;
            }
            if (dir != 0) {
                work.file = (work.file + dir + MC_SAVE_FILE_COUNT) % MC_SAVE_FILE_COUNT;
            }
            xglFontDebugPrintf(16, 48, MC_TEXT_FILE_OF_99, work.file);
            if (found[work.file] == 0) {
                xglFontDebugPrintf(16, 64, MC_TEXT_FILE_ABSENT);
            } else {
                xglFontDebugPrintf(16, 64, MC_TEXT_FILE_PRESENT);
            }
            if ((PAD_PRESSED & PAD_CIRCLE) && found[work.file] != 0) {
                load_request.file = work.file;
                load_request.data = SaveData;
                load_request.size = SAVE_DATA_SIZE;
                state = MCTEST_STATE_LOAD_READING;
                xglMcRequest(port, MC_REQ_LOAD, &load_request, &work.result);
            }
            if (PAD_PRESSED & PAD_CROSS) {
                state = MCTEST_STATE_MENU_INIT;
            }
            break;

        case MCTEST_STATE_LOAD_READING:
            xglFontDebugPrintf(0, 32, MC_TEXT_LOAD_READING, work.file);
            if (port_status[port] != MC_PORT_BUSY) {
                state = MCTEST_STATE_MENU_INIT;
            }
            break;

        case MCTEST_STATE_SAVE_QUERY_FREE:
            xglMcRequest(port, MC_REQ_FREE_SPACE, 0, &work.result);
            state = MCTEST_STATE_SAVE_CHECKING_FREE;
            work.check = 0;
            break;

        case MCTEST_STATE_SAVE_CHECKING_FREE:
            xglFontDebugPrintf(0, 32, MC_TEXT_SAVE_CHECKING_FREE);
            if (port_status[port] != MC_PORT_BUSY) {
                work.file = 0;
                state = MCTEST_STATE_SAVE_SELECT_FILE;
            }
            break;

        case MCTEST_STATE_SAVE_SELECT_FILE:
            xglFontDebugPrintf(0, 32, MC_TEXT_SAVE_FREE, work.result);
            save_file_step = (PAD_REPEATED & PAD_LEFT) ? -1 : 0;
            if (PAD_REPEATED & PAD_RIGHT) {
                save_file_step = 1;
            }
            if (save_file_step != 0) {
                work.file = (work.file + save_file_step + MC_SAVE_FILE_COUNT) % MC_SAVE_FILE_COUNT;
            }
            xglFontDebugPrintf(16, 48, MC_TEXT_FILE_NUMBER, work.file);
            if (PAD_PRESSED & PAD_CIRCLE) {
                check_request.file = work.file;
                xglMcRequest(port, MC_REQ_EXIST, &check_request, &work.check);
                state = MCTEST_STATE_SAVE_CHECKING_FILE;
            }
            if (PAD_PRESSED & PAD_CROSS) {
                state = MCTEST_STATE_MENU_INIT;
            }
            break;

        case MCTEST_STATE_SAVE_CREATE_CONFIRM:
            xglFontDebugPrintf(0, 32, MC_TEXT_SAVE_FREE, work.result);
            xglFontDebugPrintf(16, 48, MC_TEXT_CREATE_CONFIRM);
            if (PAD_PRESSED & PAD_CIRCLE) {
                create_request.data = files;
                create_request.file = work.file;
                if (work.file < MC_TEST_FILE_COUNT) {
                    create_request.data = files[work.file];
                }
                state = MCTEST_STATE_SAVE_CREATING;
                xglMcSetEntryNumbers(create_request.data, create_request.file, MC_ENTRY_PLACE);
                xglMcRequest(port, MC_REQ_CREATE, &create_request, &work.result);
            }
            if (PAD_PRESSED & PAD_CROSS) {
                state = MCTEST_STATE_MENU_INIT;
            }
            break;

        case MCTEST_STATE_SAVE_CREATING:
            xglFontDebugPrintf(0, 32, MC_TEXT_SAVE_CREATING);
            bar = progress;
            for (done = 0; done < work.result; done++) {
                *bar++ = '*';
            }
            for (; done < MC_CREATE_PROGRESS_STEPS; done++) {
                *bar++ = '.';
            }
            *bar = '\0';
            xglFontDebugPrintf(16, 48, progress);
            if (port_status[port] != MC_PORT_BUSY) {
                state = MCTEST_STATE_SAVE_START;
            }
            break;

        case MCTEST_STATE_SAVE_CHECKING_FILE:
            xglFontDebugPrintf(0, 32, MC_TEXT_SAVE_CHECKING_FILE, work.file);
            if (port_status[port] != MC_PORT_BUSY) {
                state = MCTEST_STATE_SAVE_CREATE_CONFIRM;
            }
            break;

        case MCTEST_STATE_SAVE_OVERWRITE_CONFIRM:
            xglFontDebugPrintf(0, 32, MC_TEXT_SAVE_FILE, work.file);
            xglFontDebugPrintf(16, 48, MC_TEXT_OVERWRITE_CONFIRM);
            if (PAD_PRESSED & PAD_CIRCLE) {
                state = MCTEST_STATE_SAVE_START;
            }
            if (PAD_PRESSED & PAD_CROSS) {
                state = MCTEST_STATE_MENU_INIT;
            }
            break;

        case MCTEST_STATE_SAVE_START:
            save_request.file = work.file;
            if (work.file == 0) {
                save_request.data = SaveData;
                save_request.size = SAVE_HEADER_SIZE;
            } else {
                save_request.data = D_0063F890;
                save_request.size = SAVE_DATA_SIZE - SAVE_HEADER_SIZE;
            }
            xglMcRequest(port, MC_REQ_SAVE, &save_request, &work.result);
            state = MCTEST_STATE_SAVE_WRITING;
            break;

        case MCTEST_STATE_SAVE_WRITING:
            xglFontDebugPrintf(0, 32, MC_TEXT_SAVE_WRITING, work.file);
            if (port_status[port] != MC_PORT_BUSY) {
                state = MCTEST_STATE_MENU_INIT;
            }
            break;

        case MCTEST_STATE_FORMAT_INIT:
            state = MCTEST_STATE_FORMAT_CONFIRM;
            /* fall through */
        case MCTEST_STATE_FORMAT_CONFIRM:
            xglFontDebugPrintf(0, 32, MC_TEXT_FORMAT_CONFIRM);
            if (PAD_PRESSED & PAD_CIRCLE) {
                xglMcRequest(port, MC_REQ_FORMAT, 0, &work.result);
                state = MCTEST_STATE_FORMATTING;
            }
            if (PAD_PRESSED & PAD_CROSS) {
                state = MCTEST_STATE_MENU_INIT;
            }
            break;

        case MCTEST_STATE_FORMATTING:
            xglFontDebugPrintf(0, 32, MC_TEXT_FORMATTING);
            if (port_status[port] != MC_PORT_BUSY) {
                state = MCTEST_STATE_FORMAT_DONE;
            }
            break;

        case MCTEST_STATE_FORMAT_DONE:
            xglFontDebugPrintf(0, 32, MC_TEXT_FORMAT_DONE);
            if (PAD_PRESSED & PAD_CIRCLE) {
                state = MCTEST_STATE_MENU_INIT;
            }
            break;

        case MCTEST_STATE_UNFORMAT_INIT:
            state = MCTEST_STATE_UNFORMAT_CONFIRM;
            /* fall through */
        case MCTEST_STATE_UNFORMAT_CONFIRM:
            xglFontDebugPrintf(0, 32, MC_TEXT_UNFORMAT_CONFIRM);
            if (PAD_PRESSED & PAD_CIRCLE) {
                xglMcRequest(port, MC_REQ_UNFORMAT, 0, &work.result);
                state = MCTEST_STATE_UNFORMATTING;
            }
            if (PAD_PRESSED & PAD_CROSS) {
                state = MCTEST_STATE_MENU_INIT;
            }
            break;

        case MCTEST_STATE_UNFORMATTING:
            xglFontDebugPrintf(0, 32, MC_TEXT_UNFORMATTING);
            if (port_status[port] != MC_PORT_BUSY) {
                state = MCTEST_STATE_UNFORMAT_DONE;
            }
            break;

        case MCTEST_STATE_UNFORMAT_DONE:
            xglFontDebugPrintf(0, 32, MC_TEXT_UNFORMAT_DONE);
            if (PAD_PRESSED & PAD_CIRCLE) {
                state = MCTEST_STATE_MENU_INIT;
            }
            break;

        case MCTEST_STATE_LIST_START:
            enumerate_request.file = -1;
            enumerate_request.data = found;
            enumerate_request.size = SAVE_DATA_SIZE;
            enumerate_request.callback = DebugCallbackLog;
            state = MCTEST_STATE_LISTING;
            xglMcRequest(port, MC_REQ_LIST, &enumerate_request, &work.result);
            break;

        case MCTEST_STATE_LISTING:
        case MCTEST_STATE_LIST_DONE:
            for (i = 1; i < MC_SAVE_FILE_COUNT; i++) {
                xglFontDebugPrintf((i % 10) * 8 + 128, (i / 10) * 8 + 48, D_0069D0D0, found[i]);
            }
            xglFontDebugPrintf(128, 128, D_0069D0D8, found[100], found[101]);
            {
                unsigned char *pattern = (unsigned char *)MC_TEST_PATTERN_BASE;

                xglFontDebugPrintf(16, 160, "%2x %2x %2x %2x %2x %2x %2x %2x", pattern[0], pattern[1],
                                   pattern[2], pattern[3], pattern[4], pattern[5], pattern[6], pattern[7]);
            }
            if (state == MCTEST_STATE_LISTING) {
                if (found[101] == 2) {
                    state = MCTEST_STATE_LIST_DONE;
                } else {
                    xglFontDebugPrintf(0, 32, MC_TEXT_LISTING);
                }
            } else {
                xglFontDebugPrintf(0, 32, MC_TEXT_LIST_DONE);
                if (PAD_PRESSED & PAD_CIRCLE) {
                    state = MCTEST_STATE_MENU_INIT;
                }
            }
            break;

        case MCTEST_STATE_CHECK_OLD_START:
            check_old_request.file = 0;
            check_old_request.data = found;
            check_old_request.callback = func_001EAB50;
            state = MCTEST_STATE_CHECKING_OLD;
            xglMcRequest(port, MC_REQ_CHECK_OLD, &check_old_request, &work.result);
            break;

        case MCTEST_STATE_CHECKING_OLD:
        case MCTEST_STATE_CHECK_OLD_DONE:
            for (i = 1; i < MC_SAVE_FILE_COUNT; i++) {
                xglFontDebugPrintf((i % 10) * 8 + 128, (i / 10) * 8 + 48, D_0069D0D0, found[i]);
            }
            xglFontDebugPrintf(128, 128, D_0069D0D8, found[100], found[101]);
            {
                unsigned char *pattern = (unsigned char *)MC_TEST_PATTERN_BASE;

                xglFontDebugPrintf(16, 160, "%2x %2x %2x %2x %2x %2x %2x %2x", pattern[0], pattern[1],
                                   pattern[2], pattern[3], pattern[4], pattern[5], pattern[6], pattern[7]);
            }
            if (state == MCTEST_STATE_CHECKING_OLD) {
                if (found[101] != 2) {
                    xglFontDebugPrintf(0, 32, MC_TEXT_LISTING);
                } else {
                    state = MCTEST_STATE_CHECK_OLD_DONE;
                }
            } else {
                xglFontDebugPrintf(0, 32, MC_TEXT_LIST_DONE);
                if (PAD_PRESSED & PAD_CIRCLE) {
                    state = MCTEST_STATE_MENU_INIT;
                }
            }
            break;

        case MCTEST_STATE_DUMMY_QUERY_FREE:
            xglMcRequest(port, MC_REQ_FREE_SPACE, 0, &work.result);
            state = MCTEST_STATE_DUMMY_CHECKING_FREE;
            break;

        case MCTEST_STATE_DUMMY_CHECKING_FREE:
            xglFontDebugPrintf(0, 32, MC_TEXT_DUMMY_CHECKING_FREE);
            if (port_status[port] != MC_PORT_BUSY) {
                work.check = 0;
                state = MCTEST_STATE_DUMMY_SELECT_SIZE;
            }
            break;

        case MCTEST_STATE_DUMMY_SELECT_SIZE:
            xglFontDebugPrintf(0, 32, MC_TEXT_DUMMY_FREE, work.result);
            dummy_step = (PAD_REPEATED & PAD_LEFT) ? -1 : 0;
            if (PAD_REPEATED & PAD_RIGHT) {
                dummy_step = 1;
            }
            if (PAD_HELD & PAD_SQUARE) {
                dummy_step *= 100;
            }
            if (dummy_step != 0) {
                work.check = (work.check + dummy_step + MC_DUMMY_LIMIT_KB) % MC_DUMMY_LIMIT_KB;
            }
            xglFontDebugPrintf(16, 48, MC_TEXT_DUMMY_KEEP, work.check);
            xglFontDebugPrintf(16, 64, MC_TEXT_DUMMY_HELP);
            if (PAD_PRESSED & PAD_CIRCLE) {
                sceMcSync(0, &dummy_command, &dummy_result);
                xglMcEasySave("/BISLPS-99999Dummy", (void *)DEBUG_BUFFER_BASE,
                              (work.result - work.check) << 10);
                state = MCTEST_STATE_DUMMY_QUERY_FREE;
            }
            if (PAD_PRESSED & PAD_CROSS) {
                state = MCTEST_STATE_MENU_INIT;
            }
            break;

        case MCTEST_STATE_HDD_MENU_INIT:
            work.result = 0;
            state = MCTEST_STATE_HDD_MENU;
            /* fall through */
        case MCTEST_STATE_HDD_MENU:
            xglFontDebugPrintf(0, 32, "%s", func_001EAB10(port));
            xglFontDebugPrintf(16, 48, MC_TEXT_LOAD);
            xglFontDebugPrintf(16, 64, MC_TEXT_SAVE);
            hdd_menu_step = (PAD_PRESSED & PAD_UP) ? -1 : 0;
            if (PAD_PRESSED & PAD_DOWN) {
                hdd_menu_step = 1;
            }
            if (hdd_menu_step != 0) {
                work.result = (work.result + hdd_menu_step + MC_HDD_MENU_ITEM_COUNT) % MC_HDD_MENU_ITEM_COUNT;
            }
            xglFontDebugPrintf(8, work.result * 16 + 48, D_0069D028);
            if (PAD_PRESSED & PAD_CIRCLE) {
                state = tbl_49[work.result];
            }
            if (PAD_PRESSED & PAD_CROSS) {
                state = MCTEST_STATE_INIT;
            }
            break;

        case MCTEST_STATE_HDD_LOAD_LIST:
            hdd_list_request.card = -1;
            hdd_list_request.found = found;
            if (xglHddMcExist(&hdd_list_request) < 0) {
                state = MCTEST_STATE_HDD_LOAD_NO_FILE;
            } else {
                state = MCTEST_STATE_HDD_LOAD_SELECT;
            }
            break;

        case MCTEST_STATE_HDD_LOAD_NO_FILE:
            xglFontDebugPrintf(0, 32, MC_TEXT_HDD_LOAD_NO_FILE, work.file);
            if (PAD_PRESSED & PAD_CIRCLE) {
                state = MCTEST_STATE_HDD_MENU_INIT;
            }
            break;

        case MCTEST_STATE_HDD_LOAD_SELECT:
            xglFontDebugPrintf(0, 32, MC_TEXT_HDD_LOAD);
            dir = 0;
            if (PAD_REPEATED & PAD_LEFT) {
                dir = -1;
            }
            if (PAD_REPEATED & PAD_RIGHT) {
                dir = 1;
            }
            if (dir != 0) {
                work.file = (work.file + dir + MC_SAVE_FILE_COUNT) % MC_SAVE_FILE_COUNT;
            }
            xglFontDebugPrintf(16, 48, MC_TEXT_FILE_OF_99, work.file);
            if (found[work.file] == 0) {
                xglFontDebugPrintf(16, 64, MC_TEXT_FILE_ABSENT);
            } else {
                xglFontDebugPrintf(16, 64, MC_TEXT_FILE_PRESENT);
            }
            if (PAD_PRESSED & PAD_CIRCLE) {
                if (found[work.file] != 0) {
                    hdd_load_request.card = work.file;
                    hdd_load_request.data = SaveData;
                    hdd_load_request.size = SAVE_DATA_SIZE;
                    printf("xglHddMcLoad():%d\n", xglHddMcLoad(&hdd_load_request));
                    state = MCTEST_STATE_HDD_MENU_INIT;
                }
            }
            if (PAD_PRESSED & PAD_CROSS) {
                state = MCTEST_STATE_HDD_MENU_INIT;
            }
            break;

        case MCTEST_STATE_HDD_SAVE_INIT:
            work.result = 0;
            state = MCTEST_STATE_HDD_SAVE_SELECT_FILE;
            /* fall through */
        case MCTEST_STATE_HDD_SAVE_SELECT_FILE:
            xglFontDebugPrintf(0, 32, MC_TEXT_HDD_SAVE_FREE, work.result);
            hdd_file_step = (PAD_PRESSED & PAD_LEFT) ? -1 : 0;
            if (PAD_PRESSED & PAD_RIGHT) {
                hdd_file_step = 1;
            }
            if (hdd_file_step != 0) {
                work.file = (work.file + hdd_file_step + MC_SAVE_FILE_COUNT) % MC_SAVE_FILE_COUNT;
            }
            xglFontDebugPrintf(16, 48, MC_TEXT_FILE_NUMBER, work.file);
            if (PAD_PRESSED & PAD_CIRCLE) {
                int exist;

                hdd_check_request.card = work.file;
                state = MCTEST_STATE_HDD_SAVE_CREATE_CONFIRM;
                exist = xglHddMcExist(&hdd_check_request);
                printf("xglHddMcExist():%d\n", exist);
                if (exist < 0) {
                    state = MCTEST_STATE_HDD_MENU_INIT;
                }
            }
            if (PAD_PRESSED & PAD_CROSS) {
                state = MCTEST_STATE_HDD_MENU_INIT;
            }
            break;

        case MCTEST_STATE_HDD_SAVE_CREATE_CONFIRM:
            xglFontDebugPrintf(0, 32, MC_TEXT_HDD_SAVE_FREE, work.result);
            xglFontDebugPrintf(16, 48, MC_TEXT_CREATE_CONFIRM);
            if (PAD_PRESSED & PAD_CIRCLE) {
                hdd_create_request.status = work.file;
                hdd_create_request.transfer = files;
                hdd_create_request.dataSize = SAVE_DATA_SIZE;
                printf("xglHddMcCreate():%d\n", xglHddMcCreate_001A5B80(&hdd_create_request));
                state = MCTEST_STATE_HDD_SAVE_WRITE;
            }
            if (PAD_PRESSED & PAD_CROSS) {
                state = MCTEST_STATE_HDD_MENU_INIT;
            }
            break;

        case MCTEST_STATE_HDD_SAVE_OVERWRITE_CONFIRM:
            xglFontDebugPrintf(0, 32, MC_TEXT_HDD_SAVE_FREE, work.result);
            xglFontDebugPrintf(16, 48, MC_TEXT_OVERWRITE_CONFIRM);
            if (PAD_PRESSED & PAD_CIRCLE) {
                state = MCTEST_STATE_HDD_SAVE_WRITE;
            }
            if (PAD_PRESSED & PAD_CROSS) {
                state = MCTEST_STATE_HDD_MENU_INIT;
            }
            break;

        case MCTEST_STATE_HDD_SAVE_WRITE:
            hdd_save_request.card = work.file;
            hdd_save_request.data = SaveData;
            hdd_save_request.size = SAVE_DATA_SIZE;
            printf("xglHddMcSave():%d\n", xglHddMcSave(&hdd_save_request));
            state = MCTEST_STATE_HDD_MENU_INIT;
            break;
        }

        if (PAD_PRESSED & PAD_TRIANGLE) {
            flags = D_0063F8B4;
            for (i = 0; i < SAVE_FLAGS_COUNT; i++) {
                *flags++ = xglSRand();
            }
            FlushCache(0);
        }
        shown_flags = D_0063F8B4;
        xglFontDebugPrintf(16, 180, "%2x %2x %2x %2x %2x %2x %2x %2x", shown_flags[0], shown_flags[1],
                           shown_flags[2], shown_flags[3], shown_flags[4], shown_flags[5], shown_flags[6],
                           shown_flags[7]);
        xglSleep();
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu060", CdSlotTableView);

INCLUDE_ASM("asm/main/nonmatchings/tu060", func_001EC8E8);

INCLUDE_ASM("asm/main/nonmatchings/tu060", CdSlotTest);

INCLUDE_ASM("asm/main/nonmatchings/tu060", MovieListTest);

INCLUDE_ASM("asm/main/nonmatchings/tu060", IpuMovieTest);

INCLUDE_ASM("asm/main/nonmatchings/tu060", func_001ED1C0);

INCLUDE_ASM("asm/main/nonmatchings/tu060", func_001ED358);

/* Partial view of a point: the VU0 block reads only its X and Z components. */
typedef struct PointXZ {
    int x;
    int z;
} PointXZ;

float I2F();

/* Returns 1 when the VU0 orientation value is negative, 0 otherwise. */
int func_001ED420(PointXZ *origin, PointXZ *point1, PointXZ *point2, PointXZ *point3)
{
    float origin_vec[4];
    float point1_vec[4];
    float point2_vec[4];
    float point3_vec[4];
    float orientation;

    origin_vec[0] = I2F(origin->x);
    origin_vec[2] = I2F(origin->z);
    point1_vec[0] = I2F(point1->x);
    point1_vec[2] = I2F(point1->z);
    point2_vec[0] = I2F(point2->x);
    point2_vec[2] = I2F(point2->z);
    point3_vec[0] = I2F(point3->x);
    point3_vec[2] = I2F(point3->z);
    __asm__ __volatile__(
        "lqc2 vf10, 0(%1)\n\t"
        "lqc2 vf4, 0(%2)\n\t"
        "lqc2 vf5, 0(%3)\n\t"
        "lqc2 vf6, 0(%4)\n\t"
        "vsub.xz vf1, vf4, vf10\n\t"
        "vsub.xz vf2, vf5, vf10\n\t"
        "vsub.xz vf3, vf6, vf10\n\t"
        "vmul.xz vf7, vf1, vf1\n\t"
        "vmul.xz vf8, vf2, vf2\n\t"
        "vmul.xz vf9, vf3, vf3\n\t"
        "vaddz.x vf4, vf7, vf7z\n\t"
        "vaddz.x vf5, vf8, vf8z\n\t"
        "vaddz.x vf6, vf9, vf9z\n\t"
        "vrsqrt Q, vf0w, vf4x\n\t"
        "vwaitq\n\t"
        "vaddq.x vf4, vf0, Q\n\t"
        "vmulz.x vf7, vf1, vf2z\n\t"
        "vmulz.x vf8, vf2, vf1z\n\t"
        "vmulz.x vf9, vf1, vf3z\n\t"
        "vmulz.x vf10, vf3, vf1z\n\t"
        "vsub.x vf2, vf7, vf8\n\t"
        "vsub.x vf3, vf9, vf10\n\t"
        "vrsqrt Q, vf2x, vf5x\n\t"
        "vwaitq\n\t"
        "vmulq.x vf1, vf4, Q\n\t"
        "vrsqrt Q, vf3x, vf6x\n\t"
        "vwaitq\n\t"
        "vmulq.x vf2, vf4, Q\n\t"
        "vmul.x vf3, vf1, vf2\n\t"
        "qmfc2.ni %0, vf3"
        : "=r"(orientation)
        : "r"(origin_vec), "r"(&point1_vec[0]), "r"(&point2_vec[0]), "r"(&point3_vec[0])
        : "memory");
    /* Written as !(x >= 0) in the original: the compare is c.le.s, not c.lt.s. */
    if (!(!(orientation >= 0.0f))) {
        return 1;
    }
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu060", func_001ED568);
