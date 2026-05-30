import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    # 1. ensure_elevation_access change
    old_access_code = """static void ensure_elevation_access(int y1, int x1, int y2, int x2, int target_elev, int edge_feat)
{
    int y, x, dy, dx;
    int **visited;

    visited = (int **)malloc(DUNGEON_HGT * sizeof(int *));
    for (y = 0; y < DUNGEON_HGT; y++) {
        visited[y] = (int *)calloc(DUNGEON_WID, sizeof(int));
    }

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            if (!in_bounds(y, x)) continue;

            if (get_elevation(y, x) == target_elev && !visited[y][x]) {
                /* New connected component found. We will collect all edge tiles for this component. */
                int *edge_y = (int *)malloc(DUNGEON_HGT * DUNGEON_WID * sizeof(int));
                int *edge_x = (int *)malloc(DUNGEON_HGT * DUNGEON_WID * sizeof(int));
                int edge_count = 0;

                int *q_y = (int *)malloc(DUNGEON_HGT * DUNGEON_WID * sizeof(int));
                int *q_x = (int *)malloc(DUNGEON_HGT * DUNGEON_WID * sizeof(int));
                int head = 0, tail = 0;

                q_y[tail] = y;
                q_x[tail] = x;
                tail++;
                visited[y][x] = 1;

                int comp_size = 0;

                while (head < tail) {
                    int cy = q_y[head];
                    int cx = q_x[head];
                    head++;
                    comp_size++;

                    /* Check if this tile is an edge tile: has edge_feat and adjacent to ELEV_GROUND */
                    if (cave_feat[cy][cx] == edge_feat) {
                        bool adjacent_to_ground = FALSE;
                        for (dy = -1; dy <= 1; dy++) {
                            for (dx = -1; dx <= 1; dx++) {
                                int ny = cy + dy, nx = cx + dx;
                                if (in_bounds(ny, nx) && get_elevation(ny, nx) == ELEV_GROUND) {
                                    adjacent_to_ground = TRUE;
                                    break;
                                }
                            }
                            if (adjacent_to_ground) break;
                        }
                        if (adjacent_to_ground) {
                            edge_y[edge_count] = cy;
                            edge_x[edge_count] = cx;
                            edge_count++;
                        }
                    }

                    /* Expand to neighbors (8-way connected) */
                    for (dy = -1; dy <= 1; dy++) {
                        for (dx = -1; dx <= 1; dx++) {
                            if (dy == 0 && dx == 0) continue;
                            int ny = cy + dy, nx = cx + dx;
                            if (in_bounds(ny, nx) && ny >= y1 && ny <= y2 && nx >= x1 && nx <= x2) {
                                if (get_elevation(ny, nx) == target_elev && !visited[ny][nx]) {
                                    visited[ny][nx] = 1;
                                    q_y[tail] = ny;
                                    q_x[tail] = nx;
                                    tail++;
                                }
                            }
                        }
                    }
                }

                /* Now we have all edge tiles for this component. Pick one to carve access. */
                if (comp_size > 1 && edge_count > 0) {
                    int pick = rand_int(edge_count);
                    int cy = edge_y[pick];
                    int cx = edge_x[pick];

                    /* Unified transit features */
                    int roll = rand_int(100);
                    if (roll < 40) cave_feat[cy][cx] = FEAT_RAMP;
                    else if (roll < 70) cave_feat[cy][cx] = FEAT_LADDER;
                    else cave_feat[cy][cx] = FEAT_STAIRS;

                    /* Clear hazards on ground or plateau landings */
                    for (dy = -1; dy <= 1; dy++) {
                        for (dx = -1; dx <= 1; dx++) {
                            int ny = cy + dy, nx = cx + dx;
                            if (!in_bounds(ny, nx)) continue;

                            if (get_elevation(ny, nx) == ELEV_GROUND || get_elevation(ny, nx) == target_elev) {
                                if (cave_feat[ny][nx] != edge_feat &&
                                    cave_feat[ny][nx] != FEAT_RAMP &&
                                    cave_feat[ny][nx] != FEAT_LADDER &&
                                    cave_feat[ny][nx] != FEAT_STAIRS) {

                                    /* Maintain floor type depending on elevation */
                                    if (get_elevation(ny, nx) == ELEV_LOW)
                                        cave_feat[ny][nx] = FEAT_PIT; /* Pit landing */
                                    else if (get_elevation(ny, nx) == ELEV_HIGH)
                                        cave_feat[ny][nx] = FEAT_HILL_TOP; /* Hill landing */
                                    else
                                        cave_feat[ny][nx] = FEAT_FLOOR; /* Ground landing */
                                }
                            }
                        }
                    }
                }

                free(q_y);
                free(q_x);
                free(edge_y);
                free(edge_x);
            }
        }
    }

    for (y = 0; y < DUNGEON_HGT; y++) {
        free(visited[y]);
    }
    free(visited);
}"""

    new_access_code = """static void ensure_elevation_access(int y1, int x1, int y2, int x2)
{
    int y, x, dy, dx;
    int h = y2 - y1 + 1;
    int w = x2 - x1 + 1;

    if (h > 24) h = 24;
    if (w > 24) w = 24;

    int visited[24][24];
    for (y = 0; y < 24; y++) {
        for (x = 0; x < 24; x++) {
            visited[y][x] = 0;
        }
    }

    for (y = y1; y <= y2; y++) {
        for (x = x1; x <= x2; x++) {
            if (!in_bounds(y, x)) continue;

            int target_elev = get_elevation(y, x);
            if ((target_elev == ELEV_HIGH || target_elev == ELEV_LOW) && !visited[y - y1][x - x1]) {
                int edge_feat = (target_elev == ELEV_HIGH) ? FEAT_CLIFF_UP : FEAT_CLIFF_DOWN;

                /* New connected component found. We will collect all edge tiles for this component. */
                int edge_y[576];
                int edge_x[576];
                int edge_count = 0;

                int q_y[576];
                int q_x[576];
                int head = 0, tail = 0;

                q_y[tail] = y;
                q_x[tail] = x;
                tail++;
                visited[y - y1][x - x1] = 1;

                int comp_size = 0;

                while (head < tail) {
                    int cy = q_y[head];
                    int cx = q_x[head];
                    head++;
                    comp_size++;

                    /* Check if this tile is an edge tile: has edge_feat and adjacent to ELEV_GROUND */
                    if (cave_feat[cy][cx] == edge_feat) {
                        bool adjacent_to_ground = FALSE;
                        for (dy = -1; dy <= 1; dy++) {
                            for (dx = -1; dx <= 1; dx++) {
                                int ny = cy + dy, nx = cx + dx;
                                if (in_bounds(ny, nx) && get_elevation(ny, nx) == ELEV_GROUND) {
                                    adjacent_to_ground = TRUE;
                                    break;
                                }
                            }
                            if (adjacent_to_ground) break;
                        }
                        if (adjacent_to_ground) {
                            edge_y[edge_count] = cy;
                            edge_x[edge_count] = cx;
                            edge_count++;
                        }
                    }

                    /* Expand to neighbors (8-way connected) */
                    for (dy = -1; dy <= 1; dy++) {
                        for (dx = -1; dx <= 1; dx++) {
                            if (dy == 0 && dx == 0) continue;
                            int ny = cy + dy, nx = cx + dx;
                            if (in_bounds(ny, nx) && ny >= y1 && ny <= y2 && nx >= x1 && nx <= x2) {
                                if (get_elevation(ny, nx) == target_elev && !visited[ny - y1][nx - x1]) {
                                    visited[ny - y1][nx - x1] = 1;
                                    q_y[tail] = ny;
                                    q_x[tail] = nx;
                                    tail++;
                                }
                            }
                        }
                    }
                }

                /* Now we have all edge tiles for this component. Pick one to carve access. */
                if (comp_size > 1 && edge_count > 0) {
                    int pick = rand_int(edge_count);
                    int cy = edge_y[pick];
                    int cx = edge_x[pick];

                    /* Unified transit features */
                    int roll = rand_int(100);
                    if (roll < 40) cave_feat[cy][cx] = FEAT_RAMP;
                    else if (roll < 70) cave_feat[cy][cx] = FEAT_LADDER;
                    else cave_feat[cy][cx] = FEAT_STAIRS;

                    /* Clear hazards on ground or plateau landings */
                    for (dy = -1; dy <= 1; dy++) {
                        for (dx = -1; dx <= 1; dx++) {
                            int ny = cy + dy, nx = cx + dx;
                            if (!in_bounds(ny, nx)) continue;

                            if (get_elevation(ny, nx) == ELEV_GROUND || get_elevation(ny, nx) == target_elev) {
                                if (cave_feat[ny][nx] != edge_feat &&
                                    cave_feat[ny][nx] != FEAT_RAMP &&
                                    cave_feat[ny][nx] != FEAT_LADDER &&
                                    cave_feat[ny][nx] != FEAT_STAIRS) {

                                    /* Maintain floor type depending on elevation */
                                    if (get_elevation(ny, nx) == ELEV_LOW)
                                        cave_feat[ny][nx] = FEAT_PIT; /* Pit landing */
                                    else if (get_elevation(ny, nx) == ELEV_HIGH)
                                        cave_feat[ny][nx] = FEAT_HILL_TOP; /* Hill landing */
                                    else
                                        cave_feat[ny][nx] = FEAT_FLOOR; /* Ground landing */
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}"""
    content = content.replace(old_access_code, new_access_code)

    # 2. replace the calls to ensure_elevation_access
    old_call = """    clean_isolated_features(y1, x1, y2, x2);
    ensure_elevation_access(y1, x1, y2, x2, ELEV_HIGH, FEAT_CLIFF_UP);
    ensure_elevation_access(y1, x1, y2, x2, ELEV_LOW, FEAT_CLIFF_DOWN);"""
    new_call = """    clean_isolated_features(y1, x1, y2, x2);
    ensure_elevation_access(y1, x1, y2, x2);"""
    content = content.replace(old_call, new_call)

    with open('src/generate.c', 'w') as f:
        f.write(content)

update_file()
