import pandas as pd
import sys
if len(sys.argv) == 3:
        print ("")
        print("Comparing", sys.argv[1], "with", sys.argv[2])
        base = pd.read_csv(sys.argv[1], sep="\t")
        test = pd.read_csv(sys.argv[2], sep="\t")
elif len(sys.argv) == 1:
        base = pd.read_csv("tests/lg-1-base-1000.txt", sep="\t")
        test = pd.read_csv("tests/lg-1-test-1000.txt", sep="\t")
else:
        print("Expected two filenames or none.")
        exit
indexes = test.seed
test.set_index("seed",inplace=True)
base.set_index("seed",inplace=True)
basex = base.loc[indexes]
print(pd.crosstab(index=basex["outcome"],columns=test["outcome"],rownames=["Base"], colnames=["Test"]))

movesPlus = test["moves"] > basex["moves"]
print("  Moves+:",list(test.row[movesPlus]))

movesMinus = test["moves"] < basex["moves"]
print ("  Moves-:",list(test.row[movesMinus]))

timePlus = filter(lambda i:  basex.time[i] < test.time[i], indexes)
print ("   Time+:",len(test.row[timePlus]))

bigPlus = test["time"] - basex["time"] > 1
print ("    Big+:", list(test.row[bigPlus]))

timeMinus = filter(lambda i:  basex.time[i] > test.time[i], indexes)
print("   Time-:",len(test.row[timeMinus]))

bigMinus = test["time"] - basex["time"] < -1
print ("    Big-:", list(test.row[bigMinus]))

form = "{:12.2f}"
pctForm = "{:+.2f}%"
baseTime = sum(basex.time)
testTime = sum(test.time)
print("Base time:", form.format(baseTime))
print("Test time:", form.format(testTime), pctForm.format((testTime-baseTime)*100./baseTime))

def PrintSingleCount(which, label, column):
        total = sum(column)
        print ("{0} {1}".format(which, label), f"{total:,}")
def PrintPair(label, baseColumn, testColumn):
        baseTot = sum(baseColumn)
        print ("Base {}".format(label), f"{baseTot:,}")
        testTot = sum(testColumn)
        diffString = f"{testTot:,}"
        pctString = pctForm.format((testTot-baseTot)*100./baseTot)
        print ("Test {}".format(label), diffString, pctString) 
def PrintCount(label, colName):
        if (colName in basex):
                PrintPair(label, basex[colName], test[colName])
        else:
                PrintSingleCount("Test", label, test[colName])

PrintCount("fringe size:      ", "fringe",)
PrintCount("move tree size:   ", "mvtree")
PrintCount("advances count:   ", "advances")
PrintCount("closed list size: ", "closed")
