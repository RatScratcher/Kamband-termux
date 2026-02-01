/* File: pursuit.c */

#include "angband.h"
#include "pursuit.h"

static int pursuit_r_idx = 0;
static int pursuit_hp = 0;
static int pursuit_maxhp = 0;

#define MAX_AMBUSH 20
static int ambush_count = 0;
static int ambush_r_idx[MAX_AMBUSH];
static int ambush_hp[MAX_AMBUSH];
static int ambush_maxhp[MAX_AMBUSH];

static s32b level_start_turn = 0;

static cptr dread_messages[20] = {
    "The rhythm of your own heartbeat starts to sound like drums in the distance.",
    "You suddenly realize the dungeon has fallen completely silent. Even the stones seem to be listening.",
    "A faint, wet slurping sound echoes from the corridor behind you.",
    "You feel a cold breath on the back of your neck, but when you turn, the hall is empty.",
    "You catch a glimpse of something moving just at the edge of your lantern's light.",
    "The shadows seem to stretch and grasp at your feet.",
    "An inexplicable feeling of despair washes over you.",
    "You hear the faint scratching of claws on stone.",
    "The air grows heavy, making it difficult to breathe.",
    "You feel like you are being watched by a thousand unseen eyes.",
    "A sudden chill permeates the air, despite the warmth of your light.",
    "You hear a soft, mocking laughter from the darkness.",
    "The walls seem to pulse with a faint, malevolent energy.",
    "You feel a sudden urge to flee, though you know not from what.",
    "The darkness ahead seems thicker than it should be.",
    "You hear the sound of footsteps matching your own, but stopping when you stop.",
    "A foul stench of decay briefly fills the air.",
    "You feel a phantom touch graze your arm.",
    "The silence is broken by a distant, mournful wail.",
    "You feel a sudden drop in temperature."
};

static cptr decay_messages[4] = {
    "Did that wall just wink at you, or was it a trick of the light?",
    "The floor beneath your feet feels disturbingly soft, like stepping on a tongue.",
    "You hear a scream echoing through the ruins. It sounds remarkably like your own voice.",
    "The glowing symbols on the wall are weeping a dark, viscous fluid."
};

void prepare_staircase_pursuit(void)
{
    int d, y, x, m_idx;
    monster_type *m_ptr;
    monster_race *r_ptr;

    pursuit_r_idx = 0;

    for (d = 0; d < 8; d++)
    {
        y = p_ptr->py + ddy_ddd[d];
        x = p_ptr->px + ddx_ddd[d];

        if (!in_bounds(y, x)) continue;

        m_idx = cave_m_idx[y][x];
        if (m_idx > 0)
        {
            m_ptr = &m_list[m_idx];
            r_ptr = &r_info[m_ptr->r_idx];

            if ((r_ptr->flags2 & RF2_SMART) || (r_ptr->flags1 & RF1_FRIENDS))
            {
                pursuit_r_idx = m_ptr->r_idx;
                pursuit_hp = m_ptr->hp;
                pursuit_maxhp = m_ptr->maxhp;
                /* Only one monster can follow */
                break;
            }
        }
    }
}

void execute_staircase_pursuit(void)
{
    if (pursuit_r_idx > 0)
    {
        int y, x, d;
        for (d = 1; d < 10; d++)
        {
            scatter(&y, &x, p_ptr->py, p_ptr->px, d, 0);
            if (cave_empty_bold(y, x))
            {
                if (place_monster_aux(y, x, pursuit_r_idx, 0))
                {
                    int m_idx = cave_m_idx[y][x];
                    if (m_idx > 0)
                    {
                        m_list[m_idx].hp = pursuit_hp;
                        m_list[m_idx].maxhp = pursuit_maxhp;
                        msg_print("You feel you are being pursued!");
                    }
                }
                break;
            }
        }
        pursuit_r_idx = 0;
    }
}

void prepare_recall_ambush(void)
{
    int d, y, x, m_idx;

    ambush_count = 0;

    for (d = 0; d < 8; d++)
    {
        y = p_ptr->py + ddy_ddd[d];
        x = p_ptr->px + ddx_ddd[d];

        if (!in_bounds(y, x)) continue;

        m_idx = cave_m_idx[y][x];
        if (m_idx > 0)
        {
            monster_type *m_ptr = &m_list[m_idx];
            if (ambush_count < MAX_AMBUSH)
            {
                ambush_r_idx[ambush_count] = m_ptr->r_idx;
                ambush_hp[ambush_count] = m_ptr->hp;
                ambush_maxhp[ambush_count] = m_ptr->maxhp;
                ambush_count++;
            }
        }
    }
}

void execute_recall_ambush(void)
{
    int i;
    if (ambush_count > 0 && !p_ptr->depth)
    {
        /* Randomize location in town */
        teleport_player(200);
        msg_print("You are ambushed!");

        for (i = 0; i < ambush_count; i++)
        {
            int y, x, d;
            for (d = 1; d < 10; d++)
            {
                scatter(&y, &x, p_ptr->py, p_ptr->px, d, 0);
                if (cave_empty_bold(y, x))
                {
                    if (place_monster_aux(y, x, ambush_r_idx[i], 0))
                    {
                        int m_idx = cave_m_idx[y][x];
                        if (m_idx > 0)
                        {
                            m_list[m_idx].hp = ambush_hp[i];
                            m_list[m_idx].maxhp = ambush_maxhp[i];
                        }
                    }
                    break;
                }
            }
        }
        ambush_count = 0;
    }
    /* Clear ambush if not in town (shouldn't happen with recall logic but safe) */
    if (p_ptr->depth != 0) ambush_count = 0;
}

void reset_dread(void)
{
    level_start_turn = turn;
}

void process_dread(void)
{
    int chance = 1;
    if (turn - level_start_turn > 1000) chance = 5;

    if (rand_int(100) < chance)
    {
        if (p_ptr->depth >= 50 && rand_int(2) == 0)
        {
            msg_print(decay_messages[rand_int(4)]);
        }
        else
        {
            msg_print(dread_messages[rand_int(20)]);
        }
    }
}

void process_breathing_walls(void)
{
    int i;
    if (p_ptr->depth < 50) return;

    /* Visual effect only */
    for (i = 0; i < 10; i++)
    {
        int y = rand_int(DUNGEON_HGT);
        int x = rand_int(DUNGEON_WID);

        if (!in_bounds(y, x)) continue;
        if (!panel_contains(y, x)) continue; /* Only on screen */

        if (cave_feat[y][x] >= FEAT_WALL_EXTRA && cave_feat[y][x] <= FEAT_WALL_SOLID)
        {
            char c = (rand_int(2) == 0) ? '%' : '#';
            print_rel(c, TERM_SLATE, y, x);
        }
    }
}
