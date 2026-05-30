import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    old_gold_loop = """	/* Place some small gold piles */
	for (i = 0; i < 200; i++)
	{
		int y, x;
		int d = 0;
		while (d < 1000)
		{"""

    new_gold_loop = """	/* Place some small gold piles */
	for (i = 0; i < 100; i++)
	{
		int y, x;
		int d = 0;
		while (d < 200)
		{"""

    content = content.replace(old_gold_loop, new_gold_loop)

    with open('src/generate.c', 'w') as f:
        f.write(content)

update_file()
