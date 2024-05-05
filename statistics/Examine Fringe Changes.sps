GET DATA
  /TYPE=TXT
  /FILE="/home/jon/prj/KSolve/tests/lg-1-test-1000.txt"
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
dataset name lg1test. 
dataset display.
get file = .

match files table = "/home/jon/prj/KSolve/statistics/lg-1-base-1000.sav"
		         / file = lg1test
		         by base_seed.
compute time_diff = (test_time - base_time).
format time_diff(f8.3).
	
sort cases by test_outcome (a) time_diff (d).means time_diff by test_outcome/ cells=mean semean stddev count.
EXAMINE 
	/VARIABLES= time_diff
	BY  test_outcome
	/PLOT = BOXPLOT HISTOGRAM
	/COMPARE = GROUPS
	/MISSING=LISTWISE.
means time_diff by test_outcome/cells=mean semean stddev count.
