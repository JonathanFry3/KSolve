GET DATA
  /TYPE=TXT
  /FILE="/home/jon/prj/KSolve/tests/big2000.txt"
  /ARRANGEMENT=DELIMITED
  /DELCASE=LINE
  /FIRSTCASE=2
  /DELIMITERS="\t"
  /QUALIFIER='"'
  /VARIABLES=
    row f3.0
    seed F7.0
    threads F1.0
    draw F1.0
    outcome F1.0
    moves F3.0
    passes F1.0
    time F7.3
    frmax F7.0
    branches F8.0
    treemoves F8.0.
    

compute Large = treemoves > 100000000.
recode outcome(1=0).
value labels outcome 0 "Solved" 2 "Impossible" 3 "Too big".
format large  (f1).
missing values outcome(3).

CROSSTABS 
	/TABLES= large BY outcome
	/FORMAT=AVALUE TABLES
	/STATISTICS=CHISQ 
	/CELLS=COUNT ROW COLUMN TOTAL.

