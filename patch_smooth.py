import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    old_smooth = """/*
 * Cellular Automata (CA) Cave Smoothing Pass.
 * Analyzes wall/floor neighbors and toggles them if they are overwhelmingly
 * one or the other. Softens the "boxiness" of cavern sectors.
 */
static void smooth_caverns(void)
{
	int i, y, x;
	int dir;
	byte temp_feat[DUNGEON_HGT][DUNGEON_WID];

	/* Run for 2 iterations */
	for (i = 0; i < 2; i++)
	{"""

    new_smooth = """/*
 * Cellular Automata (CA) Cave Smoothing Pass.
 * Analyzes wall/floor neighbors and toggles them if they are overwhelmingly
 * one or the other. Softens the "boxiness" of cavern sectors.
 */
static void smooth_caverns(void)
{
	int i, y, x;
	int dir;
	bool has_cavern = FALSE;

	/* Early exit if no SECTOR_CAVERN exists */
	for (y = 1; y < DUNGEON_HGT - 1; y++) {
		for (x = 1; x < DUNGEON_WID - 1; x++) {
			if (cave_sector[y][x] == SECTOR_CAVERN) {
				has_cavern = TRUE;
				break;
			}
		}
		if (has_cavern) break;
	}
	if (!has_cavern) return;

	byte temp_feat[DUNGEON_HGT][DUNGEON_WID];

	/* Run for 2 iterations */
	for (i = 0; i < 2; i++)
	{"""

    content = content.replace(old_smooth, new_smooth)

    with open('src/generate.c', 'w') as f:
        f.write(content)

update_file()
