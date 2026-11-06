#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu125", textMarkerScan);

extern unsigned char textMarkerFound;
extern signed char *textMarkerPtrs[];

/* Writes the 0xA1 0xFC page marker back at every position textMarkerScan recorded, then clears the count. */
void textMarkerRestore(void) {
    int markerIndex;

    markerIndex = 0;
    if (textMarkerFound != 0) {
        do {
            textMarkerPtrs[markerIndex][0] = -0x5F;
            textMarkerPtrs[markerIndex][1] = -4;
            markerIndex++;
        } while (markerIndex < textMarkerFound);
    }
    textMarkerFound = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu125", textMarkerCount);

INCLUDE_ASM("asm/main/nonmatchings/tu125", textEntryUpdate);

INCLUDE_ASM("asm/main/nonmatchings/tu125", textDrawControlCode);

INCLUDE_ASM("asm/main/nonmatchings/tu125", func_00236890);

INCLUDE_ASM("asm/main/nonmatchings/tu125", textDrawString);

INCLUDE_ASM("asm/main/nonmatchings/tu125", textDrawAll);

extern unsigned char textBlock[];

/* Clears the counter (+8) and the state byte (+9) of the text block at textBlock. */
void textBlockClear(void) {
    textBlock[9] = 0;
    textBlock[8] = 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu125", textBlockStart);

INCLUDE_ASM("asm/main/nonmatchings/tu125", textBlockIsActive);

INCLUDE_ASM("asm/main/nonmatchings/tu125", textBlockUpdate);

INCLUDE_ASM("asm/main/nonmatchings/tu125", textUpdate);
