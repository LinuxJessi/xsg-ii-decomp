#ifndef INCLUDE_MAIN_SRS_H
#define INCLUDE_MAIN_SRS_H

/* Functions of src/main/srs.c that other files call. */

void srsChangeSeparator(char *path);
void srsInitCdRead(void);
int srsLeaveCdRead(void);
void srsResetKeyTable(void);

#endif /* INCLUDE_MAIN_SRS_H */
