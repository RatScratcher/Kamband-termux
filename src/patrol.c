/*
 * Patrol and guard system for Kamband
 * Monsters patrol routes, guard posts, and respond to alerts
 */

#include "angband.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Scaled sine/cosine for 0-255 degrees */
static int sind(int angle) {
    return (int)(sin((double)angle * M_PI / 128.0) * 256.0);
}

static int cosd(int angle) {
    return (int)(cos((double)angle * M_PI / 128.0) * 256.0);
}

/* Helper functions */
static int get_stealth_bonus_from_cover(int y, int x) {
    int cover = get_cover_at(y, x);
    switch (cover) {
        case COVER_LIGHT: return COVER_STEALTH_LIGHT;
        case COVER_MEDIUM: return COVER_STEALTH_MEDIUM;
        case COVER_HEAVY: return COVER_STEALTH_HEAVY;
        case COVER_TOTAL: return COVER_STEALTH_HEAVY;
        default: return 0;
    }
}

static void monster_find_cover(int m_idx, int *cy, int *cx) {
    monster_type *m_ptr = &m_list[m_idx];
    int py = p_ptr->py;
    int px = p_ptr->px;
    int dy, dx;
    int best_cover = -1;
    int best_dist = 999;

    *cy = m_ptr->fy;
    *cx = m_ptr->fx;

    /* Check nearby grids for cover from player */
    for (dy = -2; dy <= 2; dy++) {
        for (dx = -2; dx <= 2; dx++) {
            int ny = m_ptr->fy + dy;
            int nx = m_ptr->fx + dx;

            if (!in_bounds(ny, nx)) continue;
            if (!cave_floor_bold(ny, nx)) continue;
            if (cave_m_idx[ny][nx] != 0 && cave_m_idx[ny][nx] != m_idx) continue;

            int cover = get_cover_vs_direction(ny, nx, py, px);
            int d = distance(ny, nx, py, px);

            if (cover > best_cover) {
                best_cover = cover;
                best_dist = d;
                *cy = ny;
                *cx = nx;
            } else if (cover == best_cover) {
                /* Prefer closer cover? Or farther? Probably safer to be somewhat distant but close enough to act. */
                /* Let's prefer closer to current position */
                int d_curr = distance(ny, nx, m_ptr->fy, m_ptr->fx);
                int d_best_curr = distance(*cy, *cx, m_ptr->fy, m_ptr->fx);
                if (d_curr < d_best_curr) {
                    *cy = ny;
                    *cx = nx;
                }
            }
        }
    }
}

/*
 * Initialize patrol system
 */
void init_patrol_system(void)
{
    int i;
    for (i = 0; i < MAX_M_IDX; i++) {
        m_guard[i] = NULL;
    }
}

/*
 * Allocate guard data for a monster
 */
monster_guard_data *alloc_guard_data(int m_idx)
{
    if (m_guard[m_idx] == NULL) {
        m_guard[m_idx] = ZNEW(monster_guard_data);
        /* Initialize defaults */
        m_guard[m_idx]->guard_state = GUARD_STATE_PATROL;
        m_guard[m_idx]->patrol_type = PATROL_TYPE_RANDOM;
        m_guard[m_idx]->num_waypoints = 0;
        m_guard[m_idx]->current_waypoint = 0;
        m_guard[m_idx]->chase_timer = 0;
    }
    return m_guard[m_idx];
}

/*
 * Free guard data
 */
void free_guard_data(int m_idx)
{
    if (m_guard[m_idx] != NULL) {
        KILL(m_guard[m_idx], monster_guard_data);
    }
}

/*
 * Setup a patrol route for a monster
 */
