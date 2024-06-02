GET DATA
  /TYPE=TXT
  /FILE="/home/jon/prj/KSolve/tests/lg-1-base-1000.txt"
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
    frmax F8.0
    branches F9.0
    treemoves F9.0.
dataset name base.
GET DATA
  /TYPE=TXT
  /FILE="/home/jon/prj/KSolve/tests/lg-1-test-1000.txt"
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
    frmax F8.0
    branches F9.0
    treemoves F9.0.
dataset name test.
missing values outcome(3).
value labels outcome 0 "Solved Mimimal" 1 Solved 2 Impossible 3 "Gave Up".    

add files file = base/ in='base'/ file=test/ in='base'.


