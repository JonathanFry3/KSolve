import pandas as pd
import sys
if len(sys.argv) == 3:
        print ("")
        print("Comparing", sys.argv[1], "with", sys.argv[2])
        base = pd.read_csv(sys.argv[1], sep="\t")
        test = pd.read_csv(sys.argv[2], sep="\t")
elif len(sys.argv) == 1:
        base = pd.read_csv("tests/sm-1-base-100.txt", sep="\t")
        test = pd.read_csv("tests/sm-1-test-100.txt", sep="\t")
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
print ("Base fringe size:     ", f"{baseTot:,}" )
print ("Test fringe size:     ", f"{testTot:,}", pctForm.format((testTot-baseTot)*100./baseTot) )
baseTot = sum(basex.advances)
testTot = sum(test.advances)
print ("Base advances count:  ", f"{baseTot:,}" )
print ("Test advances count:  ", f"{testTot:,}", pctForm.format((testTot-baseTot)*100./baseTot) )
baseTot = sum(basex.mvtree)
testTot = sum(test.mvtree)
print ("Base move tree size:  ", f"{baseTot:,}" )
print ("Test move tree size:  ", f"{testTot:,}", pctForm.format((testTot-baseTot)*100./baseTot) )
