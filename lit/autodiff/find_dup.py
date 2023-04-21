import os
# search all files with ending for multiple occurences of a string in one line

text="-d ad_func"
ending=".thorin"

for root, dirs, files in os.walk("."):
    for file in files:
        if not file.endswith(ending):
            continue
        path=os.path.join(root, file)
        with open(path, "r") as f:
            for line in f:
                if line.count(text) > 1:
                    print(path)
                    break
