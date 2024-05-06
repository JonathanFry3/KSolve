../ran --seed 3024322 --end 1 --mvlimit 10000000 > /dev/null # prime system
../ran --seed 29345 --incr 2349 --end $1 --mvlimit 10000000 > sm-1-test-$1.txt\
&& python3 compare.py sm-1-base-1000.txt sm-1-test-$1.txt
