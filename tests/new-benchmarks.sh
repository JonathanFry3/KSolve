#!/usr/bin/sh
./lg-1.sh 1000 | tee benchmark-comparison.txt
./lg-3.sh 1000 | tee -a benchmark-comparison.txt
