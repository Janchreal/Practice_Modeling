import re
path = r"D:\ProgramFiles\Practice_Modeling_47\widget.cpp"
bad = []
with open(path, encoding="utf-8", errors="replace") as f:
    for i, line in enumerate(f, 1):
        if "tr(" in line and re.search(r'tr\("[^"]*\),\s*$', line):
            bad.append((i, line.rstrip()[:140]))
print("count", len(bad))
for i, l in bad[:60]:
    print(i, l)
