../ran --seed 3024322 --end 1 --draw 3 --mvlimit 50000000 > /dev/null # prime system
../ran --seed 29345 --incr 2349 --end $1 --draw 3 --mvlimit 50000000 > lg-3-test-$1.txt\
&& python3 compare.py lg-3-base-1000.txt lg-3-test-$1.txt
