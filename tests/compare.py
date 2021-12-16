import pandas as pd
import os
# print(os.getcwd()) printed .../KSolve
base = pd.read_csv("tests/base1000.txt", sep="\t")
# print(base.loc[0:5])
test = pd.read_csv("tests/test10.txt", sep="\t")
# print(test.loc[0:5])
# print(test.moves)
# print(test.shape[0])
assert test.shape[0] <= base.shape[0]
indexes = list(range(test.shape[0]))
basex = base.loc[indexes]
#print((test.outcome == 0) & (basex.outcome != 0))
solvesPlus = filter(lambda i: (basex.outcome[i] != 0) & (test.outcome[i] == 0), indexes)
print("Solves+:",list(test.row[solvesPlus]))
solvesMinus = filter(lambda i: (basex.outcome[i] == 0) & (test.outcome[i] != 0), indexes)
print("Solves-:",list(test.row[solvesMinus]))
movesPlus = filter(lambda i: \
    ((basex.outcome[i] == test.outcome[i]) & (basex.moves[i] < test.moves[i])), indexes)
print(" Moves+:",list(test.row[movesPlus]))
movesMinus = filter(lambda i: \
    ((basex.outcome[i] == test.outcome[i]) & (basex.moves[i] > test.moves[i])), indexes)
print(" Moves-:",list(test.row[movesMinus]))
timePlus = filter(lambda i:  basex.time[i] < test.time[i], indexes)
print("  Time+:",len(list(test.row[timePlus])))
timeMinus = filter(lambda i:  basex.time[i] > test.time[i], indexes)
print("  Time-:",len(list(test.row[timeMinus])))
