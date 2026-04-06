import re

def remove_function(content, func_name):
    pattern = r"static\s+void\s+" + func_name + r"\s*\([^)]*\)\s*\{?"
    match = re.search(pattern, content)
    if not match:
        return content

    start_idx = match.start()

    brace_start = content.find('{', match.end() - 1 if content[match.end() - 1] == '{' else match.end())

    if brace_start == -1:
        return content

    brace_count = 0
    in_function = False

    for i in range(brace_start, len(content)):
        if content[i] == '{':
            if not in_function:
                in_function = True
            brace_count += 1
        elif content[i] == '}':
            brace_count -= 1
            if in_function and brace_count == 0:
                end_idx = i + 1
                return content[:start_idx] + content[end_idx:]
    return content

def main():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    # Move ensure_connectivity definition to the top of file before it's used
    ensure_pattern = r"static\s+void\s+ensure_connectivity\s*\([^)]*\)\s*\{"
    match = re.search(ensure_pattern, content)
    if match:
        start_idx = match.start()
        brace_start = content.find('{', match.end() - 1 if content[match.end() - 1] == '{' else match.end())
        brace_count = 0
        in_function = False
        end_idx = -1
        for i in range(brace_start, len(content)):
            if content[i] == '{':
                if not in_function:
                    in_function = True
                brace_count += 1
            elif content[i] == '}':
                brace_count -= 1
                if in_function and brace_count == 0:
                    end_idx = i + 1
                    break

        if end_idx != -1:
            ensure_func = content[start_idx:end_idx]
            content = content[:start_idx] + content[end_idx:]

            # Place it after distance
            idx = content.find("static void generate_fractal_shape")
            if idx == -1:
                idx = content.find("static void build_sector_cavern")
            content = content[:idx] + ensure_func + "\n" + content[idx:]


    # Remove all the deprecated ones
    content = remove_function(content, "build_sector_hill")
    content = remove_function(content, "build_sector_pit")
    content = remove_function(content, "build_sector_cliff")
    content = remove_function(content, "build_sector_fractal_pit")
    content = remove_function(content, "build_sector_fractal_hill")
    content = remove_function(content, "apply_sector_borders_and_access")
    content = remove_function(content, "generate_fractal_shape")

    populated_code = """
/*
 * Stays within a specified radius to create a standalone feature.
 * This can be called multiple times within one sector.
 */
static void stamp_organic_feature(int y, int x, int rad, int threshold, int feat, int elev)
{
    int dy, dx;
    /* We only process the bounding box of the feature */
    for (dy = -rad; dy <= rad; dy++) {
        for (dx = -rad; dx <= rad; dx++) {
            int ty = y + dy;
            int tx = x + dx;
            if (!in_bounds(ty, tx)) continue;

            /* Calculate a distance-based falloff so the fractal
               tapers off naturally at the edges of the stamp */
            int dist = distance(y, x, ty, tx);
            if (dist > rad) continue;

            /* Add some plasma noise for cragginess */
            int noise = rand_int(40);
            if (noise + (rad - dist) * 10 > threshold) {
                cave_feat[ty][tx] = feat;
                set_elevation(ty, tx, elev);
            }
        }
    }
}

/*
 * Populates a sector using stamps of organic features
 * instead of covering the entire sector.
 */
static void build_sector_populated(int y0, int x0) {
    int y1 = y0 * BLOCK_HGT, x1 = x0 * BLOCK_WID;
    int y2 = (y0 + 2) * BLOCK_HGT - 1, x2 = (x0 + 2) * BLOCK_WID - 1;

    if (y2 >= DUNGEON_HGT) y2 = DUNGEON_HGT - 1;
    if (x2 >= DUNGEON_WID) x2 = DUNGEON_WID - 1;

    /* Initialize the area as flat ground floor */
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            if (!in_bounds(y, x)) continue;
            set_elevation(y, x, ELEV_GROUND);
            cave_feat[y][x] = FEAT_FLOOR;
            cave_info[y][x] |= CAVE_ROOM;
        }
    }

    /* 1. Primary Terrain: small anchor features */
    int num_anchors = rand_range(2, 4);
    for (int i = 0; i < num_anchors; i++) {
        int cy = rand_range(y1+4, y2-4);
        int cx = rand_range(x1+4, x2-4);
        int rad = rand_range(3, 5);
        int type_roll = rand_int(100);

        if (type_roll < 45) {
            /* Hill stamp */
            stamp_organic_feature(cy, cx, rad, 40, FEAT_HILL_TOP, ELEV_HIGH);
        } else if (type_roll < 90) {
            /* Pit stamp */
            stamp_organic_feature(cy, cx, rad, 40, FEAT_CLIFF_DOWN, ELEV_LOW);
            /* Inner bottom for pits to give them some depth/safety mechanics */
            stamp_organic_feature(cy, cx, rad - 1, 30, FEAT_RUBBLE, ELEV_LOW);
        } else {
            /* Occasional streamer */
            build_streamer2(FEAT_DEEP_WATER, 3);
        }
    }

    /* 2. Tactical Debris: Filling the 'Empty' space */
    int num_debris = rand_range(4, 8);
    for (int i = 0; i < num_debris; i++) {
        int ty = rand_range(y1+2, y2-2);
        int tx = rand_range(x1+2, x2-2);

        /* Only place if it's currently empty floor */
        if (cave_naked_bold(ty, tx)) {
            int roll = rand_int(100);
            if (roll < 20) cave_feat[ty][tx] = FEAT_STONE_PILLAR;
            else if (roll < 40) cave_feat[ty][tx] = FEAT_RUBBLE;
            else if (roll < 60) cave_feat[ty][tx] = FEAT_TREES;
            else if (roll < 80) cave_feat[ty][tx] = FEAT_BOULDER;
            else cave_feat[ty][tx] = FEAT_MUD;
        }
    }

    /* 3. Atmospheric Detail */
    int num_detail = rand_range(5, 10);
    for (int i = 0; i < num_detail; i++) {
        int ty = rand_range(y1+1, y2-1);
        int tx = rand_range(x1+1, x2-1);

        if (cave_naked_bold(ty, tx)) {
            int roll = rand_int(100);
            if (roll < 50) cave_feat[ty][tx] = FEAT_GRASS;
            else if (roll < 80) cave_feat[ty][tx] = FEAT_MUD;
            else cave_feat[ty][tx] = FEAT_SHAL_WATER; /* Puddle */
        }
    }

    /* 4. Ensure Connectivity to avoid getting walled in by overlapping hills/pits */
    ensure_connectivity(y1, x1, y2, x2);
}
"""

    idx = content.find("static void build_sector_cavern")
    content = content[:idx] + populated_code + "\n" + content[idx:]

    # Also replace calls to fractal functions with build_sector_populated
    content = content.replace("build_sector_fractal_hill(y, x);", "build_sector_populated(y, x);")
    content = content.replace("build_sector_fractal_pit(y, x);", "build_sector_populated(y, x);")
    content = content.replace("build_sector_hill(y, x);", "build_sector_populated(y, x);")
    content = content.replace("build_sector_pit(y, x);", "build_sector_populated(y, x);")

    with open('src/generate.c', 'w') as f:
        f.write(content)

if __name__ == "__main__":
    main()
