#include "common.h"
#include "shared.h"

struct request {
    u8 mode;
    u8 data;
};

typedef struct {
    unsigned char status;
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned short year;
} XglClock;

struct stat;

int stat(const char *path, struct stat *status);

extern u8 *buffer;
extern unsigned int strlen(const char *string);
extern int strcmp(const char *left, const char *right);
extern int strncmp(const char *left, const char *right, unsigned int count);
extern int xglHddActivate(int state);
extern int xglHddMcUmount(void);
extern int xglHddMcLoadMount(void);
extern int xglHddMcLoadCore(void *save);

#define HDD_CHECK_TOO_MANY_FOLDERS (-2)
#define HDD_CHECK_TOO_MANY_SAVES (-3)
#define HDD_CHECK_NO_SPACE (-4)
#define HDD_DEVCTL_FLUSH 0x4804
#define HDD_DEVCTL_STATUS 0x4807
#define HDD_DEVCTL_FREE_SECTORS_TOTAL 0x480a
#define HDD_DEVCTL_ZONE_SIZE 0x5001
#define HDD_DEVCTL_FREE_ZONES 0x5002

/* The header of the card data image: the begin and end of the transferred
 * payload (carddata.new). */
struct HddTransferInfo {
    int unmodeled_00;
    int begin;
    int end;
};

struct HddSaveImage;

/* The card save request: xglHddMcCheck/xglHddMcCreate read it through
 * `transfer`, which file.c fills with its card data buffer. */
