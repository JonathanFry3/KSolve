../ran --seed 1509215 --end 1 --mvlimit 50000000 > /dev/null # prime system
../ran --seed 29345 --incr 2349 --end $1 --mvlimit 50000000 > lg-1-test-$1.txt\
&& python3 compare.py lg-1-base-1000.txt lg-1-test-$1.txt
