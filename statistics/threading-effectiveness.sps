GET DATA
  /TYPE=TXT
  /FILE="/home/jon/prj/KSolve/statistics/thread-test-34041.csv"
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
    time F6.3.

sort cases threads.
GRAPH SCATTERPLOT(BIVARIATE) = threads WITH time.
AGGREGATE OUTFILE=* MODE=ADDVARIABLES
	/BREAK= threads
	/min_time "Minimum Time" = MIN(time).
 
compute constant = 1.
 aggregate out=* mode=add / break = constant / gl_max_time "Global Maximum Time" = max(min_time).
 compute Speedup = gl_max_time/min_time-1.
 GRAPH SCATTERPLOT(BIVARIATE) = threads WITH Speedup.
 select if (threads <= 16).
 GRAPH SCATTERPLOT(BIVARIATE) = threads WITH Speedup.
 compute square = threads*threads.
REGRESSION
	/VARIABLES= threads square
	/DEPENDENT= Speedup
	/METHOD=ENTER
	/STATISTICS=COEFF R.
temporary.
compute threads = threads-1.
compute hi_threads = threads*(threads > 8).
 REGRESSION
	/VARIABLES= threads hi_threads
	/DEPENDENT= Speedup
	/origin
	/METHOD=ENTER
	/STATISTICS=COEFF R.