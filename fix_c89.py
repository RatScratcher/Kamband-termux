import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    # Fix mixed declarations in smooth_caverns
    old_smooth = """	bool has_cavern = FALSE;

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

	byte temp_feat[DUNGEON_HGT][DUNGEON_WID];"""

    new_smooth = """	bool has_cavern = FALSE;
	byte temp_feat[DUNGEON_HGT][DUNGEON_WID];

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
	if (!has_cavern) return;"""

    content = content.replace(old_smooth, new_smooth)

    # Fix mixed declarations in ensure_elevation_access
    old_access = """        for (x = x1; x <= x2; x++) {
            if (!in_bounds(y, x)) continue;

            int target_elev = get_elevation(y, x);
            if ((target_elev == ELEV_HIGH || target_elev == ELEV_LOW) && !visited[y - y1][x - x1]) {"""

    new_access = """        for (x = x1; x <= x2; x++) {
            int target_elev;
            if (!in_bounds(y, x)) continue;

            target_elev = get_elevation(y, x);
            if ((target_elev == ELEV_HIGH || target_elev == ELEV_LOW) && !visited[y - y1][x - x1]) {"""

    content = content.replace(old_access, new_access)

    with open('src/generate.c', 'w') as f:
        f.write(content)

update_file()
