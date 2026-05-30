import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    # Clean unused variables based on compiler warnings
    # generate.c:4893:15: warning: unused variable 'i' [-Wunused-variable]
    content = content.replace("    int y, x, i;\n\n    /* 1. Identify Edges for Cliffs */", "    int y, x;\n\n    /* 1. Identify Edges for Cliffs */")

    # generate.c:6049:37: warning: unused variable 'by' [-Wunused-variable]
    content = content.replace("				int by = y / BLOCK_HGT;\n\n				/* Only apply to SECTOR_CAVERN to maintain other sector structures */", "				/* Only apply to SECTOR_CAVERN to maintain other sector structures */")

    with open('src/generate.c', 'w') as f:
        f.write(content)

update_file()