void setup_monster_patrol(int m_idx, int type)
{
    monster_type *m_ptr = &m_list[m_idx];
    monster_guard_data *guard = alloc_guard_data(m_idx);
    int i;

    guard->patrol_type = type;
    guard->home_y = m_ptr->fy;
    guard->home_x = m_ptr->fx;
    guard->guard_state = GUARD_STATE_PATROL;

    switch (type) {
        case PATROL_TYPE_CIRCUIT:
        case PATROL_TYPE_BACKFORTH:
            /* Create waypoints in a circuit around starting point */
            guard->num_waypoints = 4 + rand_int(4); /* 4-7 waypoints */

            for (i = 0; i < guard->num_waypoints; i++) {
                int angle = (i * 256) / guard->num_waypoints;
                int dist = 3 + rand_int(PATROL_RADIUS - 2);

                guard->waypoints[i].y = guard->home_y + (dist * sind(angle)) / 256;
                guard->waypoints[i].x = guard->home_x + (dist * cosd(angle)) / 256;
                guard->waypoints[i].wait_turns = 5 + rand_int(PATROL_REST_TURNS);

                /* Ensure valid location */
                if (!in_bounds(guard->waypoints[i].y, guard->waypoints[i].x) ||
                    !cave_floor_bold(guard->waypoints[i].y, guard->waypoints[i].x)) {
                    /* Fallback to home */
                    guard->waypoints[i].y = guard->home_y;
                    guard->waypoints[i].x = guard->home_x;
                }
            }
            break;

        case PATROL_TYPE_RANDOM:
            /* Just wander randomly within radius */
            guard->num_waypoints = 0;
            break;

        case PATROL_TYPE_STATIONARY:
            /* Guard post - don't move */
            guard->num_waypoints = 1;
            guard->waypoints[0].y = guard->home_y;
            guard->waypoints[0].x = guard->home_x;
            guard->waypoints[0].wait_turns = 0;
            guard->guard_state = GUARD_STATE_GUARD;
            break;
    }
}

/*
 * Setup a guard post
 */
void setup_guard_post(int m_idx, int post_type, int y, int x)
{
    monster_type *m_ptr = &m_list[m_idx];
    monster_guard_data *guard = alloc_guard_data(m_idx);
    int dy, dx;

    guard->guard_post_type = post_type;
    guard->home_y = y;
    guard->home_x = x;
    guard->guard_state = GUARD_STATE_GUARD;

    switch (post_type) {
        case GUARD_POST_DOOR:
            /* Position at door, facing outward */
            if (m_ptr->fy != y || m_ptr->fx != x) {
                monster_swap(m_ptr->fy, m_ptr->fx, y, x);
            }
            /* Look for open space to face */
            for (dy = -1; dy <= 1; dy++) {
                for (dx = -1; dx <= 1; dx++) {
                    if (dy == 0 && dx == 0) continue;
                    if (cave_floor_bold(y + dy, x + dx)) {
                        /* Face this direction (store in monster data if possible) */
                        break;
                    }
                }
            }
            break;

        case GUARD_POST_HIGHGROUND:
            /* Find nearby high ground/cover */
            {
                int best_y = y, best_x = x;
                int best_score = get_elevation(y, x) + get_cover_at(y, x);

                for (dy = -3; dy <= 3; dy++) {
                    for (dx = -3; dx <= 3; dx++) {
                        int sy = y + dy;
                        int sx = x + dx;
                        if (!in_bounds(sy, sx)) continue;

                        int score = get_elevation(sy, sx) + get_cover_at(sy, sx);
                        if (score > best_score && cave_floor_bold(sy, sx)) {
                            best_score = score;
                            best_y = sy;
                            best_x = sx;
                        }
                    }
                }

                /* Move monster safely */
                if (best_y != y || best_x != x) {
                    monster_swap(y, x, best_y, best_x);
                }
            }
            break;

        case GUARD_POST_TREASURE:
            /* Position near valuable item */
            m_ptr->fy = y;
            m_ptr->fx = x;
            guard->waypoints[0].y = y;
            guard->waypoints[0].x = x;
            guard->waypoints[0].wait_turns = 50; /* Long rest at treasure */
            guard->num_waypoints = 1;
            guard->patrol_type = PATROL_TYPE_STATIONARY;
            break;

        default:
            m_ptr->fy = y;
            m_ptr->fx = x;
            break;
    }
}

