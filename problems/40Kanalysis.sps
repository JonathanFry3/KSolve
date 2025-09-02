GET DATA
  /TYPE=TXT
  /FILE="/home/jon/prj/KSolve/ran40kd3.txt"
  /ARRANGEMENT=DELIMITED
  /DELCASE=LINE
  /FIRSTCASE=2
  /DELIMITERS="\t,"
  /QUALIFIER='"'
  /VARIABLES=
    row F3.0
    seed F3.0
    threads F2.0
    draw F1.0
    outcome F1.0
    moves F3.0
    passes F2.0
    time F8.3
    fringe F9.0
    advances F9.0
    mvtree F9.0.
VARIABLE WIDTH row (4).
VARIABLE WIDTH seed (4).
VARIABLE WIDTH draw (5).
VARIABLE WIDTH moves (5).
VARIABLE WIDTH passes (7).
VARIABLE WIDTH time (6).
compute fringe = fringe/1000000.
compute advances = advances/1000000.
compute mvtree = mvtree/1000000.
format fringe to mvtree (f10.4).

dataset name full.

temporary.
select if outcome ne 3.
GRAPH SCATTERPLOT(BIVARIATE) = advances WITH time.
FREQUENCIES
	/VARIABLES= outcome
	/FORMAT=AVALUE TABLE
	/STATISTICS=NONE.

select if time > advances*0.5 - .14.
dataset name outliers.
frequencies var = outcome.

COMPUTE log_advances = lg10(advances).
COMPUTE log_time = lg10(time).
GRAPH SCATTERPLOT(BIVARIATE) = log_advances WITH log_time.
	
SORT CASES BY  outcome time(D).
split file by outcome.

REGRESSION
	/VARIABLES= advances
	/DEPENDENT= time
	/ORIGIN
	/METHOD=ENTER
	/STATISTICS=COEFF.

dataset activate full.
temporary.
select if advances lt .001  and outcome ne 3 and advances > .5e-4 and time < .012.
GRAPH SCATTERPLOT(BIVARIATE) = advances WITH time.
