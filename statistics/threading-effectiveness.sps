GET DATA
  /TYPE=TXT
  /FILE="/home/jon/prj/KSolve/statistics/thread-test-gcc-2.txt"
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
 sort cases threads.
AGGREGATE OUTFILE=* MODE=ADDVARIABLES
	/BREAK= threads
	/min_time "Minimum Time" = MIN(time).
	 (time).
GRAPH SCATTERPLOT(BIVARIATE) = threads WITH min_time.
 compute gmps= treemoves/min_time.
 variable label gmps "Generated moves per second".
 GRAPH scatterplot(bivariate) = threads with gmps.
 
compute constant = 1.
 aggregate out=* mode=add / break = constant / gl_max_time "Global Maximum Time" = max(min_time).
 compute Speedup = gl_max_time/min_time.
 GRAPH SCATTERPLOT(BIVARIATE) = threads WITH Speedup.
 select if (threads <= 16).
 GRAPH SCATTERPLOT(BIVARIATE) = threads WITH Speedup.
REGRESSION
	/VARIABLES= threads
	/DEPENDENT= Speedup
	/METHOD=ENTER
	/STATISTICS=COEFF R.

 