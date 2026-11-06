#include "common.h"
#include "main/tu110.h"

int func_A9F0A8(int id);
void func_A9CC70(int id, int code);
int func_A9F0C0(int id, int mode);
void ptAddMember(int handle);
void ptRemoveMember(int handle);
void func_0022BAF8(void);

/* Script native "ptJoin": one argument; its handle (argument 1) goes to ptAddMember, then the refresh calls. */
int ptJoin(int context)
{
    int handle;

    if (func_A9F0A8(context) != 1) {
        func_A9CC70(context, 2);
        return 0;
    }
    handle = func_A9F0C0(context, 1);
    ptAddMember(handle);
    func_0022BAF8();
    ptRefreshCurrent();
    return 1;
}

/* Script native "ptLeave": one argument; its handle (argument 1) goes to ptRemoveMember, then the refresh calls. */
int ptLeave(int context)
{
    int handle;

    if (func_A9F0A8(context) != 1) {
        func_A9CC70(context, 2);
        return 0;
    }
    handle = func_A9F0C0(context, 1);
    ptRemoveMember(handle);
    func_0022BAF8();
    ptRefreshCurrent();
    return 1;
}

INCLUDE_ASM("asm/main/nonmatchings/tu080", menu_001FC700);

INCLUDE_ASM("asm/main/nonmatchings/tu080", ptSet);
