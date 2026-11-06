#include "common.h"
#include "main/tu121.h"

void func_00233D50(void) {

}

/* Resolves a table entry as an offset relative to the table base: table + table[index]. */
int txdTableEntry(int *table, int index) {
    return (int)table + table[index];
}

void func_00233D70(void) {

}
