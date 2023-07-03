GET DATA
  /TYPE=TXT
  /FILE="/home/jon/prj/KSolve/tests/base3-2000-110M-moves.txt"
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


compute Large = treemoves > 10000000.
recode outcome (0 1 = 1) (2 = 0) (else = 9) into Solvable.
format large solvable (f1).
missing values solvable (9).
variable labels Large "Moves in state tree".
value labels /solvable 0 "no" 1 "yes" 9  "unknown"
		   / large 0 "<= 10M" 1 " > 10M".
CROSSTABS 
	/TABLES= large BY solvable
	/FORMAT=AVALUE TABLES
	/STATISTICS=CHISQ 
	/CELLS=COUNT ROW COLUMN TOTAL.

