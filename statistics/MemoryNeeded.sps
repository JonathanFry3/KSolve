GET DATA
  /TYPE=TXT
  /FILE="/home/jon/prj/KSolve/ran20kd3.txt"
  /ARRANGEMENT=DELIMITED
  /DELCASE=LINE
  /FIRSTCASE=2
  /DELIMITERS="\t"
  /VARIABLES=
    row F3.0
    seed F7.0
    threads F1.0
    draw F1.0
    outcome F1.0
    moves F3.0
    passes F1.0
    time F7.3
    fringe F8.0
    branches F9.0
    treemoves F9.0.
missing values outcome(3).
value labels outcome 0 "Solved Mimimal" 1 Solved 2 Impossible 3 "Gave Up".    
FREQUENCIES
	/VARIABLES= outcome
	/FORMAT=AVALUE TABLE
	/STATISTICS=NONE.
NUMERIC mmoves.
COMPUTE mmoves = trunc((treemoves+10000000-1)/10000000)*10.
VARIABLE LABEL mmoves 'Needs no more than this many millions of moves'.
format mmoves(f3.0).
missing values mmoves().
frequencies /variables = mmoves/statistics=none.

