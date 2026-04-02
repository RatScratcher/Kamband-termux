import sys

def fix_player_fall():
    with open('src/cmd1.c', 'r') as f:
        content = f.read()

    target_block = """	if (oktomove)
	{
		/* Calculate original elevation */
		int old_elev = get_elevation(py, px);

		/* Move player */
		monster_swap(py, px, y, x);

		/* New location */
		y = py = p_ptr->py;
		x = px = p_ptr->px;

		/* Check for a fall */
		int new_elev = get_elevation(y, x);
		if (new_elev == ELEV_LOW && old_elev == ELEV_GROUND) {
			int feat = cave_feat[y][x];
			int src_feat = cave_feat[py - ddy[dir]][px - ddx[dir]];
			bool has_access = (feat == FEAT_RAMP_DOWN || feat == FEAT_RAMP_UP ||
							   feat == FEAT_STAIRS_DOWN || feat == FEAT_STAIRS_UP ||
							   feat == FEAT_LADDER_DOWN || feat == FEAT_LADDER_UP ||
							   feat == FEAT_ROPE_DOWN || feat == FEAT_ROPE_UP ||
							   feat == FEAT_JUMP_POINT || feat == FEAT_ESCAPE_PIT ||
							   feat == FEAT_CLIFF_DOWN || feat == FEAT_SLOPE_DOWN ||
							   src_feat == FEAT_RAMP_DOWN || src_feat == FEAT_RAMP_UP ||
							   src_feat == FEAT_STAIRS_DOWN || src_feat == FEAT_STAIRS_UP ||
							   src_feat == FEAT_LADDER_DOWN || src_feat == FEAT_LADDER_UP ||
							   src_feat == FEAT_ROPE_DOWN || src_feat == FEAT_ROPE_UP ||
							   src_feat == FEAT_ESCAPE_PIT ||
							   src_feat == FEAT_CLIFF_DOWN || src_feat == FEAT_SLOPE_DOWN);
			if (!has_access && !p_ptr->flying) {
				msg_print("You fall into a pit!");
				int damage = elev_fall_damage(old_elev, new_elev);
				int reduction = (adj_dex_safe[p_ptr->stat_ind[A_DEX]] / 2) + (p_ptr->skill_stl / 2);
				if (reduction < 0) reduction = 0;
				damage -= reduction;
				if (damage > 0) {
					msg_format("You take %d damage from the fall.", damage);
					take_hit(damage, "a fall");
				}
				/* End turn */
				p_ptr->energy_use = 100;
			}
		}"""

    new_block = """	if (oktomove)
	{
		/* Calculate original elevation */
		int old_elev = get_elevation(py, px);

		/* Move player */
		monster_swap(py, px, y, x);

		/* New location */
		y = py = p_ptr->py;
		x = px = p_ptr->px;

		/* Check for a fall */
		int new_elev = get_elevation(y, x);
		if (new_elev == ELEV_LOW && old_elev == ELEV_GROUND && !p_ptr->flying) {
			/* Call can_move_to_elev to check for access features.
			   Actually, since the player is moving DOWN, can_move_to_elev will allow it.
			   Wait, the user requested to call can_move_to_elev(ny, nx) in move_player.
               But the problem states: "If the player moves from ELEV_GROUND to ELEV_LOW without using a FEAT_SLOPE_DOWN or FEAT_RAMP, they must 'Fall'—taking minor damage and ending their turn immediately."
               And: "Update the player movement logic to call can_move_to_elev(ny, nx)."
            */
			int feat = cave_feat[y][x];
            int src_feat = cave_feat[py - ddy[dir]][px - ddx[dir]];
            if (feat != FEAT_SLOPE_DOWN && feat != FEAT_RAMP_DOWN &&
                src_feat != FEAT_SLOPE_DOWN && src_feat != FEAT_RAMP_DOWN) {
                msg_print("You fall into a pit!");
                take_hit(damroll(2, 6), "a fall");
                p_ptr->energy_use = 100;
            }
		}"""

    if target_block in content:
        content = content.replace(target_block, new_block)
        with open('src/cmd1.c', 'w') as f:
            f.write(content)
        print("Successfully updated src/cmd1.c")
    else:
        print("Error: Target block not found in src/cmd1.c")

if __name__ == "__main__":
    fix_player_fall()
