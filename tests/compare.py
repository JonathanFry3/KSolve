import pandas as pd
import sys
if len(sys.argv) == 3:
        print ("")
        print("Comparing", sys.argv[1], "with", sys.argv[2])
        base = pd.read_csv(sys.argv[1], sep="\t")
        test = pd.read_csv(sys.argv[2], sep="\t")
elif len(sys.argv) == 1:
        base = pd.read_csv("tests/lg-1-base-1000.txt", sep="\t")
        test = pd.read_csv("tests/lg-1-test-20.txt", sep="\t")
else:
        print("Expected two filenames or none.")
        exit
assert test.shape[0] <= base.shape[0]
indexes = test["seed"]
test.set_index("seed",inplace=True)
base.set_index("seed",inplace=True)
basex = base.loc[indexes]
print(pd.crosstab(index=basex["outcome"],columns=test["outcome"],rownames=["Base"], colnames=["Test"]))
movesPlus = test["moves"] > basex["moves"]
print("  Moves+:",list(test.row[movesPlus]))
movesMinus = test["moves"] < basex["moves"]
print("  Moves-:",list(test.row[movesMinus]))
timePlus = filter(lambda i:  basex.time[i] < test.time[i], indexes)
print("   Time+:",len(test.row[timePlus]))
timeMinus = filter(lambda i:  basex.time[i] > test.time[i], indexes)
print("   Time-:",len(test.row[timeMinus]))
form = "{:12.2f}"
pctForm = "{:+.2f}%"
baseTot = sum(basex.time)
testTot = sum(test.time)
print("Base time:", form.format(baseTot))
print("Test time:", form.format(testTot), pctForm.format((testTot-baseTot)*100./baseTot))
baseTot = sum(basex.fringe)
testTot = sum(test.fringe)
print ("Base fringe size:", baseTot )
print ("Test fringe size:", testTot, pctForm.format((testTot-baseTot)*100./baseTot) )
baseTot = sum(basex.closed)
testTot = sum(test.closed)
print ("Base closed list size:", baseTot )
print ("Test closed list size:", testTot, pctForm.format((testTot-baseTot)*100./baseTot) )
baseTot = sum(basex.mvtree)
testTot = sum(test.mvtree)
print ("Base move tree size:", baseTot )
print ("Test move tree size:", testTot, pctForm.format((testTot-baseTot)*100./baseTot) )
