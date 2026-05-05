1. **Modify `alloc_stairs` function in `src/generate.c`**
   - The current `alloc_stairs` takes `num` (the number of stairs) and randomly allocates them. Since we now want exactly one stair per sector (a total of 420 for UP and 420 for DOWN), we should modify the placement logic so that it iterates over all `row_rooms / 2` and `col_rooms / 2` (the 2x2 block sectors).
   - For each sector (defined by `y` from `s_y * 2` to `s_y * 2 + 1` and `x` from `s_x * 2` to `s_x * 2 + 1` where a block is `BLOCK_HGT`x`BLOCK_WID`), pick a random naked floor tile within those bounds.
   - If a naked floor tile is found, place the requested stair (taking into account town/quest logic) and apply the `CAVE_GLOW` flag.
   - If no valid tile is found after several attempts in that sector, just skip that sector (as it might be completely filled with walls/hazards).

2. **Update the calls to `alloc_stairs` in `cave_gen` (`src/generate.c`)**
   - Change the calls to `alloc_stairs(FEAT_MORE, rand_range(135, 140), 3, ...)` to `alloc_stairs(FEAT_MORE, 420, 0, ...)`. Although since `alloc_stairs` will be rewritten to use a per-sector approach, the `num` parameter might be repurposed or ignored. We'll update the function signature or logic to simply place one per sector instead of taking `num` and picking randomly globally.

3. **Pre-commit and Test**
   - Run the pre-commit instructions.
   - Compile the game and ensure there are no compilation errors.
   - Run the game and verify the map generation works correctly and the stairs glow.

4. **Submit the changes**