/*
 * Check if position is good for ambush (cover + line of sight to approach)
 */
static bool is_good_ambush_spot(int y, int x, int home_y, int home_x)
{
    if (!cave_floor_bold(y, x)) return FALSE;

    /* Must have cover */
    if (get_cover_at(y, x) < COVER_LIGHT) return FALSE;

    /* Must be able to see approach to home */
    if (los(y, x, home_y, home_x)) return TRUE;

    return FALSE;
}

/*
 * Find next patrol waypoint
 */
static void advance_patrol_waypoint(monster_guard_data *guard)
{
    if (guard->num_waypoints == 0) return;

    switch (guard->patrol_type) {
        case PATROL_TYPE_CIRCUIT:
            guard->current_waypoint++;
            if (guard->current_waypoint >= guard->num_waypoints) {
                guard->current_waypoint = 0;
            }
            break;

        case PATROL_TYPE_BACKFORTH:
            /* Use direction flag in upper bit of current_waypoint */
            if (guard->current_waypoint & 0x80) {
                /* Going backward */
                int wp = guard->current_waypoint & 0x7F;
                wp--;
                if (wp < 0) {
                    wp = 1;
                    guard->current_waypoint = wp; /* Forward now */
                } else {
                    guard->current_waypoint = wp | 0x80;
                }
            } else {
                /* Going forward */
                guard->current_waypoint++;
                if (guard->current_waypoint >= guard->num_waypoints) {
                    guard->current_waypoint = (guard->num_waypoints - 2) | 0x80;
                }
            }
            break;

        case PATROL_TYPE_RANDOM:
            /* Pick random point within radius */
            guard->waypoints[0].y = guard->home_y + rand_int(PATROL_RADIUS * 2) - PATROL_RADIUS;
            guard->waypoints[0].x = guard->home_x + rand_int(PATROL_RADIUS * 2) - PATROL_RADIUS;
            guard->current_waypoint = 0;
            break;
    }
}

/*
 * Alert nearby guards when combat starts
 */
void alert_nearby_guards(int y, int x, int radius)
{
    int m_idx;

    for (m_idx = 1; m_idx < m_max; m_idx++) {
        monster_type *m_ptr = &m_list[m_idx];
        monster_guard_data *guard = m_guard[m_idx];

        if (!m_ptr->r_idx) continue;
        if (guard == NULL) continue; /* Not a guard */

        /* Check distance */
        if (distance(y, x, m_ptr->fy, m_ptr->fx) > radius) continue;

        /* Same "faction" - smart monsters alert each other */
        monster_race *r_ptr = &r_info[m_ptr->r_idx];
        if (!(r_ptr->flags2 & RF2_SMART) && !(r_ptr->flags1 & RF1_FRIENDS)) continue;

        /* Alert this guard */
        if (guard->guard_state == GUARD_STATE_PATROL ||
            guard->guard_state == GUARD_STATE_GUARD ||
            guard->guard_state == GUARD_STATE_SLEEP) {

            guard->guard_state = GUARD_STATE_ALERT;
            guard->alert_y = y;
            guard->alert_x = x;

            /* Visual feedback for close alerts */
            if (player_has_los_bold(m_ptr->fy, m_ptr->fx)) {
                msg_print("A nearby guard is alerted!");
            }
        }
    }
}

/*
 * Execute patrol movement for a monster
 * Returns TRUE if movement/action was handled, FALSE if standard AI should take over.
 */
