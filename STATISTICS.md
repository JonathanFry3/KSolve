*This is currently out of date*
These statistics were based on 2000 random deals run once with draw 1 specified 
and once with draw 3 specified.  The commands required to reproduce the sample 
using the _ran_ program from this repository are:

    ran -s 109510906 -i 328743 -e 2000 -st 60000000 > ran-d1-e2000.txt
    ran -s 109510906 -i 328743 -e 2000 -st 60000000 -d 3 > ran-d3-e2000.txt
## Winning Proportion
To compute the proportion of winnable deals, one must decide what to do with deals too complex to
solve using the number of states allowed.  In his statistics report, @shootme concluded that such deals should
be counted as solved.  I tested statistically whether the outcome proportion varied with the number of unique states required  
for the decided deals and found no evidence of dependence, so I see no reason to assign those deals to either 
category.  They are excluded from these statistics.

Draw | Winnable
---- | --------
1    | 92.97%
3    | 82.50%
## How Many States are Needed
This table reports the observed proportion of deals that were decided (solved or found to be unsolvable)
using a number of unique states no greater than the given number.  

State Limit|Draw 1 |Draw 3
-----------|-------|------
 10,000,000|79.35% |89.65%
 20,000,000|89.70% |95.95%
 30,000,000|93.85% |98.10%
 40,000,000|95.35% |98.60%
 50,000,000|96.10% |99.05%
 60,000,000|96.70% |99.15%
 ## How Much Memory is Needed
 These figures were acquired by running a deal too complex to be decided with any of the 
 state limits tested and observing the peak memory used.  They were run on a Windows 10 machine
 with the _ran_ program compiled with the Microsoft MVSC compiler, although the compiler used 
 will make little difference. The figures would be nearly the same for the _KSolve_ program.

 State Limit|Peak Memory
 -----------|-----------
 10,000,000 |640MB
 20,000,000 |1280MB
 30,000,000 |2440MB
 40,000,000 |2580MB
 50,000,000 |2690MB
 60,000,000 |4900MB 

