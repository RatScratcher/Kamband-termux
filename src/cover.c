/* File: cover.c */

/*
 * Cover system for Kamband
 * Provides tactical cover mechanics for strategic combat
 */

#include "angband.h"

/*
 * Initialize cover system
 */
void init_cover_system(void)
{
    int y, x;
    for (y = 0; y < DUNGEON_HGT; y++) {
        for (x = 0; x < DUNGEON_WID; x++) {
            if (cave_cover[y][x]) {
                KILL(cave_cover[y][x], cover_data);
            }
        }
    }
}

/*
 * Create cover at location
 */
void create_cover_at(int y, int x, int cover_type, int durability, int feat)
{
    if (!in_bounds(y, x)) return;

    if (cave_cover[y][x] != NULL) {
        KILL(cave_cover[y][x], cover_data);
    }

    MAKE(cave_cover[y][x], cover_data);
    cave_cover[y][x]->durability = durability;
    cave_cover[y][x]->max_durability = durability;
    cave_cover[y][x]->cover_type = cover_type;
    cave_cover[y][x]->terrain_feat = feat;

    cave_set_feat(y, x, feat);
}

/*
 * Destroy cover at location
 */
void destroy_cover(int y, int x)
{
    if (!in_bounds(y, x)) return;

    if (cave_cover[y][x] != NULL) {
        KILL(cave_cover[y][x], cover_data);
    }

    msg_print("The cover is destroyed!");
    cave_set_feat(y, x, FEAT_FLOOR);
}

/*
 * Get cover level at location (regardless of direction)
 */
int get_cover_at(int y, int x)
{
    int feat;

    if (!in_bounds(y, x)) return COVER_NONE;

    /* Check for destructible cover first */
    if (cave_cover[y][x] != NULL) {
        if (cave_cover[y][x]->durability > 0) {
            return cave_cover[y][x]->cover_type;
        }
        /* Cover destroyed */
        return COVER_NONE;
    }

    feat = cave_feat[y][x];

    switch (feat) {
        /* Heavy cover - total protection from one side, heavy from others */
        case FEAT_WALL_INNER:
        case FEAT_WALL_OUTER:
        case FEAT_WALL_SOLID:
        case FEAT_PERM_INNER:
        case FEAT_PERM_OUTER:
        case FEAT_PERM_SOLID:
        case FEAT_STONE_PILLAR:
            return COVER_HEAVY;

        /* Medium cover */
        case FEAT_TREES:
        case FEAT_BOULDER:
        case FEAT_RUBBLE:
            return COVER_MEDIUM;

        /* Light cover */
        case FEAT_FALLEN_TREE:
        case FEAT_CRATE:
        case FEAT_TALL_GRASS:
        case FEAT_REEDS:
        case FEAT_SHRUB:
            return COVER_LIGHT;

        /* Special: Fog provides concealment but not solid cover */
        case FEAT_FOG:
        case FEAT_FOG_DENSE:
        case FEAT_SMOKE:
        case FEAT_CHAOS_FOG:
            /* Fog is special - it provides stealth but not damage reduction */
            return COVER_LIGHT;

        /* Explosive cover - light until hit, then becomes none */
        case FEAT_BARREL:
            return COVER_LIGHT;

        default:
            return COVER_NONE;
    }
}

/*
 * Get cover level against attack from specific direction
 * Returns highest cover between target and attacker
 */
int get_cover_vs_direction(int ty, int tx, int ay, int ax)
{
    int dy = ty - ay;
    int dx = tx - ax;
    int dist = distance(ay, ax, ty, tx);
    int best_cover = COVER_NONE;
    int y, x;
    int i;

    /* Check each grid between attacker and target for cover */
    for (i = 1; i < dist; i++) {
        y = ay + (dy * i) / dist;
        x = ax + (dx * i) / dist;

        if (!in_bounds(y, x)) continue;

        int cover = get_cover_at(y, x);

        /* Fog only helps if you're IN it, not behind it */
        if (cave_feat[y][x] == FEAT_FOG ||
            cave_feat[y][x] == FEAT_FOG_DENSE ||
            cave_feat[y][x] == FEAT_SMOKE) {
            /* Fog doesn't block line of sight for cover, just provides concealment */
            continue;
        }

        if (cover > best_cover) {
            best_cover = cover;
        }

        /* Total cover blocks completely */
        if (best_cover >= COVER_TOTAL) {
            return COVER_TOTAL;
        }
    }

    /* Check target's own grid for cover (standing in/behind cover) */
    int target_cover = get_cover_at(ty, tx);

    /* If target is IN cover, they get that benefit */
    if (target_cover > best_cover && target_cover <= COVER_MEDIUM) {
        best_cover = target_cover;
    }

    return best_cover;
}

