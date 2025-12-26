#include "angband.h"

/*
 * Implementation of Sanctums and Puzzles.
 */

/*
 * Determine if a grid is a sanctum wall
 */
bool is_sanctum_wall(int y, int x)
{
    return (cave_feat[y][x] == FEAT_SANCTUM_WALL);
}

/*
 * Initialize the puzzle state for a new level
 */
void reset_puzzle_state(void)
{
    int i;
    for (i = 0; i < 8; i++) {
        p_ptr->puzzle_solution[i] = 0;
        p_ptr->puzzle_attempt[i] = 0;
    }
    p_ptr->puzzle_next = 0;
}

/*
 * Open the Sanctum Door
 */
static void open_sanctum_door(void)
{
    int y, x;
    bool opened = FALSE;

    /* Find the sanctum door(s) and open them */
    /* Since we don't store the door location, scan the level?
       Or just scan near the player since they must be at the puzzle.
       Scanning the whole level is safe enough for 66x198 or similar sizes. */

    for (y = 0; y < DUNGEON_HGT; y++) {
        for (x = 0; x < DUNGEON_WID; x++) {
            if (cave_feat[y][x] == FEAT_SANCTUM_DOOR || cave_feat[y][x] == FEAT_SANCTUM_WALL) {
                /* Only open if it looks like a door (connected to floor on both sides?)
                   For now, just convert all Sanctum Doors to floor.
                   We rely on the generation to place FEAT_SANCTUM_DOOR specifically. */
                if (cave_feat[y][x] == FEAT_SANCTUM_DOOR) {
                    cave_set_feat(y, x, FEAT_FLOOR);
                    note_spot(y, x);
                    lite_spot(y, x);
                    opened = TRUE;
                }
            }
        }
    }

    if (opened) {
        msg_print("The sanctum seal fades away!");
    }
}

/*
 * Handle Rune Interaction (Echo Lock)
 */
void interaction_rune(int y, int x)
{
    int rune_id = cave_feat[y][x] - FEAT_RUNE_A;
    if (rune_id < 0 || rune_id > 4) return;

    /* Check if correct next rune */
    if (rune_id == p_ptr->puzzle_solution[p_ptr->puzzle_next]) {
        msg_print("The rune glows brightly.");
        p_ptr->puzzle_next++;

        /* Check for completion */
        if (p_ptr->puzzle_solution[p_ptr->puzzle_next] == 255) {
            msg_print("The sequence is complete!");
            open_sanctum_door();
            /* Disable puzzle? */
            reset_puzzle_state();
        }
    } else {
        msg_print("The runes flicker and darken. You feel a psychic backlash!");
        take_sanity_hit(damroll(1, 4), "a puzzle trap");
        p_ptr->puzzle_next = 0;
    }
}

/*
 * Build the Echo Lock Puzzle
 */
static void build_echo_lock(int y, int x)
{
    /*
     * Echo Lock:
     * Place 3-5 runes.
     */
    int num_runes = 3 + rand_int(3); /* 3 to 5 */
    int i;
    int runes[5];

    /* Find spots for runes near the center */
    for (i = 0; i < num_runes; i++) {
        int ry, rx;
        int d = 0;
        while (d < 100) {
            ry = rand_spread(y, 3);
            rx = rand_spread(x, 3);
            if (in_bounds(ry, rx) && cave_clean_bold(ry, rx)) {
                cave_feat[ry][rx] = FEAT_RUNE_A + i;
                runes[i] = i; /* Rune index 0-4 */
                break;
            }
            d++;
        }
    }

    /* Shuffle solution */
    for (i = 0; i < num_runes; i++) {
        int j = rand_int(num_runes);
        int temp = runes[i];
        runes[i] = runes[j];
        runes[j] = temp;
    }

    /* Store solution */
    for (i = 0; i < num_runes; i++) {
        p_ptr->puzzle_solution[i] = runes[i];
    }
    p_ptr->puzzle_solution[num_runes] = 255; /* Terminator */
    p_ptr->puzzle_next = 0;
}

/*
 * Build Flow Conduit
 */
static void build_flow_conduit(int y, int x)
{
    /*
     * Simple version: 2 levers. Both must be pulled to open.
     * We use puzzle_solution to track state.
     * [0] = Left Lever State (0/1)
     * [1] = Right Lever State (0/1)
     * Target: Both 1.
     */

    /* Place Levers */
    int i;
    for (i = 0; i < 2; i++) {
        int ly, lx;
        int d = 0;
        while (d < 100) {
            ly = rand_spread(y, 3);
            lx = rand_spread(x, 3);
            if (in_bounds(ly, lx) && cave_clean_bold(ly, lx)) {
                cave_feat[ly][lx] = (i == 0) ? FEAT_LEVER_LEFT : FEAT_LEVER_RIGHT;
                break;
            }
            d++;
        }
    }

    /* Place Acid Flow (Visual only for now, or hazardous) */
    /* Block the path to the door */
    cave_feat[y][x-1] = FEAT_FLOW_ACID;
    cave_feat[y][x+1] = FEAT_FLOW_ACID;
}

