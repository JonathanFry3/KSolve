import pandas as pd
import sys
base = pd.read_csv("tests/draw3.out", sep="\t")
bigx = base["outcome"] == 3
bigs = list(base["seed"][bigx])
f = open("tests/big3.txt","w")
for s in bigs:
    print("R", s, file=f)
f.close()