/*
 * Calculate directional cover - which sides are protected
 * Used for standing behind walls, trees, etc.
 */
static byte get_directional_cover(int y, int x)
{
    byte cover_dirs = 0;
    int dy, dx;
    int dir;

    /* Check all 8 directions for adjacent cover */
    for (dir = 0; dir < 8; dir++) {
        dy = ddy_ddd[dir];
        dx = ddx_ddd[dir];

        int ny = y + dy;
        int nx = x + dx;

        if (!in_bounds(ny, nx)) continue;

        int feat = cave_feat[ny][nx];

        /* Solid features provide cover from that direction */
        if (feat >= FEAT_WALL_EXTRA || feat == FEAT_TREES ||
            feat == FEAT_BOULDER || feat == FEAT_STONE_PILLAR) {
            cover_dirs |= (1 << dir);
        }
    }

    return cover_dirs;
}

/*
 * Check if attack from direction is blocked by directional cover
 */
static bool is_attack_blocked_by_cover(int ty, int tx, int ay, int ax)
{
    byte dir_cover = get_directional_cover(ty, tx);

    /* Determine direction from target to attacker */
    int dy = ay - ty;
    int dx = ax - tx;

    /* Convert to 8-direction */
    int dir;
    if (dy < 0 && dx < 0) dir = 7; /* NW */
    else if (dy < 0 && dx == 0) dir = 0; /* N */
    else if (dy < 0 && dx > 0) dir = 1; /* NE */
    else if (dy == 0 && dx > 0) dir = 2; /* E */
    else if (dy > 0 && dx > 0) dir = 3; /* SE */
    else if (dy > 0 && dx == 0) dir = 4; /* S */
    else if (dy > 0 && dx < 0) dir = 5; /* SW */
    else if (dy == 0 && dx < 0) dir = 6; /* W */
    else return FALSE; /* Same grid */

    /* Check if there's cover from that direction */
    if (dir_cover & (1 << dir)) {
        /* Have cover from that direction - check if it's total */
        int cover = get_cover_at(ty + ddy_ddd[dir], tx + ddx_ddd[dir]);
        if (cover >= COVER_HEAVY) {
            /* Heavy cover from that direction = blocked */
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * Damage destructible cover
 */
void damage_cover(int y, int x, int damage)
{
    if (!in_bounds(y, x)) return;

    int feat = cave_feat[y][x];

    /* Special: Barrels explode! */
    if (feat == FEAT_BARREL) {
        msg_print("The barrel explodes!");
        /* Fireball effect */
        fire_ball(GF_FIRE, 0, 30, 2); /* Direction 0 is self? fire_ball(typ, dir, dam, rad) */
        /* To explode at (y,x), we need fire_explosion or project */
        fire_explosion(y, x, GF_FIRE, 2, 30);
        cave_set_feat(y, x, FEAT_FLOOR);
        return;
    }

    /* Check for destructible cover data */
    if (cave_cover[y][x] != NULL) {
        cave_cover[y][x]->durability -= damage;

        if (cave_cover[y][x]->durability <= 0) {
            destroy_cover(y, x);
        } else if (cave_cover[y][x]->durability < cave_cover[y][x]->max_durability / 4) {
            /* Cover nearly destroyed */
            /* Only print if player can see */
            if (player_has_los_bold(y, x))
                msg_print("The cover is nearly destroyed!");
        }
        return;
    }

    /* Trees can be destroyed by enough damage */
    if (feat == FEAT_TREES && damage > 20) {
        if (rand_int(100) < damage) {
            if (player_has_los_bold(y, x))
                msg_print("The tree crashes down!");
            cave_set_feat(y, x, FEAT_FALLEN_TREE);
            /* Create fallen tree cover */
            create_cover_at(y, x, COVER_LIGHT, COVER_DURABILITY_TREE / 2, FEAT_FALLEN_TREE);
        }
    }

    /* Crates break easily */
    if (feat == FEAT_CRATE) {
        MAKE(cave_cover[y][x], cover_data);
        cave_cover[y][x]->durability = 20;
        cave_cover[y][x]->max_durability = 20;
        cave_cover[y][x]->cover_type = COVER_LIGHT;
        cave_cover[y][x]->terrain_feat = FEAT_CRATE;

        damage_cover(y, x, damage); /* Apply damage */
    }
}

/*
 * Main cover resolution function
 * Called when attack is made
 * Modifies damage and applies damage to cover
 * Returns TRUE if attack hits target (FALSE if stopped by cover)
 */
bool attack_through_cover(int ay, int ax, int ty, int tx, int *damage, int *cover_damage_out)
{
    int cover = get_cover_vs_direction(ty, tx, ay, ax);
    int cover_damage = 0;
    int original_damage = *damage;
    int i;

    *cover_damage_out = 0;

    /* No cover = full damage */
    if (cover == COVER_NONE) return TRUE;

    /* Check directional cover (standing behind something) */
    if (is_attack_blocked_by_cover(ty, tx, ay, ax)) {
        /* msg_print("The attack is blocked by cover!"); */
        *damage = 0;
        return FALSE;
    }

    /* Calculate damage absorbed by cover */
    int absorb_percent = 0;
    int miss_chance = 0;

    switch (cover) {
        case COVER_LIGHT:
            absorb_percent = COVER_ABSORB_LIGHT;
            miss_chance = 25;
            break;
        case COVER_MEDIUM:
            absorb_percent = COVER_ABSORB_MEDIUM;
            miss_chance = 40;
            break;
        case COVER_HEAVY:
            absorb_percent = COVER_ABSORB_HEAVY;
            miss_chance = 60;
            break;
        case COVER_TOTAL:
            absorb_percent = COVER_ABSORB_TOTAL;
            miss_chance = 100;
            break;
    }

    /* Miss chance - attack hits cover instead */
    if (rand_int(100) < miss_chance) {
        /* Attack hits the cover instead */
        cover_damage = original_damage;
        *damage = 0;

        /* Find and damage the cover */
        int dy = ty - ay;
        int dx = tx - ax;
        int dist = distance(ay, ax, ty, tx);

        for (i = 1; i <= dist; i++) {
            int cy = ay + (dy * i) / dist;
            int cx = ax + (dx * i) / dist;

            if (get_cover_at(cy, cx) >= cover) {
                damage_cover(cy, cx, cover_damage);
                /* msg_print("Your attack strikes the cover!"); */
                *cover_damage_out = cover_damage;
                return FALSE;
            }
        }

        /* Fallback if cover grid not found but cover detected (e.g. target is in cover) */
        if (get_cover_at(ty, tx) >= cover) {
             damage_cover(ty, tx, cover_damage);
             *cover_damage_out = cover_damage;
             return FALSE;
        }

        return FALSE;
    }

    /* Partial cover - damage reduced */
    cover_damage = (original_damage * absorb_percent) / 100;
    *damage = original_damage - cover_damage;

    /* Damage the cover that absorbed the hit */
    if (cover_damage > 0) {
        int dy = ty - ay;
        int dx = tx - ax;
        int dist = distance(ay, ax, ty, tx);

        for (i = 1; i <= dist; i++) {
            int cy = ay + (dy * i) / dist;
            int cx = ax + (dx * i) / dist;

            if (get_cover_at(cy, cx) >= cover) {
                damage_cover(cy, cx, cover_damage);
                break;
            }
        }
        /* Fallback */
        if (i > dist && get_cover_at(ty, tx) >= cover) {
             damage_cover(ty, tx, cover_damage);
        }
    }

    *cover_damage_out = cover_damage;
    return (*damage > 0);
}

/*
 * Get color for cover display
 */
byte get_cover_color(int y, int x, byte base_color)
{
    int cover = get_cover_at(y, x);

    switch (cover) {
        case COVER_HEAVY: return TERM_L_UMBER;  /* Brown for heavy */
        case COVER_MEDIUM: return TERM_UMBER;    /* Dark brown for medium */
        case COVER_LIGHT: return TERM_GREEN;     /* Green for light */
        default: return base_color;
    }
}