/*
 * Handle Lever Interaction
 */
void interaction_lever(int y, int x)
{
    /* Toggle lever state */
    if (cave_feat[y][x] == FEAT_LEVER_LEFT) {
        p_ptr->puzzle_attempt[0] = !p_ptr->puzzle_attempt[0];
        msg_print("You pull the lever.");
    } else if (cave_feat[y][x] == FEAT_LEVER_RIGHT) {
        p_ptr->puzzle_attempt[1] = !p_ptr->puzzle_attempt[1];
        msg_print("You pull the lever.");
    }

    /* Check solution (Both on) */
    if (p_ptr->puzzle_attempt[0] && p_ptr->puzzle_attempt[1]) {
        msg_print("The acid drains away!");
        open_sanctum_door();

        /* Remove Acid */
        /* Scan nearby */
        int dy, dx;
        for (dy = -5; dy <= 5; dy++) {
            for (dx = -5; dx <= 5; dx++) {
                if (in_bounds(y+dy, x+dx) && cave_feat[y+dy][x+dx] == FEAT_FLOW_ACID) {
                    cave_set_feat(y+dy, x+dx, FEAT_FLOOR);
                }
            }
        }
    }
}


/*
 * Build Mirror Alignment
 */
static void build_mirror_alignment(int y, int x)
{
    /* Place Emitter and Crystal */
    cave_feat[y-2][x] = FEAT_EMITTER;
    cave_feat[y+2][x] = FEAT_CRYSTAL;

    /* Place a Plate that "rotates" mirrors (or just checks alignment) */
    cave_feat[y][x] = FEAT_MIRROR_PLATE;
}

/*
 * Raycast for Mirror Puzzle
 */
static bool check_mirror_beam(int y, int x) {
    int dy = 1; /* Beam starts moving South from Emitter at y-2 */
    int dx = 0;
    int cur_y = y - 1; /* Start one step south of Emitter */
    int cur_x = x;
    int dist = 0;

    while (dist < 20) {
        if (!in_bounds(cur_y, cur_x)) return FALSE;

        int feat = cave_feat[cur_y][cur_x];

        /* Hit Crystal? */
        if (feat == FEAT_CRYSTAL) return TRUE;

        /* Hit Mirror? */
        if (feat == FEAT_MIRROR_PLATE) {
            /* Basic logic: Plate acts as a mirror here for simplicity */
            /* Or maybe we just say the plate activates the beam */
            return TRUE; /* Simplified: Plate activates beam, if clear line to crystal */
        }

        /* Hit Wall? */
        if (!cave_floor_bold(cur_y, cur_x) && feat != FEAT_SANCTUM_WALL) {
             /* Stop */
             return FALSE;
        }

        cur_y += dy;
        cur_x += dx;
        dist++;
    }
    return FALSE;
}

void interaction_plate(int y, int x)
{
    msg_print("You step on the pressure plate. A beam of light shoots forth!");

    /* Check if beam hits crystal */
    /* Emitter is at y-2, Crystal at y+2. */
    /* Beam path: y-1, y, y+1. */
    /* Player is at y,x. */

    if (check_mirror_beam(y, x)) {
        msg_print("The crystal hums with power!");
        open_sanctum_door();
    } else {
        msg_print("The beam fizzles out.");
    }
}


/*
 * Build the Sanctum Vault
 */
