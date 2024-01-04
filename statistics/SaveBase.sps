GET DATA
  /TYPE=TXT
  /FILE="/home/jon/prj/KSolve/tests/lg-1-base-1000.txt"
  /ARRANGEMENT=DELIMITED
  /DELCASE=LINE
  /FIRSTCASE=2
  /DELIMITERS="\t"
  /VARIABLES=
    base_row F3.0
    base_seed F7.0
    base_threads F1.0
    base_draw F1.0
    base_outcome F1.0
    base_moves F3.0
    base_passes F1.0
    base_time F7.3
    base_frmax F8.0
    base_branches F9.0
    base_treemoves F9.0.
value labels base_outcome 0 "Solved Mimimal" 1 Solved 2 Impossible 3 "Gave Up".    

save outfile="/home/jon/prj/KSolve/statistics/lg-1-base-1000.sav".

GET DATA
  /TYPE=TXT
  /FILE="/home/jon/prj/KSolve/tests/lg-3-base-1000.txt"
  /ARRANGEMENT=DELIMITED
  /DELCASE=LINE
  /FIRSTCASE=2
  /DELIMITERS="\t"
  /VARIABLES=
    base_row F3.0
    base_seed F7.0
    base_threads F1.0
    base_draw F1.0
    base_outcome F1.0
    base_moves F3.0
    base_passes F1.0
    base_time F7.3
    base_frmax F8.0
    base_branches F9.0
    base_treemoves F9.0.
value labels base_outcome 0 "Solved Mimimal" 1 Solved 2 Impossible 3 "Gave Up".    

save outfile="/home/jon/prj/KSolve/statistics/lg-3-base-1000.sav".
