1. **Increase bounds in `src/generate.c`:**
    - Change `TUNN_MAX` from 9000 to 30000.
    - Change `WALL_MAX` from 2000 to 8000.

2. **Add safety checks in `build_tunnel_winding` (`src/generate.c`):**
    - Inside the loop where `dun->tunn_n` is checked, add `if (dun->tunn_n >= TUNN_MAX - 1) break;`.
    - Inside the loop where `dun->wall_n` is checked, add `if (dun->wall_n >= WALL_MAX - 1) break;`.

3. **Modify `dun_body` allocation in `cave_gen` (`src/generate.c`):**
    - Replace the local variable `dun_data dun_body;` with dynamic allocation: `dun_data *dun_body = malloc(sizeof(dun_data));`.
    - Update `dun = &dun_body;` to `dun = dun_body;`.
    - Ensure `free(dun_body);` is called at the end of the `cave_gen` function. Also ensure that if there are any early `return` statements in `cave_gen`, the memory is freed there too (though glancing at the code, it seems there are none, but I will double-check).

4. **Complete pre commit steps**
   - Ensure proper testing, verification, review, and reflection are done.

5. **Submit the change**
    - Commit and submit the code once testing is complete.
