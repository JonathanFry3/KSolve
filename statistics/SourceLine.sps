GET DATA
  /TYPE=TXT
  /FILE="/home/jon/prj/KSolve/tests/sources.txt"
  /ARRANGEMENT=DELIMITED
  /DELCASE=LINE
  /DELIMITERS=" ,"
  /QUALIFIER='"'
  /VARIABLES=
    seed F8.0
    source_line F2.0.

FREQUENCIES
	/VARIABLES= seed source_line
	/FORMAT=AVALUE TABLE
	/STATISTICS=NONE.