void build_sanctum_vault(int y, int x)
{
    int y1 = y - 6;
    int y2 = y + 6;
    int x1 = x - 10;
    int x2 = x + 10;
    int i, j;

    /* Verify bounds */
    if (!in_bounds(y1, x1) || !in_bounds(y2, x2)) return;

    /* Clear area */
    for (i = y1; i <= y2; i++) {
        for (j = x1; j <= x2; j++) {
            cave_feat[i][j] = FEAT_FLOOR;
            cave_info[i][j] |= (CAVE_ROOM | CAVE_GLOW);
        }
    }

    /* Outer Walls (Sanctum Wall) */
    for (i = y1; i <= y2; i++) {
        cave_feat[i][x1] = FEAT_SANCTUM_WALL;
        cave_feat[i][x2] = FEAT_SANCTUM_WALL;
    }
    for (j = x1; j <= x2; j++) {
        cave_feat[y1][j] = FEAT_SANCTUM_WALL;
        cave_feat[y2][j] = FEAT_SANCTUM_WALL;
    }

    /* Inner Reward Room */
    int ry1 = y - 2;
    int ry2 = y + 2;
    int rx1 = x - 3;
    int rx2 = x + 3;

    for (i = ry1; i <= ry2; i++) {
        cave_feat[i][rx1] = FEAT_WALL_INNER;
        cave_feat[i][rx2] = FEAT_WALL_INNER;
    }
    for (j = rx1; j <= rx2; j++) {
        cave_feat[ry1][j] = FEAT_WALL_INNER;
        cave_feat[ry2][j] = FEAT_WALL_INNER;
    }

    /* Divider wall */
    int div_x = x - 4;
    for (i = y1 + 1; i < y2; i++) {
        cave_feat[i][div_x] = FEAT_SANCTUM_WALL;
    }

    /* Door in divider (The Sealed Arch) */
    cave_feat[y][div_x] = FEAT_SANCTUM_DOOR;

    /* Pick a puzzle type */
    int puzzle_type = rand_int(3);
    int p_center_x = x1 + (div_x - x1)/2;

    reset_puzzle_state();

    if (puzzle_type == 0) {
        build_echo_lock(y, p_center_x);
    } else if (puzzle_type == 1) {
        build_flow_conduit(y, p_center_x);
    } else {
        build_mirror_alignment(y, p_center_x);
    }

    /* Reward */
    /* Select reward type */
    int reward_type = rand_int(5);
    int rx = x + 3;

    switch (reward_type) {
        case 0: /* Chamber of Clarity (Stat Potion) */
            /* Place Potion of Augmentation or similar */
            place_object(y, rx, TRUE, TRUE); /* Simplified */
            break;
        case 1: /* Vault of Scrolls */
            place_object(y, rx, TRUE, TRUE);
            place_object(y, rx + 1, TRUE, TRUE);
            break;
        case 2: /* Armory of Echoes */
            place_object(y, rx, TRUE, TRUE); /* Artifacts have chance */
            break;
        case 3: /* Quiet Market */
            /* Summon a shopkeeper or place store entrance? */
            /* For now, just a pile of gold */
            place_object(y, rx, FALSE, FALSE);
            break;
        case 4: /* Threshold of Whispers */
            cave_feat[y][rx] = FEAT_DREAM_PORTAL;
            break;
    }

    /* Hint NPC/Idol */
    cave_feat[y1+1][x1+1] = FEAT_WHISPERING_IDOL;
}

/*
 * Build Folly Vault
 */
void build_folly_vault(int y, int x)
{
    /* Big room */
    int y1 = y - 10;
    int y2 = y + 10;
    int x1 = x - 20;
    int x2 = x + 20;
    int i, j;

    if (!in_bounds(y1, x1) || !in_bounds(y2, x2)) return;

    for (i = y1; i <= y2; i++) {
        for (j = x1; j <= x2; j++) {
            cave_feat[i][j] = FEAT_FLOOR;
            cave_info[i][j] |= (CAVE_ROOM | CAVE_GLOW);
        }
    }

    /* Walls */
    for (i = y1; i <= y2; i++) {
        cave_feat[i][x1] = FEAT_FOLLY_WALL;
        cave_feat[i][x2] = FEAT_FOLLY_WALL;
    }
    for (j = x1; j <= x2; j++) {
        cave_feat[y1][j] = FEAT_FOLLY_WALL;
        cave_feat[y2][j] = FEAT_FOLLY_WALL;
    }

    /* Monsters */
    for (i = 0; i < 20; i++) {
        place_monster(y + rand_range(-5, 5), x + rand_range(-10, 10), MON_ALLOC_PIT | MON_ALLOC_HORDE);
    }

    /* Traps */
    for (i = 0; i < 10; i++) {
        place_trap(y + rand_range(-8, 8), x + rand_range(-15, 15));
    }

    /* Loot */
    for (i = 0; i < 5; i++) {
        place_object(y + rand_range(-5, 5), x + rand_range(-10, 10), TRUE, TRUE);
    }
}

/*
 * Handle Hints
 */
void interaction_idol(int y, int x)
{
    if (p_ptr->au >= 5000) {
        if (get_check("Offer 5000 gold for a hint? ")) {
            int rune_idx = p_ptr->puzzle_solution[p_ptr->puzzle_next];

            p_ptr->au -= 5000;

            if (rune_idx == 255) {
                msg_print("The idol whispers: 'You have already solved the riddle.'");
            } else {
                char hint[80];
                /* 0-4 are runes A-E */
                sprintf(hint, "The idol whispers: 'Seek the Rune of %c...'", 'A' + rune_idx);
                msg_print(hint);
            }
        }
    } else {
        msg_print("The idol remains silent. You feel poor.");
    }
}
