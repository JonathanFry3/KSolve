GET DATA
  /TYPE=TXT
  /FILE="C:\Users\Jon\prj\KSolve\thread-test-MSVC-1.txt"
  /ARRANGEMENT=DELIMITED
  /DELCASE=LINE
  /FIRSTCASE=2
  /DELIMITERS="\t,"
  /QUALIFIER='"'
  /VARIABLES=
    row F3.0
    seed F5.0
    threads F2.0
    draw F1.0
    outcome F1.0
    moves F3.0
    passes F1.0
    time F6.3
    frmax F6.0
    branches F7.0
    treemoves F7.0.
 GRAPH SCATTERPLOT(BIVARIATE) = threads WITH time.
 compute moves_generated_per_second = treemoves/time.
 GRAPH scatterplot(bivariate) = threads with moves_generated_per_second.
sort cases threads.
AGGREGATE OUTFILE=* MODE=ADDVARIABLES
	/BREAK= threads
	/mean_time "Mean Time" = MEAN (time).





