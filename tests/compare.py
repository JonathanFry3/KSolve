import pandas as pd
import os
# print(os.getcwd()) printed .../KSolve
base = pd.read_csv("tests/base1000.txt", sep="\t")
test = pd.read_csv("tests/test1000.txt", sep="\t")
assert test.shape[0] <= base.shape[0]
indexes = list(range(test.shape[0]))
basex = base.loc[indexes]
#print((test.outcome == 0) and (basex.outcome != 0))
solvesPlus = basex["outcome"].isin([2,3,4]) & test["outcome"].isin([0,1])
print("Solves+:",list(test.row[solvesPlus]))
solvesMinus = basex["outcome"].isin([0,1]) & test["outcome"].isin([2,3,4])
print("Solves-:",list(test.row[solvesMinus]))
imposPlus = filter(lambda i: (basex.outcome[i] != 2) and (test.outcome[i] == 2), indexes)
print("Imposs+:",list(test.row[imposPlus]))
imposMinus = filter(lambda i: (basex.outcome[i] == 2) and (test.outcome[i] != 2), indexes)
print("Imposs-:",list(test.row[imposMinus]))
movesPlus = basex["outcome"].isin([0,1]) & test["outcome"].isin([0,1]) \
        & (test["moves"] > basex["moves"])
print(" Moves+:",list(test.row[movesPlus]))
movesMinus = basex["outcome"].isin([0,1]) & test["outcome"].isin([0,1]) \
        & (test["moves"] < basex["moves"])
print(" Moves-:",list(test.row[movesMinus]))
timePlus = filter(lambda i:  basex.time[i] < test.time[i], indexes)
print("  Time+:",len(list(test.row[timePlus])))
timeMinus = filter(lambda i:  basex.time[i] > test.time[i], indexes)
print("  Time-:",len(list(test.row[timeMinus])))
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
