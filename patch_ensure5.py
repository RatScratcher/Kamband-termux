with open('src/generate.c', 'r') as f:
    content = f.read()

target = """    /* 2.4 Ensure Elevation Access */
    ensure_elevation_access(y1, x1, y2, x2, ELEV_HIGH, FEAT_CLIFF_UP);
    ensure_elevation_access(y1, x1, y2, x2, ELEV_LOW, FEAT_CLIFF_DOWN);"""

new_code = """    /* 2.4 Clean up single tiles and Ensure Elevation Access */
    clean_isolated_features(y1, x1, y2, x2);
    ensure_elevation_access(y1, x1, y2, x2, ELEV_HIGH, FEAT_CLIFF_UP);
    ensure_elevation_access(y1, x1, y2, x2, ELEV_LOW, FEAT_CLIFF_DOWN);"""

content = content.replace(target, new_code)

with open('src/generate.c', 'w') as f:
    f.write(content)