struct HddCheckState {
    int status;
    union {
        unsigned char *bytes; /* file.c: the card data buffer it hands in */
        struct HddTransferInfo *info; /* xgl_hdd: the begin/end transfer header */
        struct HddSaveImage *image; /* xgl_hdd: the whole saved image, header first */
    } transfer;
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

/*
 * The xglHdd layer keeps its own copy of the "hdd0:" device name at
 * main:0x004DC368; the HddTest debug tool has an identical string of its own
 * at 0x004DA2D0, which accepted source calls hdd_device.  Neither address
 * carries an original symbol, so this one is named for the layer that owns
 * it.
 */
extern char xgl_hdd_device[];

extern char hdd_mc_path[];
extern char mount_device[];
extern int xglHddMcGetFree(void);

int xglHddMcExist(struct HddExistRequest *request);
int xglHddMcSave(const struct HddSaveRequest *save);

/* Size of the icon.sys image that opens every save (the icon header). */
#define MC_ICON_SYS_SIZE 964

char *xglMcSetFullPath(int card, int selector);

unsigned int xglClockDayTime2UInt(XglClock *clock_time);

void xglCdArcCheck(void);

struct HddIoStat {
    unsigned int mode;
    unsigned int attributes;
    unsigned int size;
    unsigned char created[8];
    unsigned char accessed[8];
    unsigned char modified[8];
    unsigned int sizeHigh;
    unsigned int privateData[6];
};

struct HddFileStat {
    unsigned int mode;
    unsigned int flags;
    unsigned char unmodeled_08[21];
    unsigned char second;
    unsigned char minute;
    unsigned char hour;
    unsigned char day;
    unsigned char month;
    unsigned short year;
    unsigned char unmodeled_24[4];
    unsigned int privateData[6];
};

struct HddDirectoryEntry {
    struct HddFileStat stat;
    char name[256];
    unsigned char unmodeled_140[16];
};

extern u8 system_cnf[];

int sceRead(int descriptor, void *buffer, int bytes);
int xglHddCheck2(void);
int xglHddCheckCore(void);

extern int sceUmount(const char *path);

int xglHddMcLoad(void *save);
int xglHddUninstall(void);

extern int sceRemove(const char *path);
extern void xglCdArcCheck(void);

extern int sceMount(char *mount_point, char *device, int flags,
                    void *payload, unsigned int payload_length);

extern int sceDevctl(const char *device, int command, const void *input,
                     unsigned int input_size, void *output, unsigned int output_size);

extern int sceOpen(const char *path, int flags, ...);
extern int sceDopen(const char *path);
extern int sceDread(int descriptor, struct HddDirectoryEntry *entry);
extern int sceDclose(int descriptor);
extern int sceGetstat(const char *path, struct HddFileStat *stat);
extern int sceClose(int descriptor);
extern int strncmp(const char *left, const char *right, unsigned int length);
extern unsigned int xglClockDayTime2UInt(XglClock *clock_time);
extern int sceWrite(int descriptor, const void *buffer, int size);
extern int sceChstat(const char *path, const struct HddIoStat *stat, int flags);

int Judge_MakeNewFolder(void);

extern int Judge_MakeNewSavedata(int descriptor, int card);

static int xglHddMcCheckYourSaves(int);

int xglHddMcCheckCore(struct HddCheckState *state);

extern int xglHddMcCheck(struct HddCheckState *state);

int create_file(int card, int slot, const void *data, int size);

/* sceOpen flags and permission modes (0666, 0555, 0777). */
#define HDD_OPEN_READ 1

#define HDD_OPEN_WRITE_CREATE_TRUNC 0x602
#define HDD_MODE_RW 438

/* sceMount flags, and the errors the layer tests for. */
#define HDD_MOUNT_READ_WRITE 4

#define HDD_MOUNT_READ_ONLY 5
#define HDD_ERR_NOENT (-2)
#define HDD_ERR_BUSY (-16)
#define HDD_ERR_NODEV (-19)

/* Results of xglHddCheckCore() beyond 0 (ready). */
#define HDD_NOT_READY (-5)

#define HDD_RESET_REQUIRED (-6)
#define HDD_PATH_MAX 256
#define HDD_DIR_SCAN_LIMIT 1024
#define HDD_MAX_FOLDERS 256
#define HDD_FOLDER_ENTRY_LIMIT 0x3FE
#define HDD_SLACK_ZONES 7

/* Total free sectors above which the save area can be grown, and the free
 * sectors (total, and the largest free block) the game installation needs. */
#define HDD_LARGE_DISK_SECTORS 0x1fffff

#define HDD_LARGE_DISK_FREE_KB 0x100000
#define HDD_SAVE_ERR_WRITE (-24)

/* xglHddMcLoadCore() */
#define HDD_LOAD_ERR_OPEN (-6)

#define HDD_LOAD_ERR_READ (-7)
#define HDD_LOAD_ERR_CLOSE (-8)

/* The saved image begins with the transfer header. icon.sys occupies +0x40
 * through +0x403; the game payload starts at +0x440. The gaps remain unknown. */
struct HddSaveImage {
    struct HddTransferInfo header;
    unsigned char unmodeled_0c[0x40 - 0x0c];
    char iconSystem[MC_ICON_SYS_SIZE];
    unsigned char unmodeled_404[0x440 - 0x404];
    unsigned char payloadStart; /* First byte; the transfer header gives size. */
};

/* xglHddMount removes the high bit from this stored volume identifier. */
extern char partitionname[];

extern char commonname[];
extern char yoursaves[];
extern char pfs1RootPath[];
extern char currentDirEntryName[];
extern char parentDirEntryName[];
extern char commonname[14];
extern char yoursaves[17];
extern char xgl_hdd_device[6];
extern char hdd_mc_path[6];
extern char pfs1RootPath[8];
extern char currentDirEntryName[8];
extern char parentDirEntryName[8];
extern u8 HddActive;
extern char mount_device[6];

INCLUDE_ASM("asm/main/nonmatchings/xgl_hdd", func_001A4EC8);

INCLUDE_ASM("asm/main/nonmatchings/xgl_hdd", xglHddCheck2);

INCLUDE_ASM("asm/main/nonmatchings/xgl_hdd", func_001A4F08);

INCLUDE_ASM("asm/main/nonmatchings/xgl_hdd", xglHddErrorScreen);

int xglHddMcLoadMount(void)
{
    int result;

    if (xglHddCheck2() != 0)
        return -1;

    result = sceMount(hdd_mc_path, commonname, HDD_MOUNT_READ_ONLY, 0, 0);
    if (result < 0 && result != HDD_ERR_BUSY)
        return -1;
    return 0;
}

int xglHddMcUmount(void)
{
    int result = sceUmount(hdd_mc_path);
    return result < 0 ? -2 : 0;
}

char *make_fullpath(char *destination, int card, int slot)
{
    const char *source = yoursaves;

    while ((*destination = *source) != 0) {
        source++;
        destination++;
    }

    source = xglMcSetFullPath(card, slot);
    while ((*destination = *source) != 0) {
        source++;
        destination++;
    }
    return destination;
}

int xglHddMcExist(struct HddExistRequest *request)
{
    char filePath[HDD_PATH_MAX];
    char name[HDD_PATH_MAX];
    struct HddDirectoryEntry entry;
    char savePath[HDD_PATH_MAX];
    XglClock timestamp;
    int result;
    int descriptor;
    int save;
    int count;
    int newestSave;
    unsigned int newestTime;
    unsigned int time;
    unsigned char *found;
    char *source;
    char *destination;
    unsigned char *clearPosition;

    result = xglHddMcLoadMount();
    if (result < 0)
        return result;

    if (request->card >= 0) {
        make_fullpath(filePath, request->card, 0);
        descriptor = sceOpen(filePath, HDD_OPEN_READ, HDD_MODE_RW);
        if (descriptor >= 0) {
            result = sceClose(descriptor) < 0 ? -3 : 1;
        } else {
            result = 0;
        }
    } else {
        found = request->found;
        source = xglMcSetFullPath(-1, -1);
        source++;
        newestTime = 0;
        newestSave = 0;
        destination = name;
        *destination = *source;
        if (((unsigned int)*destination << 24) != 0) {
            do {
                source++;
                destination++;
                *destination = *source;
            } while (((unsigned int)*destination << 24) != 0);
        }
        /* The -1 card path ends in a wildcard; compare its directory stem. */
        destination[-1] = 0;

        count = 100;
        clearPosition = &found[100];
        do {
            count--;
            *clearPosition = 0;
            clearPosition--;
        } while (count >= 0);

        count = 0;
        descriptor = sceDopen(yoursaves);
        while (descriptor >= 0 && sceDread(descriptor, &entry) > 0) {
            if (strncmp(name, entry.name, 16) != 0)
                continue;
            save = entry.name[16] * 10 + entry.name[17] - (10 * '0' + '0');
            make_fullpath(savePath, save, 0);
            if (sceGetstat(savePath, &entry.stat) < 0)
                continue;
            found[save] = 1;
            timestamp.status = 0;
            timestamp.year = entry.stat.year;
            timestamp.month = entry.stat.month;
            timestamp.day = entry.stat.day;
            timestamp.hour = entry.stat.hour;
            timestamp.minute = entry.stat.minute;
            timestamp.second = entry.stat.second;
            time = xglClockDayTime2UInt(&timestamp);
            if (newestTime < time) {
                newestTime = time;
                newestSave = save;
            }
            count++;
        }
        found[100] = newestSave;
        result = count;
        if (descriptor >= 0 && sceDclose(descriptor) < 0)
            result = -5;
    }
    xglHddMcUmount();
    return result;
}

int xglHddMcLoadCore(void *save)
{
    struct HddLoadRequest *request = save;
    char path[HDD_PATH_MAX];
    int descriptor;
    int result;

    make_fullpath(path, request->card, 0);
    descriptor = sceOpen(path, HDD_OPEN_READ, HDD_MODE_RW);
    if (descriptor < 0) {
        result = descriptor == HDD_ERR_NOENT ? 1 : HDD_LOAD_ERR_OPEN;
    } else {
        result = 0;
        if (sceRead(descriptor, request->data, request->size) < 0)
            result = HDD_LOAD_ERR_READ;
        if (sceClose(descriptor) < 0)
            result = HDD_LOAD_ERR_CLOSE;
    }
    return result;
}

int xglHddMcLoad(void *save)
{
    int result;

    result = xglHddMcLoadMount();
    if (result < 0)
        return result;
    result = xglHddMcLoadCore(save);
    if (result < 0)
        return result;
    return xglHddMcUmount();
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_hdd", Judge_MakeNewFolder);

int Judge_MakeNewSavedata(int descriptor, int card)
{
    extern unsigned int strlen(const char *string);
    extern char *strchr(const char *string, int character);
    struct HddDirectoryEntry entry;
    int directoryCount;
    int entriesRead;
    int result;
    int closeResult;
    char *path;
    char *entryName;
    unsigned int pathLength;

    directoryCount = 0;
    result = 0;
    path = xglMcSetFullPath(card, -1) + 1;
    if (card < 0) {
        char *end = path;

        if (*path != '\0') {
            do {
                end++;
            } while (*end != '\0');
        }
        end[-1] = '\0';
    }
    pathLength = strlen(path);

    for (entriesRead = 0; entriesRead < HDD_DIR_SCAN_LIMIT; entriesRead++) {
        result = sceDread(descriptor, &entry);
        if (result <= 0)
            break;

        entryName = entry.name;
        if (strcmp(entryName, currentDirEntryName) == 0 ||
            strcmp(entryName, parentDirEntryName) == 0 ||
            strncmp(entryName, path, pathLength) == 0)
            continue;

        if ((entry.stat.mode & 0xF000) == 0x1000 &&
            (entry.stat.flags & 0x4000) == 0)
            continue;

        if (strchr(entryName, ':') != 0 &&
            (entry.stat.mode & 0xF000) == 0x2000)
            continue;

        if ((entry.stat.mode & 0xF000) == 0x4000)
            continue;
        if (entry.stat.privateData[0] != 0xFFFF)
            continue;
        if (entry.stat.privateData[1] == entry.stat.privateData[0])
            directoryCount++;
    }

    closeResult = sceDclose(descriptor);
    if (result >= 0)
        result = closeResult;

    if (result < 0)
        return result;
    return directoryCount < HDD_FOLDER_ENTRY_LIMIT;
}

static int xglHddMcCheckYourSaves(int card)
{
    int descriptor = sceDopen(yoursaves);

    if (descriptor < 0)
        return Judge_MakeNewFolder() == 1 ? 1 : HDD_CHECK_TOO_MANY_FOLDERS;
    return Judge_MakeNewSavedata(descriptor, card) == 1 ? 0 : HDD_CHECK_TOO_MANY_SAVES;
}

int xglHddMcCheckCore(struct HddCheckState *state)
{
    int result = xglHddMcCheckYourSaves(state->status);

    if (result < 0)
        return result;
    {
        int clusterSize = sceDevctl(hdd_mc_path, HDD_DEVCTL_ZONE_SIZE, 0, 0, 0, 0);
        int freeClusters = sceDevctl(hdd_mc_path, HDD_DEVCTL_FREE_ZONES, 0, 0, 0, 0);
        int dataClusters = (state->dataSize + clusterSize - 1) / clusterSize;
        int reserveClusters = ((unsigned int)clusterSize + MC_ICON_SYS_SIZE - 1) / (unsigned int)clusterSize;
        int transferClusters =
            ((unsigned int)(state->transfer.info->end - state->transfer.info->begin) + clusterSize - 1)
            / (unsigned int)clusterSize;

        dataClusters += reserveClusters;
        if (freeClusters < dataClusters + transferClusters + HDD_SLACK_ZONES) {
            int capacity;

            if (sceDevctl(xgl_hdd_device, HDD_DEVCTL_FREE_SECTORS_TOTAL, 0, 0, &capacity, 4) < 0)
                return -1;
            return capacity <= HDD_LARGE_DISK_SECTORS ? HDD_CHECK_NO_SPACE : 1;
        }
    }
    return 0;
}

int xglHddMcCheck(struct HddCheckState *state)
{
    int previous;
    int result;

    if (xglHddCheck2() != 0)
        return -1;

    xglHddMcUmount();
    if (sceMount(hdd_mc_path, commonname, HDD_MOUNT_READ_WRITE, 0, 0) < 0)
        return -1;

    previous = state->status;
    state->status = -1;
    result = xglHddMcCheckCore(state);
    state->status = previous;
    xglHddMcUmount();
    return result;
}

int xglHddMcGetFree(void)
{
    int result;
    int freeSpace;
    int clusterSize;
    int freeClusters;
    int capacity;

    result = xglHddMcLoadMount();
    if (result >= 0) {
        freeSpace = xglHddMcCheckYourSaves(-1);
        if (freeSpace >= 0) {
            if (sceDevctl(xgl_hdd_device, HDD_DEVCTL_FREE_SECTORS_TOTAL, 0, 0, &capacity, 4) == 0
                && capacity > HDD_LARGE_DISK_SECTORS) {
                freeSpace = HDD_LARGE_DISK_FREE_KB;
            } else {
                clusterSize = sceDevctl(hdd_mc_path, HDD_DEVCTL_ZONE_SIZE, 0, 0, 0, 0);
                freeClusters = sceDevctl(hdd_mc_path, HDD_DEVCTL_FREE_ZONES, 0, 0, 0, 0);
                freeSpace = (freeClusters < 0 ? 0 : freeClusters) * (clusterSize / 1024);
            }
        }
        xglHddMcUmount();
        result = freeSpace;
    }
    return result;
}

int create_file(int card, int slot, const void *data, int size)
{
    char path[HDD_PATH_MAX];
    struct HddIoStat stat;
    int descriptor;
    int result;

    make_fullpath(path, card, slot);
    descriptor = sceOpen(path, HDD_OPEN_WRITE_CREATE_TRUNC, HDD_MODE_RW);
    if (descriptor < 0)
        return -1;

    result = 0;
    if (sceWrite(descriptor, data, size) < 0)
        result = -1;
    if (sceClose(descriptor) < 0)
        result = -1;
    stat.attributes = 0x8497;
    if (sceChstat(path, &stat, 2) < 0)
        result = -1;
    return result;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_hdd", xglHddMcCreate_001A5B80);

int xglHddMcSave(const struct HddSaveRequest *save)
{
    int result;

    if (xglHddCheck2() != 0)
        return -1;
    if (sceMount(hdd_mc_path, commonname, HDD_MOUNT_READ_WRITE, 0, 0) < 0)
        return -1;

    result = create_file(save->card, 0, save->data, save->size) < 0 ? HDD_SAVE_ERR_WRITE : 0;
    xglHddMcUmount();
    return result;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_hdd", func_001A5ED0);

int xglHddUninstall(void)
{
    int result;

    xglHddActivate(0);
    result = sceUmount(mount_device);
    if (result < 0 && result != HDD_ERR_NODEV)
        return -1;
    result = sceRemove(partitionname);
    return result < 0 ? -5 : 0;
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_hdd", xglHddInstallReadCB);

INCLUDE_ASM("asm/main/nonmatchings/xgl_hdd", func_001A5FF8);

int xglHddActivate(int state)
{
    int previous = HddActive;

    if (state != -1) {
        if (previous != 2 && (state & 0xff) < 4 && (state & 0xff) >= 0) {
            HddActive = (u8)state;
            if (state < 0x100)
                xglCdArcCheck();
        }
    }
    return previous < 2 ? previous : 0;
}

void decrypt(void)
{
    u8 *source = partitionname;
    int value = *source;
    register int masked = value & 0xff;
    register u8 *destination = source;

    if (masked != 0x68) {
        if (masked != 0) {
            do {
                *destination = (unsigned int)value ^ 0x80;
                source++;
                value = *source;
                destination++;
            } while (value != 0);
        }
    }
    partitionname[17] = system_cnf[0x1a];
}

INCLUDE_ASM("asm/main/nonmatchings/xgl_hdd", xglHddMount);
