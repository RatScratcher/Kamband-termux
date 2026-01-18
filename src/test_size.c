#include "angband.h"

int main(void) {
    printf("DUNGEON_HGT: %d\n", DUNGEON_HGT);
    printf("DUNGEON_WID: %d\n", DUNGEON_WID);
    printf("BLOCK_HGT: %d\n", BLOCK_HGT);
    printf("BLOCK_WID: %d\n", BLOCK_WID);
    printf("MAX_ROOMS_ROW (calculated): %d\n", DUNGEON_HGT / BLOCK_HGT);
    printf("MAX_ROOMS_COL (calculated): %d\n", DUNGEON_WID / BLOCK_WID);
    return 0;
}
