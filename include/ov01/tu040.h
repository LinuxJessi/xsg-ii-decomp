#ifndef INCLUDE_OV01_TU040_H
#define INCLUDE_OV01_TU040_H

/* Functions of src/ov01/tu040.c that other files call. */

struct SbImageHeader;
struct VmKeyTable;

struct SbImageHeader *ov01ScriptKeyGet(void);
struct VmKeyTable *ov01ScriptImageGet(void);
void func_00A9B0F8(void);

#endif
