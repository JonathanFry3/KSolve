../ran --seed 3024322 --end 1 --draw 3 --mvlimit 10000000 > /dev/null # prime system
../ran --seed 29345 --incr 2349 --end $1 --draw 3 --mvlimit 10000000 > sm-3-test-$1.txt\
&& python3 compare.py sm-3-base-1000.txt sm-3-test-$1.txt
