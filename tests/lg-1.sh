../ran --seed 405185 --end 1 --mvlimit 50000000 > /dev/null # prime system
../ran --seed 29351 --incr 2349 --end $1 --mvlimit 50000000 > lg-1-test-$1.txt\
&& python3 compare.py lg-1-base-1000.txt lg-1-test-$1.txt

