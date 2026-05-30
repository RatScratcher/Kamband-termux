import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    old = """		/* Try adjacent grids */
		for (dy = -1; dy <= 1; dy++)"""

    new = """		/* Try adjacent grids */
		for (dy = -1; dy <= 1; dy++)"""

    # Checking for any other mixed declarations
    # In apply_sector_borders_and_access
    old2 = """        for (x = x1; x <= x2; x++) {
            if (get_elevation(y, x) == elev_check) {
                bool is_edge = FALSE;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {"""

    new2 = """        for (x = x1; x <= x2; x++) {
            if (get_elevation(y, x) == elev_check) {
                bool is_edge = FALSE;
                int dy, dx;
                for (dy = -1; dy <= 1; dy++) {
                    for (dx = -1; dx <= 1; dx++) {"""

    content = content.replace(old2, new2)

    with open('src/generate.c', 'w') as f:
        f.write(content)

update_file()
