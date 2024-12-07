# Project size (in code lines) counting script

import os
import sys

file_stat = {}

def visit_directory(path: str):
    for f in map(lambda f: path + '/' + f, os.listdir(path)):
        if os.path.isdir(f):
            visit_directory(f)
        else:
            with open(f) as file:
                file_stat[f] = len(file.readlines())

dirs = sys.argv[1:]
if len(dirs) == 0:
    dirs = ['.']

for dir in dirs:
    visit_directory(dir)

ext_stat = {}

for (name, l) in file_stat.items():
    index = name.rfind('.')
    extension = name[index:] if index != -1 else ''
    if extension not in ext_stat:
        ext_stat[extension] = 0
    ext_stat[extension] += l

print(ext_stat)