bool execute_patrol_behavior(int m_idx)
{
    monster_type *m_ptr = &m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    monster_guard_data *guard = m_guard[m_idx];

    if (guard == NULL) return FALSE; /* Not a patrol/guard monster */

    int ty, tx; /* Target coordinates */
    bool moved = FALSE;

    /* State machine */
    switch (guard->guard_state) {
        case GUARD_STATE_SLEEP:
            /* Sleeping - check for disturbances */
            if (player_has_los_bold(m_ptr->fy, m_ptr->fx) &&
                (p_ptr->skill_stl + get_stealth_bonus_from_cover(p_ptr->py, p_ptr->px)) < r_ptr->aaf) {
                /* Player spotted */
                guard->guard_state = GUARD_STATE_CHASE;
                guard->alert_y = p_ptr->py;
                guard->alert_x = p_ptr->px;
                guard->chase_timer = GUARD_CHASE_TIMEOUT;
                alert_nearby_guards(m_ptr->fy, m_ptr->fx, GUARD_ALERT_RADIUS);
                msg_print("The guard wakes up!");
                return FALSE; /* Switch to chase immediately */
            }
            return TRUE; /* Don't move while sleeping */

        case GUARD_STATE_GUARD:
            /* Stationary guarding - only move if alert */
            if (player_has_los_bold(m_ptr->fy, m_ptr->fx)) {
                guard->guard_state = GUARD_STATE_CHASE;
                guard->alert_y = p_ptr->py;
                guard->alert_x = p_ptr->px;
                guard->chase_timer = GUARD_CHASE_TIMEOUT;
                alert_nearby_guards(m_ptr->fy, m_ptr->fx, GUARD_ALERT_RADIUS);
                return FALSE; /* Switch to chase */
            }
            return TRUE; /* Stay put */

        case GUARD_STATE_ALERT:
            /* Investigating last known position */
            ty = guard->alert_y;
            tx = guard->alert_x;

            if (m_ptr->fy == ty && m_ptr->fx == tx) {
                /* Reached alert spot - didn't find anything */
                guard->guard_state = GUARD_STATE_RETURN;
            } else if (player_has_los_bold(m_ptr->fy, m_ptr->fx)) {
                /* Spotted player while investigating */
                guard->guard_state = GUARD_STATE_CHASE;
                guard->chase_timer = GUARD_CHASE_TIMEOUT;
                return FALSE; /* Chase */
            } else {
                /* Move toward alert spot */

                /* Simple move toward */
                int dy = (ty > m_ptr->fy) ? 1 : ((ty < m_ptr->fy) ? -1 : 0);
                int dx = (tx > m_ptr->fx) ? 1 : ((tx < m_ptr->fx) ? -1 : 0);

                if (cave_floor_bold(m_ptr->fy + dy, m_ptr->fx + dx)) {
                    monster_swap(m_ptr->fy, m_ptr->fx, m_ptr->fy + dy, m_ptr->fx + dx);
                    moved = TRUE;
                }
            }
            break;

        case GUARD_STATE_CHASE:
            /* Actively pursuing target */
            guard->chase_timer--;

            if (player_has_los_bold(m_ptr->fy, m_ptr->fx)) {
                /* Can see player - update target */
                guard->alert_y = p_ptr->py;
                guard->alert_x = p_ptr->px;
                guard->chase_timer = GUARD_CHASE_TIMEOUT;

                /* Use smart monster AI with cover awareness */
                if (r_ptr->flags2 & RF2_SMART) {
                    /* Smart monsters use cover while approaching */
                    if (get_cover_at(m_ptr->fy, m_ptr->fx) < COVER_MEDIUM &&
                        get_cover_at(p_ptr->py, p_ptr->px) >= COVER_MEDIUM) {
                        /* Player has better cover - seek cover or flanking position */
                        int cover_y, cover_x;
                        monster_find_cover(m_idx, &cover_y, &cover_x);
                        if (cover_y != m_ptr->fy || cover_x != m_ptr->fx) {
                            /* Move toward cover - Set target for standard AI to follow if possible?
                               Actually, standard AI chases player. If we want them to seek cover,
                               we must handle it or trick get_moves.
                               For now, let's just update alert pos and let standard AI chase player.
                               To implement cover seeking properly would require overriding get_moves logic.
                            */
                        }
                    }
                }

                /* Standard monster movement toward target handled by caller if we return FALSE */
                return FALSE;

            } else {
                /* Lost sight - move to last known position */
                /* Standard AI usually chases player last position too?
                   But if player teleported away, m_ptr->ty might be wrong.
                   Standard AI in process_monster calls find_target_nearest.
                   If not visible, find_target_nearest targets player if known?
                   Or if not, it targets alert_y/x?
                   process_monster handles simple chasing.
                   Let's return FALSE to use standard AI, but update monster memory?
                */
                ty = guard->alert_y;
                tx = guard->alert_x;

                if (m_ptr->fy == ty && m_ptr->fx == tx) {
                    /* At last known position, player gone */
                    if (guard->chase_timer <= 0) {
                        guard->guard_state = GUARD_STATE_RETURN;
                        return TRUE; /* Start returning next turn */
                    }
                }
                return FALSE; /* Continue chasing last known spot */
            }

            /* Timeout - give up chase */
            if (guard->chase_timer <= 0 && !player_has_los_bold(m_ptr->fy, m_ptr->fx)) {
                guard->guard_state = GUARD_STATE_RETURN;
            }
            return FALSE;

        case GUARD_STATE_RETURN:
            /* Return to post/patrol route */
            if (guard->patrol_type == PATROL_TYPE_STATIONARY) {
                ty = guard->home_y;
                tx = guard->home_x;
            } else {
                /* Return to current waypoint */
                if (guard->num_waypoints > 0) {
                    ty = guard->waypoints[guard->current_waypoint].y;
                    tx = guard->waypoints[guard->current_waypoint].x;
                } else {
                    ty = guard->home_y;
                    tx = guard->home_x;
                }
            }

            if (m_ptr->fy == ty && m_ptr->fx == tx) {
                /* Back at post */
                if (guard->patrol_type == PATROL_TYPE_STATIONARY) {
                    guard->guard_state = GUARD_STATE_GUARD;
                } else {
                    guard->guard_state = GUARD_STATE_PATROL;
                }
            } else {
                /* Move toward post */
                int dy = (ty > m_ptr->fy) ? 1 : ((ty < m_ptr->fy) ? -1 : 0);
                int dx = (tx > m_ptr->fx) ? 1 : ((tx < m_ptr->fx) ? -1 : 0);

                if (cave_floor_bold(m_ptr->fy + dy, m_ptr->fx + dx)) {
                    monster_swap(m_ptr->fy, m_ptr->fx, m_ptr->fy + dy, m_ptr->fx + dx);
                    moved = TRUE;
                }
            }
            break;

        case GUARD_STATE_PATROL:
            /* Normal patrol behavior */
            if (player_has_los_bold(m_ptr->fy, m_ptr->fx)) {
                /* Spotted player! */
                guard->guard_state = GUARD_STATE_CHASE;
                guard->alert_y = p_ptr->py;
                guard->alert_x = p_ptr->px;
                guard->chase_timer = GUARD_CHASE_TIMEOUT;
                alert_nearby_guards(m_ptr->fy, m_ptr->fx, GUARD_ALERT_RADIUS);
                return FALSE; /* Chase */
            }

            if (guard->num_waypoints == 0) {
                /* Random patrol */
                if (rand_int(100) < 30) {
                    int dy = rand_int(3) - 1;
                    int dx = rand_int(3) - 1;
                    if (cave_floor_bold(m_ptr->fy + dy, m_ptr->fx + dx)) {
                        monster_swap(m_ptr->fy, m_ptr->fx, m_ptr->fy + dy, m_ptr->fx + dx);
                        moved = TRUE;
                    }
                }
            } else {
                /* Waypoint patrol */
                patrol_waypoint *wp = &guard->waypoints[guard->current_waypoint];

                if (m_ptr->fy == wp->y && m_ptr->fx == wp->x) {
                    /* At waypoint - rest */
                    wp->wait_turns--;
                    if (wp->wait_turns <= 0) {
                        /* Move to next */
                        wp->wait_turns = 5 + rand_int(PATROL_REST_TURNS);
                        advance_patrol_waypoint(guard);
                    }
                } else {
                    /* Move toward waypoint */
                    int dy = (wp->y > m_ptr->fy) ? 1 : ((wp->y < m_ptr->fy) ? -1 : 0);
                    int dx = (wp->x > m_ptr->fx) ? 1 : ((wp->x < m_ptr->fx) ? -1 : 0);

                    if (cave_floor_bold(m_ptr->fy + dy, m_ptr->fx + dx)) {
                        monster_swap(m_ptr->fy, m_ptr->fx, m_ptr->fy + dy, m_ptr->fx + dx);
                        moved = TRUE;
                    }
                }
            }
            break;
    }

    return TRUE;
}

