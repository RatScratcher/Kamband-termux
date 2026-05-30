import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    # ensure_elevation_access change wasn't completely right, wait no the code actually has:
    #             int edge_feat = (target_elev == ELEV_HIGH) ? FEAT_CLIFF_UP : FEAT_CLIFF_DOWN;
    #
    #             /* New connected component found. We will collect all edge tiles for this component. */
    #             int edge_y[576];
    #             int edge_x[576];
    #             int edge_count = 0;

    old_access2 = """                int edge_feat = (target_elev == ELEV_HIGH) ? FEAT_CLIFF_UP : FEAT_CLIFF_DOWN;

                /* New connected component found. We will collect all edge tiles for this component. */
                int edge_y[576];
                int edge_x[576];
                int edge_count = 0;

                int q_y[576];
                int q_x[576];
                int head = 0, tail = 0;"""

    new_access2 = """                int edge_feat = (target_elev == ELEV_HIGH) ? FEAT_CLIFF_UP : FEAT_CLIFF_DOWN;

                /* New connected component found. We will collect all edge tiles for this component. */
                int edge_y[576];
                int edge_x[576];
                int edge_count = 0;

                int q_y[576];
                int q_x[576];
                int head = 0, tail = 0;
                int comp_size = 0;"""

    content = content.replace(old_access2, new_access2)

    old_comp_size = """                tail++;
                visited[y - y1][x - x1] = 1;

                int comp_size = 0;

                while (head < tail) {"""

    new_comp_size = """                tail++;
                visited[y - y1][x - x1] = 1;

                while (head < tail) {"""

    content = content.replace(old_comp_size, new_comp_size)

    with open('src/generate.c', 'w') as f:
        f.write(content)

update_file()
