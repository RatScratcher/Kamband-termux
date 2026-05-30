import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    # Clean unused h and w
    old = """static void ensure_elevation_access(int y1, int x1, int y2, int x2)
{
    int y, x, dy, dx;
    int h = y2 - y1 + 1;
    int w = x2 - x1 + 1;

    if (h > 24) h = 24;
    if (w > 24) w = 24;

    int visited[24][24];"""

    new = """static void ensure_elevation_access(int y1, int x1, int y2, int x2)
{
    int y, x, dy, dx;
    int visited[24][24];"""

    content = content.replace(old, new)

    with open('src/generate.c', 'w') as f:
        f.write(content)

update_file()
