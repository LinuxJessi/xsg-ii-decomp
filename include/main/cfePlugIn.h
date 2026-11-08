#ifndef INCLUDE_MAIN_CFEPLUGIN_H
#define INCLUDE_MAIN_CFEPLUGIN_H

/* Functions of src/main/cfePlugIn.c that other files call. */

void cfeRegisterPlugIn(void);

/* One 0x50-byte sound record of cfeSndRecords: the name is copied to offset 0 and the tween
   fields sit at +0x40..+0x4C (cfeEvaluate.c's func_00249248 tweens them). */
typedef struct CfeSndRecord {
    char name[0x40];
    int value;    /* +0x40: moves toward the target */
    int duration; /* +0x44 */
    unsigned char unmodeled_48[8];
} CfeSndRecord;

/* Sound records in use: cfeSndRecordCount of them. */
extern CfeSndRecord cfeSndRecords[];
extern int cfeSndRecordCount;

#endif /* INCLUDE_MAIN_CFEPLUGIN_H */
