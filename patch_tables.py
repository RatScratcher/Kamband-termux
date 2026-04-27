import re

with open('src/tables.c', 'r') as f:
    content = f.read()

start_str = '\t{"Resist nether", "You resist nether.",\n\t\t"You stop resisting nether."},\n\n  /*-*-*/\n\n'
end_str = '\t{"Echolocation Pulse",\n\t\t\t"You feel a thrumming in your mind.",\n\t\t"The thrumming in your mind stops."},\n'

start_idx = content.find(start_str)
end_idx = content.find(end_str)

if start_idx != -1 and end_idx != -1:
    replacement = start_str
    for i in range(25):
        replacement += '\t{NULL, NULL, NULL},\n'
    replacement += '\n' + end_str

    new_content = content[:start_idx] + replacement + content[end_idx + len(end_str):]
    with open('src/tables.c', 'w') as f:
        f.write(new_content)
    print("Replaced!")
else:
    print("Could not find start or end index.")
    print(f"start_idx: {start_idx}, end_idx: {end_idx}")
