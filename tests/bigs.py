import pandas as pd
import sys
base = pd.read_csv("tests/lg-1-base-1000.txt", sep="\t")
bigx = base["outcome"] == 3
bigs = list(base["seed"][bigx])
f = open("tests/bigprobs.txt","w")
for s in bigs:
    print("R", s, file=f)
f.close()
