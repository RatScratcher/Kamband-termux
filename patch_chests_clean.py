import re

def update_file():
    with open('src/generate.c', 'r') as f:
        content = f.read()

    # Clean up chest_count reset we introduced earlier
    content = content.replace("		/* Reset tracked chests */\n		chest_count = 0;", "")

    with open('src/generate.c', 'w') as f:
        f.write(content)

update_file()
