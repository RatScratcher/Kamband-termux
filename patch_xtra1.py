import re

with open('src/xtra1.c', 'r') as f:
    content = f.read()

# We need to remove from "case MUT_MINUS_INT:" up to the end of "case MUT_MINUS_FIGHT:" block.
# Wait, "case MUT_POISONED" etc. are in there too. Let's just remove all these cases.

cases_to_remove = [
    "MUT_MINUS_INT", "MUT_MINUS_STR", "MUT_MINUS_CON", "MUT_MINUS_WIS", "MUT_MINUS_CHR", "MUT_MINUS_DEX",
    "MUT_MINUS_SPEED", "MUT_AGGRAVATE", "MUT_MINUS_AC", "MUT_BLIND", "MUT_COWARD", "MUT_HALLUC",
    "MUT_CONFUSED", "MUT_LEGLESS", "MUT_POISONED", "MUT_PARALYZED", "MUT_BLEEDING", "MUT_PARASITES",
    "MUT_STUNNED", "MUT_TELEPORT", "MUT_EXP_DRAIN", "MUT_MINUS_DEVICES", "MUT_MINUS_SAVE",
    "MUT_MINUS_STEALTH", "MUT_MINUS_FIGHT"
]

# The structure is `case MUT_XYZ:\n ... break;\n\n`
for case in cases_to_remove:
    # Regex to match `\t\tcase MUT_XYZ:[\s\S]*?\t\t\tbreak;\n\n`
    # Also some might not have double newlines, but let's match up to `break;\n` and any following empty lines
    pattern = re.compile(r'\t\tcase ' + case + r':\n.*?\n\t\t\tbreak;\n(\n*)', re.DOTALL)
    # Actually MUT_TELEPORT etc. might just have `p_ptr->teleport = TRUE;\n\t\t\tbreak;`
    content = pattern.sub(r'', content)

with open('src/xtra1.c', 'w') as f:
    f.write(content)
print("Removed cases.")
