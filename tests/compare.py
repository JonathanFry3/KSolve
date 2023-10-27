import pandas as pd
import sys
if len(sys.argv) == 3:
        print ("")
        print("Comparing", sys.argv[1], "with", sys.argv[2])
        base = pd.read_csv(sys.argv[1], sep="\t")
        test = pd.read_csv(sys.argv[2], sep="\t")
elif len(sys.argv) == 1:
        base = pd.read_csv("tests/big3-2000.txt", sep="\t")
        test = pd.read_csv("tests/bigtest3-2000.txt", sep="\t")
else:
        print("Expected two filenames or none.")
        exit
assert test.shape[0] <= base.shape[0]
indexes = list(range(test.shape[0]))
basex = base.loc[indexes]
print(pd.crosstab(index=basex["outcome"],columns=test["outcome"],rownames=["Base"], colnames=["Test"]))
movesPlus = basex["outcome"].isin([0,1]) & test["outcome"].isin([0,1]) \
        & (test["moves"] > basex["moves"])
print("  Moves+:",list(test.row[movesPlus]))
movesMinus = basex["outcome"].isin([0,1]) & test["outcome"].isin([0,1]) \
        & (test["moves"] < basex["moves"])
print("  Moves-:",list(test.row[movesMinus]))
timePlus = filter(lambda i:  basex.time[i] < test.time[i], indexes)
print("   Time+:",len(list(test.row[timePlus])))
timeMinus = filter(lambda i:  basex.time[i] > test.time[i], indexes)
print("   Time-:",len(list(test.row[timeMinus])))
form = "{:12.2f}"
pctForm = "{:+.2f}%"
baseTot = sum(basex.time)
testTot = sum(test.time)
print("Base time:", form.format(baseTot))
print("Test time:", form.format(testTot), pctForm.format((testTot-baseTot)*100./baseTot))
baseTot = sum(basex.treemoves)
testTot = sum(test.treemoves)
print ("Base tree moves:", baseTot )
print ("Test tree moves:", testTot, pctForm.format((testTot-baseTot)*100./baseTot) )
