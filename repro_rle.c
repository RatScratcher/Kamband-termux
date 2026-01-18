#include <stdio.h>
#include <stdlib.h>

#define DUNGEON_HGT 10
#define DUNGEON_WID 10
#define MAX_UCHAR 255
typedef unsigned char byte;

byte cave_info[DUNGEON_HGT][DUNGEON_WID];
byte buffer[1024];
int buf_pos = 0;

void wr_byte(byte v) {
    if (buf_pos < 1024)
        buffer[buf_pos++] = v;
    else
        printf("Buffer overflow!\n");
}

void wr_dungeon_sim(int fix) {
    int y, x;
    byte tmp8u;
    byte count = 0;
    byte prev_char = 0;

    // Reset buffer
    buf_pos = 0;

    for (y = 0; y < DUNGEON_HGT; y++) {
        for (x = 0; x < DUNGEON_WID; x++) {
            tmp8u = cave_info[y][x];
            if ((tmp8u != prev_char) || (count == MAX_UCHAR)) {
                if (count) {
                    wr_byte(count);
                    wr_byte(prev_char);
                }
                prev_char = tmp8u;
                count = 1;
            } else {
                count++;
            }
        }
    }

    if (fix) {
        if (count) {
            wr_byte(count);
            wr_byte(prev_char);
        }
    }
}

int main() {
    int y, x;

    // Test Case 1: All zeros
    // Should result in a single run of 100 zeros.
    // Without flush: 0 bytes.
    // With flush: 2 bytes (100, 0).

    for (y = 0; y < DUNGEON_HGT; y++)
        for (x = 0; x < DUNGEON_WID; x++)
            cave_info[y][x] = 0;

    printf("Test Case 1: All zeros (100 items)\n");

    wr_dungeon_sim(0); // No fix
    printf("Without fix: Written bytes: %d\n", buf_pos);
    if (buf_pos != 0) printf("FAIL: Expected 0 bytes without flush.\n");
    else printf("PASS: Buffer empty as expected (bug reproduced).\n");

    wr_dungeon_sim(1); // With fix
    printf("With fix:    Written bytes: %d\n", buf_pos);
    if (buf_pos != 2) printf("FAIL: Expected 2 bytes with flush.\n");
    else printf("PASS: Buffer has data (100, 0).\n");

    if (buf_pos == 2) {
        printf("Data: count=%d, val=%d\n", buffer[0], buffer[1]);
    }

    // Test Case 2: 50 zeros then 50 ones.
    // Without flush: writes (50, 0), then buffers (50, 1) but discards it.
    // With flush: writes (50, 0), then (50, 1).

    for (y = 0; y < DUNGEON_HGT; y++)
        for (x = 0; x < DUNGEON_WID; x++)
            cave_info[y][x] = (y < 5) ? 0 : 1;

    printf("\nTest Case 2: 50 zeros, 50 ones\n");

    wr_dungeon_sim(0);
    printf("Without fix: Written bytes: %d\n", buf_pos);
    if (buf_pos != 2) printf("FAIL: Expected 2 bytes (first run only).\n"); // Should verify content too
    else printf("PASS: Missing second run.\n");

    wr_dungeon_sim(1);
    printf("With fix:    Written bytes: %d\n", buf_pos);
    if (buf_pos != 4) printf("FAIL: Expected 4 bytes.\n");
    else printf("PASS: Both runs written.\n");

    return 0;
}