/*
 * Check if monster is currently guarding
 */
bool monster_is_guarding(int m_idx)
{
    monster_guard_data *guard = m_guard[m_idx];
    if (guard == NULL) return FALSE;

    return (guard->guard_state == GUARD_STATE_GUARD ||
            guard->guard_state == GUARD_STATE_SLEEP);
}

/*
 * Monster spotted target - transition to chase
 */
void monster_spotted_target(int m_idx, int ty, int tx)
{
    monster_guard_data *guard = m_guard[m_idx];
    if (guard == NULL) return;

    guard->guard_state = GUARD_STATE_CHASE;
    guard->alert_y = ty;
    guard->alert_x = tx;
    guard->chase_timer = GUARD_CHASE_TIMEOUT;

    alert_nearby_guards(m_list[m_idx].fy, m_list[m_idx].fx, GUARD_ALERT_RADIUS);
}

/*
 * Setup patrol for a group of monsters (squad patrol)
 */
void setup_squad_patrol(int *m_idx_list, int num_monsters, int center_y, int center_x)
{
    int i;

    /* Create a shared patrol route */
    patrol_waypoint shared_waypoints[PATROL_MAX_WAYPOINTS];
    int num_waypoints = 4 + rand_int(4);

    for (i = 0; i < num_waypoints; i++) {
        int angle = (i * 256) / num_waypoints;
        int dist = 4 + rand_int(6);

        shared_waypoints[i].y = center_y + (dist * sind(angle)) / 256;
        shared_waypoints[i].x = center_x + (dist * cosd(angle)) / 256;
        shared_waypoints[i].wait_turns = 10 + rand_int(20);

        if (!in_bounds(shared_waypoints[i].y, shared_waypoints[i].x) ||
            !cave_floor_bold(shared_waypoints[i].y, shared_waypoints[i].x)) {
            shared_waypoints[i].y = center_y;
            shared_waypoints[i].x = center_x;
        }
    }

    /* Assign to each squad member with offset positions */
    for (i = 0; i < num_monsters; i++) {
        monster_guard_data *guard = alloc_guard_data(m_idx_list[i]);

        guard->patrol_type = PATROL_TYPE_CIRCUIT;
        guard->num_waypoints = num_waypoints;
        guard->home_y = center_y;
        guard->home_x = center_x;

        /* Copy waypoints */
        int j;
        for (j = 0; j < num_waypoints; j++) {
            guard->waypoints[j] = shared_waypoints[j];
        }

        /* Offset starting position */
        if (num_monsters > 0)
            guard->current_waypoint = (i * num_waypoints) / num_monsters;
        else
            guard->current_waypoint = 0;

        /* Set initial waypoint near start position*/
        {
            int wy = shared_waypoints[guard->current_waypoint].y + rand_int(3) - 1;
            int wx = shared_waypoints[guard->current_waypoint].x + rand_int(3) - 1;

            if (!in_bounds(wy, wx)) {
                 wy = center_y;
                 wx = center_x;
            }

            if (cave_floor_bold(wy, wx)) {
                monster_swap(m_list[m_idx_list[i]].fy, m_list[m_idx_list[i]].fx, wy, wx);
            }
        }
    }
}
