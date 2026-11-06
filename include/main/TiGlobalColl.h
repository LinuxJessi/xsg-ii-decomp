#ifndef INCLUDE_MAIN_TIGLOBALCOLL_H
#define INCLUDE_MAIN_TIGLOBALCOLL_H

/* Functions of src/main/TiGlobalColl.c that other files call. */

void tiGlobalCollSetHandle(int handle);
void tiGlobalCollSolveBody(void *pNode, void *context);

#endif /* INCLUDE_MAIN_TIGLOBALCOLL_H */
