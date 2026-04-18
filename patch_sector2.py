with open('src/generate.c', 'r') as f:
    content = f.read()

target = "    /* 2.5 Secondary Pass: Monsters, Items, Hazards, and Walls */"
new_code = """
    /* 2.4 Ensure Elevation Access */
    ensure_elevation_access(y1, x1, y2, x2, ELEV_HIGH, FEAT_CLIFF_UP);
    ensure_elevation_access(y1, x1, y2, x2, ELEV_LOW, FEAT_CLIFF_DOWN);

    /* 2.5 Secondary Pass: Monsters, Items, Hazards, and Walls */"""

content = content.replace(target, new_code)

with open('src/generate.c', 'w') as f:
    f.write(content)
