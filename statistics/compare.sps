GET DATA
  /TYPE=TXT
  /FILE="/home/jon/prj/KSolve/tests/lg-1-100-test.txt"
  /ARRANGEMENT=DELIMITED
  /DELCASE=LINE
  /FIRSTCASE=2
  /DELIMITERS="\t"
  /VARIABLES=
    test_row F3.0
    base_seed F7.0
    test_threads F1.0
    test_draw F1.0
    test_outcome F1.0
    test_moves F3.0
    test_passes F1.0
    test_time F7.3
    test_frmax F8.0
    test_branches F9.0
    test_treemoves F9.0.
missing values test_outcome(3).
value labels test_outcome 0 "Solved Mimimal" 1 Solved 2 Impossible 3 "Gave Up".    

match files table = "/home/jon/prj/KSolve/statistics/lg-1-base-1000.sav"
		         / file = * 
		         by base_seed.
compute slower = test_time > base_time.
format slower(f1.0).
means base_treemoves by slower.
