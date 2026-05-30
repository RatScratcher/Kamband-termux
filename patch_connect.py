import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    old_connect = """	/* Loop until fully connected */
	while (loop_safe++ < 100)
	{"""

    new_connect = """	/* Loop until fully connected */
	while (loop_safe++ < 30)
	{"""

    content = content.replace(old_connect, new_connect)

    with open('src/generate.c', 'w') as f:
        f.write(content)

update_file()
