import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    old_scatter = r"int density = \(DUNGEON_HGT \* DUNGEON_WID\) / 50;"
    new_scatter = r"int density = (DUNGEON_HGT * DUNGEON_WID) / 200;"
    content = re.sub(old_scatter, new_scatter, content)

    with open('src/generate.c', 'w') as f:
        f.write(content)

update_file()
